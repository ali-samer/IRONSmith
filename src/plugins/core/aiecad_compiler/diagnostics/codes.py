from __future__ import annotations
from dataclasses import dataclass
from enum import Enum, auto

class Severity(Enum):
    INFO = auto()
    WARN = auto()
    ERROR = auto()

@dataclass(frozen=True)      # immutable
class Code:
    id: str                  # e.g., "XML001"
    severity: Severity       # default severity
    template: str            # default message template (format kwargs allowed)

class Codes:
    # XML / Schema / Parsing
    NO_XML_HANDLER     = Code("XML001", Severity.WARN,
                              "No handler for XML element <{tag}>.")
    BAD_XML_PLACEMENT  = Code("XML002", Severity.ERROR,
                              "<{3", Severity.ERROR,
                              "<{tag}> missing required attribute '{attr}'.")
    BAD_ATTRIBUTE_TYPE = Code("XML004", Severity.ERROR,
                              "<{tag}> attribute '{attr}' must be {expected}, got {actual}.")
    MISSING_TEXT       = Code("XML005", Severity.ERROR,
                              "<{tag}> requires inner text value.")
    UNEXPECTED_CHILD   = Code("XML006", Severity.ERROR,
                              "Unexpected child <{child}> inside <{tag}>.")
    UNKNOWN_SYMBOL     = Code("XML007", Severity.ERROR,
                              "Unknown symbol '{symbol}' referenced in <{ttag}> not allowed under <{parent}>.")
    MISSING_ATTRIBUTE  = Code("XML00ag}>.")

    # Symbols / IR
    DUP_SYMBOL         = Code("SYM001", Severity.ERROR,
                              "Duplicate symbol '{name}'.")
    TYPE_MISMATCH      = Code("IR001",  Severity.ERROR,
                              "Type mismatch: expected {expected}, got {actual}.")
    UNSUPPORTED_OP     = Code("IR002",  Severity.ERROR,
                              "Unsupported operation '{op}' in expression.")

    # Graph / Builder
    GB_RULE_FAILED     = Code("GB001", Severity.ERROR,
                              "Graph rule failed for <{tag}>: {reason}.")
    GB_INVARIANT       = Code("GB002", Severity.ERROR,
                              "Graph invariant violated: {reason}.")

    # Codegen (reserved for later)
    CG_RULE_FAILED     = Code("CG001", Severity.ERROR,
                              "Codegen rule failed for node '{node}': {reason}.")

codes = Codes()