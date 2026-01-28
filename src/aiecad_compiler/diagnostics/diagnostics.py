from __future__ import annotations
from dataclasses import dataclass, asdict
from datetime import datetime
from typing import Callable, Dict, Any, Optional, List
import json
import sys

from .codes import Code, Severity, codes as default_codes

def _format_human(evt: Dict[str, Any]) -> str:
    """Human-friendly single-line format."""
    loc = evt.get("loc")
    loc_str = f" @ {loc}" if loc else ""
    return f"[{evt['ts']}] {evt['severity']} {evt['code']}{loc_str}: {evt['message']}"

def _format_json(evt: Dict[str, Any]) -> str:
    return json.dumps(evt, ensure_ascii=False)


def console_sink(line: str, stream = sys.stderr) -> None:
    stream.write(line + "\n")
    stream.flush()

"""
To be set by aiecad-compiler frontend during initialization
"""
@dataclass
class DiagnosticsConfig:
    formatter: Callable[[Dict[str, Any]], str] = _format_human
    sink: Callable[[str], None] = console_sink
    attach_process_info: bool = True
    codes = default_codes


class Diagnostics:
    """Structured diagnostics with stable codes and pluggable sinks."""
    def __init__(self, config: Optional[DiagnosticsConfig] = None):
        self.config = config or DiagnosticsConfig()
        self.codes = self.config.codes  # expose as diag.codes

    # public API
    def info(self, code: Code, *, msg: Optional[str] = None, **kwargs):
        self._emit(code, Severity.INFO, msg, kwargs)

    def warn(self, code: Code, *, msg: Optional[str] = None, **kwargs):
        self._emit(code, Severity.WARN, msg, kwargs)

    def error(self, code: Code, *, msg: Optional[str] = None, **kwargs):
        self._emit(code, Severity.ERROR, msg, kwargs)

    # core emit
    def _emit(self, code: Code, level: Severity, msg: Optional[str], kv: Dict[str, Any]):
        try:
            message = msg if msg is not None else code.template.format(**kv)
        except KeyError as e:
            # if template placeholders missing, degrade gracefully
            missing = str(e).strip("'")
            message = f"{code.template} (missing: {missing})"

        evt = {
            "ts": datetime.utcnow().isoformat(timespec="milliseconds") + "Z",
            "code": code.id,
            "severity": level.name,
            "message": message,
        }

        # common structured fields
        for field in ("loc", "tag", "parent", "attr", "expected", "actual",
                      "symbol", "name", "op", "reason", "node", "extra"):
            if field in kv and kv[field] is not None:
                evt[field] = kv[field]

        if self.config.attach_process_info:
            try:
                import os
                evt["pid"] = os.getpid()
            except Exception:
                pass

        line = self.config.formatter(evt)
        self.config.sink(line)

    # helper function to create a child diagnostics with a different sink/formatter
    def with_config(self, **overrides) -> "Diagnostics":
        cfg = DiagnosticsConfig(
            formatter = overrides.get("formatter", self.config.formatter),
            sink      = overrides.get("sink", self.config.sink),
            attach_process_info = overrides.get("attach_process_info", self.config.attach_process_info),
        )
        return Diagnostics(cfg)