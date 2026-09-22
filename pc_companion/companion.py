"""
Guition JC3636K718C PC Companion Service - Stage 2
Zero-Admin Windows Telemetry, Media GSMTC & Meeting Controller
"""

import sys
import os
import time
import datetime
import json
import asyncio
import io
import psutil
import serial
import serial.tools.list_ports
from PIL import Image

# Windows Media (GSMTC) WinRT modules
try:
    from winrt.windows.media.control import (
        GlobalSystemMediaTransportControlsSessionManager as MediaManager,
        GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus
    )
    from winrt.windows.storage.streams import DataReader
    HAS_WINRT = True
except ImportError:
    HAS_WINRT = False

try:
    from teams_client import TeamsClient
except ImportError:
    from pc_companion.teams_client import TeamsClient


def find_esp32_port():
    """Locate the ESP32-S3 serial port, bypassing Bluetooth port collisions."""
    if sys.platform == "win32":
        try:
            import winreg
            path = r"SYSTEM\CurrentControlSet\Control\DeviceClasses\{86e0d1e0-8089-11d0-9ce4-08003e301f73}"
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path) as key:
                i = 0
                while True:
                    try:
                        sub = winreg.EnumKey(key, i)
                        if "VID_303A" in sub.upper() and "MI_01" in sub.upper():
                            return r"\\?\\" + sub[4:]
                        i += 1
                    except OSError:
                        break
        except Exception:
            pass

    ports = serial.tools.list_ports.comports()
    for p in ports:
        if p.vid == 0x303A and "Bluetooth" not in p.description:
            return p.device
    for p in ports:
        if ("ESP32" in p.description or "USB JTAG" in p.description) and "Bluetooth" not in p.description:
            return p.device
    return None


def is_workstation_locked():
    """Detect if the Windows workstation is locked without requiring admin privileges."""
    try:
        import ctypes
        user32 = ctypes.windll.User32
        desk = user32.OpenInputDesktop(0, False, 0x0001)
        if desk == 0:
            return True
        user32.CloseDesktop(desk)
        return False
    except Exception:
        return False


def extract_dominant_accent_color(img_bytes: bytearray) -> tuple[int, int, int]:
    """Extract the most vibrant, non-neutral accent color from album art bytes."""
    try:
        img = Image.open(io.BytesIO(img_bytes)).convert("RGB")
        w, h = img.size
        # Crop 10% edges to discard black letterboxing/letterbars
        crop_w = int(w * 0.1)
        crop_h = int(h * 0.1)
        img_crop = img.crop((crop_w, crop_h, w - crop_w, h - crop_h))

        small = img_crop.resize((24, 24), Image.Resampling.BOX)
        colors = small.getcolors(maxcolors=24 * 24)
        if not colors:
            return 0, 210, 255

        best_color = None
        best_score = -1.0
        for count, (r, g, b) in colors:
            max_c = max(r, g, b)
            min_c = min(r, g, b)
            delta = max_c - min_c
            # Filter out near-black or near-white pixels
            if max_c < 35 or max_c > 245:
                continue
            sat = delta / max_c if max_c > 0 else 0
            bri = max_c / 255.0
            # Score emphasizes saturation and moderate brightness
            score = (sat * 2.2 + bri) * (count ** 0.5)
            if score > best_score:
                best_score = score
                best_color = (r, g, b)

        if best_color:
            return best_color
    except Exception:
        pass
    return 0, 210, 255  # Default cyan


class MediaWatcher:
    """Monitors Windows Global System Media Transport Controls (GSMTC)."""

    def __init__(self):
        self.manager = None
        self.cached_track_key = ""
        self.cached_accent = (0, 210, 255)
        self.last_status_summary = ""

    async def init_manager(self):
        if not HAS_WINRT:
            return
        try:
            self.manager = await MediaManager.request_async()
        except Exception as e:
            print(f"[Media] Failed to initialize GSMTC manager: {e}")

    async def get_media_packet(self) -> dict:
        if not HAS_WINRT or not self.manager:
            return {
                "type": "media",
                "title": "No Media Playing",
                "artist": "Tidal / YouTube / Spotify",
                "playing": False,
                "pos": 0,
                "dur": 0,
                "r": 0,
                "g": 210,
                "b": 255
            }

        try:
            session = self.manager.get_current_session()
            if not session:
                return {
                    "type": "media",
                    "title": "No Media Playing",
                    "artist": "Tidal / YouTube / Spotify",
                    "playing": False,
                    "pos": 0,
                    "dur": 0,
                    "r": 0,
                    "g": 210,
                    "b": 255
                }

            props = await session.try_get_media_properties_async()
            info = session.get_playback_info()
            timeline = session.get_timeline_properties()

            title = props.title if props and props.title else "Unknown Title"
            artist = props.artist if props and props.artist else "Unknown Artist"

            # 4 = Playing, 5 = Paused
            is_playing = False
            if info:
                # PlaybackStatus.PLAYING == 4
                is_playing = (int(info.playback_status) == 4)

            pos_ms = 0
            dur_ms = 0
            if timeline:
                if timeline.position:
                    pos_ms = int(timeline.position.total_seconds() * 1000)
                if timeline.end_time:
                    dur_ms = int(timeline.end_time.total_seconds() * 1000)

            # Check if track changed to recompute dominant artwork color
            track_key = f"{title}:{artist}"
            if track_key != self.cached_track_key:
                self.cached_track_key = track_key
                self.cached_accent = (0, 210, 255)  # Reset default

                if props and props.thumbnail:
                    try:
                        stream = await props.thumbnail.open_read_async()
                        size = stream.size
                        if size > 0:
                            reader = DataReader(stream)
                            await reader.load_async(size)
                            buf = bytearray(size)
                            reader.read_bytes(buf)
                            self.cached_accent = extract_dominant_accent_color(buf)
                    except Exception:
                        pass

                status_str = f"[{'PLAYING' if is_playing else 'PAUSED'}] '{title}' by '{artist}' (Accent: RGB{self.cached_accent})"
                if status_str != self.last_status_summary:
                    print(f"[Media] {status_str}")
                    self.last_status_summary = status_str

            r, g, b = self.cached_accent
            return {
                "type": "media",
                "title": title,
                "artist": artist,
                "playing": is_playing,
                "pos": pos_ms,
                "dur": dur_ms,
                "r": r,
                "g": g,
                "b": b
            }

        except Exception as e:
            # Re-request manager on failure
            try:
                self.manager = await MediaManager.request_async()
            except Exception:
                pass
            return {
                "type": "media",
                "title": "No Media Playing",
                "artist": "Tidal / YouTube / Spotify",
                "playing": False,
                "pos": 0,
                "dur": 0,
                "r": 0,
                "g": 210,
                "b": 255
            }


async def async_main():
    print("=" * 65)
    print(" Guition JC3636K718C PC Companion Service - Stage 2")
    print(" Windows Media GSMTC + Hardware Telemetry + Lock Detection")
    print("=" * 65)

    if not HAS_WINRT:
        print("[!] Note: winrt-Windows.Media.Control not found.")
        print("    Install dependencies via: pip install -r pc_companion/requirements.txt\n")

    port = None
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        print("Scanning for Guition ESP32 USB COM port...")
        port = find_esp32_port()

    if not port:
        print("[-] No ESP32 device found on USB.")
        print("    Available ports:")
        for p in serial.tools.list_ports.comports():
            print(f"      - {p.device}: {p.description} (VID: {hex(p.vid) if p.vid else 'None'})")
        print("\nUsage: python pc_companion/companion.py [COM_PORT]")
        return

    print(f"[+] Connecting to {port} at 115200 baud...")
    try:
        ser = serial.Serial(port, 115200, timeout=0.05)
    except Exception as e:
        print(f"[-] Failed to open {port}: {e}")
        return

    print(f"[+] Connected to {port}! Press Ctrl+C to exit.\n")

    media_watcher = MediaWatcher()
    await media_watcher.init_manager()

    teams_client = TeamsClient()
    teams_task = asyncio.create_task(teams_client.run())

    last_bytes_recv = psutil.net_io_counters().bytes_recv
    last_bytes_sent = psutil.net_io_counters().bytes_sent
    last_net_check = time.time()
    last_media_sent = 0
    last_teams_sent = 0

    try:
        while True:
            now = datetime.datetime.now()
            current_time = time.time()

            # 1. Send Clock Sync Packet (every ~500ms)
            clock_pkt = {
                "type": "clock",
                "hour": now.hour,
                "minute": now.minute,
                "sec": now.second,
                "date": now.strftime("%d %b"),
                "day": now.strftime("%A")
            }
            ser.write((json.dumps(clock_pkt) + "\n").encode("utf-8"))

            # 2. Send Hardware Telemetry (every 1.0 second)
            dt = current_time - last_net_check
            if dt >= 1.0:
                net = psutil.net_io_counters()
                down_mb = ((net.bytes_recv - last_bytes_recv) / dt) / (1024 * 1024)
                up_mb = ((net.bytes_sent - last_bytes_sent) / dt) / (1024 * 1024)
                last_bytes_recv = net.bytes_recv
                last_bytes_sent = net.bytes_sent
                last_net_check = current_time

                cpu_pct = int(psutil.cpu_percent())
                ram_pct = int(psutil.virtual_memory().percent)

                hw_pkt = {
                    "type": "hw",
                    "cpu": cpu_pct,
                    "ram": ram_pct,
                    "down": round(down_mb, 1),
                    "up": round(up_mb, 1)
                }
                ser.write((json.dumps(hw_pkt) + "\n").encode("utf-8"))

            # 3. Send Media Packet (every 500ms)
            if current_time - last_media_sent >= 0.5:
                last_media_sent = current_time
                media_pkt = await media_watcher.get_media_packet()
                ser.write((json.dumps(media_pkt) + "\n").encode("utf-8"))

            # 4. Send MS Teams State (on change or every 2.0 seconds)
            if teams_client.state_changed or (current_time - last_teams_sent >= 2.0):
                last_teams_sent = current_time
                teams_pkt = teams_client.get_packet()
                ser.write((json.dumps(teams_pkt) + "\n").encode("utf-8"))

            # 5. Check Workstation Lock State
            if is_workstation_locked():
                ser.write((json.dumps({"type": "lock", "locked": True}) + "\n").encode("utf-8"))

            # 6. Read incoming responses or events from device
            while ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    print(f"[Device]: {line}")
                    if line.startswith("{") and "teams_toggle" in line:
                        try:
                            cmd_data = json.loads(line)
                            cmd = cmd_data.get("cmd")
                            if cmd == "teams_toggle_mute":
                                await teams_client.toggle_mute()
                                ser.write((json.dumps(teams_client.get_packet()) + "\n").encode("utf-8"))
                            elif cmd == "teams_toggle_hand":
                                await teams_client.toggle_hand()
                                ser.write((json.dumps(teams_client.get_packet()) + "\n").encode("utf-8"))
                        except Exception as e:
                            print(f"[Teams] Error handling command: {e}")

            await asyncio.sleep(0.5)

    except (KeyboardInterrupt, asyncio.CancelledError):
        print("\nExiting companion service.")
    finally:
        teams_task.cancel()
        ser.close()



def main():
    try:
        asyncio.run(async_main())
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
