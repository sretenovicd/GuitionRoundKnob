import time
import datetime
import json
import sys
import serial
import serial.tools.list_ports
import psutil

def find_esp32_port():
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
    """Detect if the Windows workstation is locked without needing admin rights."""
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

def main():
    print("=" * 60)
    print("Guition JC3636K718C PC Companion (Stage 1)")
    print("Zero-Admin Host Service")
    print("=" * 60)

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
        print("\nUsage: python companion.py [COM_PORT]")
        return

    print(f"[+] Connecting to {port} at 115200 baud...")
    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
    except Exception as e:
        print(f"[-] Failed to open {port}: {e}")
        return

    print(f"[+] Connected to {port}! Press Ctrl+C to exit.\n")

    last_bytes_recv = psutil.net_io_counters().bytes_recv
    last_bytes_sent = psutil.net_io_counters().bytes_sent
    last_net_check = time.time()

    try:
        while True:
            now = datetime.datetime.now()
            current_time = time.time()

            # 1. Send Clock Sync Packet
            clock_pkt = {
                "type": "clock",
                "hour": now.hour,
                "minute": now.minute,
                "sec": now.second,
                "date": now.strftime("%d %b"),
                "day": now.strftime("%A")
            }
            ser.write((json.dumps(clock_pkt) + "\n").encode("utf-8"))

            # 2. Check Network & System Stats (every 1 second)
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

            # 3. Check Lock State
            if is_workstation_locked():
                ser.write((json.dumps({"type": "lock", "locked": True}) + "\n").encode("utf-8"))

            # 4. Read incoming responses or events from device
            while ser.in_waiting:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    print(f"[Device]: {line}")

            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\nExiting companion service.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
