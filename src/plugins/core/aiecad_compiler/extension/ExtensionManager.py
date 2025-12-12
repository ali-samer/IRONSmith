#!/usr/bin/env python3
"""
ExtensionManager.py - Coordinates GraphExtender and CodeGeneratorExtender

This module manages the extension system that allows adding new XML node types
and code generation patterns without modifying the core GraphDriver and CodeGenerator.

Architecture:
- GraphExtender: Handles new XML node types -> graph conversion
- CodeGeneratorExtender: Handles new graph node types -> code generation
- ExtensionManager: Coordinates both and provides unified interface
"""

from typing import Dict, Type, Optional
from pathlib import Path


class ExtensionManager:
    """
    Manages extensions for both graph building and code generation.
    
    Provides a unified interface for registering and using extensions
    across the entire compilation pipeline.
    """
    
    def __init__(self):
        self.graph_extensions: Dict[str, Type] = {}
        self.codegen_extensions: Dict[str, Type] = {}
    
    def register_graph_extension(self, tag: str, extension_class: Type):
        """Register a GraphExtender extension for a specific XML tag."""
        self.graph_extensions[tag.lower()] = extension_class
    
    def register_codegen_extension(self, kind: str, extension_class: Type):
        """Register a CodeGeneratorExtender extension for a specific node kind."""
        self.codegen_extensions[kind] = extension_class
    
    def get_graph_extension(self, tag: str) -> Optional[Type]:
        """Get GraphExtender extension for a tag."""
        return self.graph_extensions.get(tag.lower())
    
    def get_codegen_extension(self, kind: str) -> Optional[Type]:
        """Get CodeGeneratorExtender extension for a node kind."""
        return self.codegen_extensions.get(kind)
    
    def apply_to_graph_builder(self, builder):
        """Apply all graph extensions to a GraphBuilder instance."""
        from extension.GraphExtender import register_extensions
        register_extensions(builder)
    
    def apply_to_code_generator(self, generator):
        """Apply all code generation extensions to a CodeGenerator instance."""
        from extension.CodeGeneratorExtender import register_codegen_extensions
        register_codegen_extensions(generator)


# Global extension manager instance
_extension_manager = ExtensionManager()


def get_extension_manager() -> ExtensionManager:
    """Get the global extension manager instance."""
    return _extension_manager
