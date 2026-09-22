"""
MS Teams Local WebSocket Client & Hotkey Fallback Engine
Guition JC3636K718C Smart Knob - Stage 4
"""

import asyncio
import json
import os
import sys
import ctypes
from ctypes import wintypes
import psutil

# Windows SendInput definitions for fallback hotkeys
user32 = ctypes.windll.user32
KEYEVENTF_KEYUP = 0x0002

class KEYBDINPUT(ctypes.Structure):
    _fields_ = [
        ('wVk', wintypes.WORD),
        ('wScan', wintypes.WORD),
        ('dwFlags', wintypes.DWORD),
        ('time', wintypes.DWORD),
        ('dwExtraInfo', ctypes.POINTER(wintypes.ULONG))
    ]

class INPUT(ctypes.Structure):
    class _INPUT(ctypes.Union):
        _fields_ = [('ki', KEYBDINPUT)]
    _anonymous_ = ('_input',)
    _fields_ = [('type', wintypes.DWORD), ('_input', _INPUT)]


def send_windows_hotkey(vk_codes: list[int]):
    """Press and release a combination of virtual key codes using SendInput."""
    n = len(vk_codes)
    inputs = (INPUT * (n * 2))()
    # Key down in order
    for i, vk in enumerate(vk_codes):
        inputs[i].type = 1
        inputs[i].ki.wVk = vk
        inputs[i].ki.dwFlags = 0
    # Key up in reverse order
    for i, vk in enumerate(reversed(vk_codes)):
        idx = n + i
        inputs[idx].type = 1
        inputs[idx].ki.wVk = vk
        inputs[idx].ki.dwFlags = KEYEVENTF_KEYUP
    user32.SendInput(n * 2, ctypes.byref(inputs), ctypes.sizeof(INPUT))


def is_teams_process_running() -> bool:
    """Check if Microsoft Teams is running on the system."""
    for proc in psutil.process_iter(['name']):
        try:
            name = proc.info['name']
            if name and ('teams' in name.lower() or 'ms-teams' in name.lower()):
                return True
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return False


class TeamsClient:
    """Manages two-way Microsoft Teams state synchronization via WebSocket or Hotkey Fallback."""

    def __init__(self, token_path: str = None):
        if token_path is None:
            base_dir = os.path.dirname(os.path.abspath(__file__))
            token_path = os.path.join(base_dir, "teams_token.json")
        self.token_path = token_path
        self.token = self._load_token()

        self.ws = None
        self.is_connected = False
        self.is_in_meeting = False
        self.is_muted = True
        self.is_hand_raised = False
        self.request_id = 1
        self.state_changed = False

    def _load_token(self) -> str:
        if os.path.exists(self.token_path):
            try:
                with open(self.token_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    return data.get("token", "")
            except Exception:
                pass
        return ""

    def _save_token(self, token: str):
        self.token = token
        try:
            with open(self.token_path, "w", encoding="utf-8") as f:
                json.dump({"token": token}, f)
            print(f"[Teams] Saved pairing token to {self.token_path}")
        except Exception as e:
            print(f"[Teams] Failed to save token: {e}")

    async def run(self):
        """Continuously manages connection to the Teams Third-Party API."""
        try:
            import websockets
        except ImportError:
            print("[Teams] 'websockets' package not found. Running in Hotkey Fallback mode.")
            return

        while True:
            try:
                uri = "ws://127.0.0.1:8124?protocol-version=2.0.0&manufacturer=Guition&device=SmartKnob&app=Companion&app-version=1.0.0"
                if self.token:
                    uri += f"&token={self.token}"

                async with websockets.connect(uri, ping_interval=10, ping_timeout=5) as ws:
                    self.ws = ws
                    self.is_connected = True
                    print("[Teams] Connected to MS Teams Local Third-Party API (127.0.0.1:8124)!")

                    async for message in ws:
                        try:
                            msg = json.loads(message)
                            self._handle_teams_message(msg)
                        except Exception as e:
                            print(f"[Teams] Error parsing message: {e}")

            except (websockets.exceptions.ConnectionClosed, OSError):
                if self.is_connected:
                    print("[Teams] Disconnected from MS Teams API.")
                self.is_connected = False
                self.ws = None

            # Fallback: check if Teams is running when WebSocket is down
            if not self.is_connected:
                # Update meeting assumption based on process presence
                if not is_teams_process_running():
                    if self.is_in_meeting:
                        self.is_in_meeting = False
                        self.state_changed = True

            await asyncio.sleep(3.0)

    def _handle_teams_message(self, msg: dict):
        # 1. Pairing token response
        if "token" in msg:
            token = msg["token"]
            if token and token != self.token:
                print(f"[Teams] Received new pairing token: {token}")
                self._save_token(token)

        # 2. Meeting update
        if "meetingUpdate" in msg:
            update = msg["meetingUpdate"]
            state = update.get("meetingState", {})
            in_meeting = state.get("isInMeeting", False)
            muted = state.get("isMuted", True)
            hand = state.get("isHandRaised", False)

            if (in_meeting != self.is_in_meeting or
                muted != self.is_muted or
                hand != self.is_hand_raised):
                self.is_in_meeting = in_meeting
                self.is_muted = muted
                self.is_hand_raised = hand
                self.state_changed = True
                print(f"[Teams] Meeting: {'Active' if in_meeting else 'None'}, "
                      f"Mic: {'Muted' if muted else 'Live'}, Hand: {'Raised' if hand else 'Down'}")

    async def toggle_mute(self):
        """Toggle microphone mute via WebSocket or SendInput Ctrl+Shift+M."""
        if self.is_connected and self.ws:
            self.request_id += 1
            payload = {
                "action": "toggle-mute",
                "parameters": {},
                "requestId": self.request_id
            }
            try:
                await self.ws.send(json.dumps(payload))
                print("[Teams] Sent toggle-mute via WebSocket")
                return
            except Exception as e:
                print(f"[Teams] WebSocket send error: {e}")

        # Fallback: Windows global shortcut Ctrl+Shift+M
        print("[Teams] Fallback: Sending Ctrl+Shift+M keystroke")
        send_windows_hotkey([0x11, 0x10, 0x4D])  # VK_CONTROL, VK_SHIFT, VK_M
        self.is_muted = not self.is_muted
        self.state_changed = True

    async def toggle_hand(self):
        """Toggle raise/lower hand via WebSocket or SendInput Ctrl+Shift+K."""
        if self.is_connected and self.ws:
            self.request_id += 1
            payload = {
                "action": "toggle-hand",
                "parameters": {},
                "requestId": self.request_id
            }
            try:
                await self.ws.send(json.dumps(payload))
                print("[Teams] Sent toggle-hand via WebSocket")
                return
            except Exception as e:
                print(f"[Teams] WebSocket send error: {e}")

        # Fallback: Windows global shortcut Ctrl+Shift+K
        print("[Teams] Fallback: Sending Ctrl+Shift+K keystroke")
        send_windows_hotkey([0x11, 0x10, 0x4B])  # VK_CONTROL, VK_SHIFT, VK_K
        self.is_hand_raised = not self.is_hand_raised
        self.state_changed = True

    def get_packet(self) -> dict:
        """Generate JSON packet to sync with Guition ESP32 knob."""
        self.state_changed = False
        return {
            "type": "teams",
            "meeting": self.is_in_meeting,
            "muted": self.is_muted,
            "hand": self.is_hand_raised
        }
