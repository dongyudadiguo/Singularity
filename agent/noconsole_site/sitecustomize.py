"""Force Windows child processes to start without a console window.

Loaded automatically via site.sitecustomize when this directory is on PYTHONPATH.
Keeps tool calls from flashing a terminal and stealing focus from the user.
"""
from __future__ import annotations

import os
import subprocess
import sys

if os.name == "nt" and not getattr(subprocess, "_ae_noconsole_patched", False):
    _CREATE_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0x08000000)
    _STARTF_USESHOWWINDOW = getattr(subprocess, "STARTF_USESHOWWINDOW", 0x00000001)
    _SW_HIDE = 0

    def _startupinfo(existing=None):
        try:
            si = existing or subprocess.STARTUPINFO()
        except Exception:
            return existing
        try:
            si.dwFlags |= _STARTF_USESHOWWINDOW
            # Only force hide when caller did not set a show window value.
            if getattr(si, "wShowWindow", 0) in (0, _SW_HIDE):
                si.wShowWindow = _SW_HIDE
        except Exception:
            pass
        return si

    def _silence_kwargs(kwargs: dict) -> dict:
        kwargs = dict(kwargs)
        # Avoid double-console creation flags when caller already asked for none.
        flags = kwargs.get("creationflags", 0) or 0
        kwargs["creationflags"] = flags | _CREATE_NO_WINDOW
        if "startupinfo" in kwargs:
            kwargs["startupinfo"] = _startupinfo(kwargs.get("startupinfo"))
        else:
            kwargs["startupinfo"] = _startupinfo()
        return kwargs

    _orig_popen = subprocess.Popen
    _orig_run = subprocess.run
    _orig_call = subprocess.call
    _orig_check_call = subprocess.check_call
    _orig_check_output = subprocess.check_output

    class Popen(_orig_popen):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **_silence_kwargs(kwargs))

    def run(*args, **kwargs):
        return _orig_run(*args, **_silence_kwargs(kwargs))

    def call(*args, **kwargs):
        return _orig_call(*args, **_silence_kwargs(kwargs))

    def check_call(*args, **kwargs):
        return _orig_check_call(*args, **_silence_kwargs(kwargs))

    def check_output(*args, **kwargs):
        return _orig_check_output(*args, **_silence_kwargs(kwargs))

    subprocess.Popen = Popen
    subprocess.run = run
    subprocess.call = call
    subprocess.check_call = check_call
    subprocess.check_output = check_output
    subprocess._ae_noconsole_patched = True

    # os.system / os.spawn* still flash consoles; route system through hidden subprocess.
    import os as _os

    _orig_system = _os.system

    def _system(command):
        try:
            completed = run(command, shell=True)
            return completed.returncode
        except Exception:
            return _orig_system(command)

    _os.system = _system
