#!/usr/bin/env python3
"""
main.py — End-to-end XML to Python code generation

This is the main entry point for the IRON code generation system.
It takes an XML file as input and generates Python code by:
0. Expanding GUI-friendly XML to complete XML (XMLGenerator) if needed
1. Converting XML to semantic graph (GraphDriver)
2. Generating Python code from graph (CodeGenerator)
3. Optionally executes the generated code (with --run flag)

Usage:
    python main.py <xml_file> [--run]

Examples:
    python main.py examples/passthrough2/passthrough_gui.xml      # Generate from GUI XML
    python main.py examples/passthrough/passthrough.xml           # Generate from complete XML
    python main.py examples/passthrough/passthrough.xml --run     # Generate and execute code

Output:
    - <xml_dir>/<name>_complete.xml: Complete XML (if GUI XML was input)
    - <xml_dir>/<name>.graphml: Semantic graph representation
    - <xml_dir>/generated_<name>.py: Generated Python code
"""

import sys
import subprocess
from pathlib import Path

project_root = Path(__file__).resolve().parent
sys.path.insert(0, str(project_root))

from codegen.backends.CodeGenerator import CodeGenerator
from graph_builder.GraphDriver import GraphBuilder
from graph_builder.XMLGenerator import XMLTransformer


def _is_gui_xml(xml_path: Path) -> bool:
    """Detect if XML is GUI-friendly format by checking filename."""
    return '_gui' in xml_path.name


def main():
    """
    Main entry point for XML to Python code generation.

    Process:
    0. Detect if input is GUI XML and expand it to complete XML (XMLGenerator)
    1. Validate command-line arguments
    2. Build semantic graph from complete XML (GraphDriver)
    3. Generate Python code from graph (CodeGenerator)
    4. Optionally execute generated code
    5. Report results
    """
    # Parse arguments
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print(f"Usage: {sys.argv[0]} <xml_file> [--run]")
        print()
        print("Arguments:")
        print("  <xml_file>  Path to XML input file (GUI or complete)")
        print("  --run       Execute generated code after creation (optional)")
        print()
        print("Examples:")
        print(f"  {sys.argv[0]} examples/passthrough2/passthrough_gui.xml      # GUI XML workflow")
        print(f"  {sys.argv[0]} examples/passthrough/passthrough.xml           # Complete XML workflow")
        print(f"  {sys.argv[0]} examples/passthrough/passthrough.xml --run     # Generate and execute")
        print()
        print("Output files are created in the same directory as the input XML:")
        print("  - <name>_complete.xml (if GUI XML was used)")
        print("  - <name>.graphml (semantic graph)")
        print("  - generated_<name>.py (Python code)")
        sys.exit(1)

    xml_path = Path(sys.argv[1])
    run_after = len(sys.argv) == 3 and sys.argv[2] == '--run'

    # Validate --run flag if provided
    if len(sys.argv) == 3 and sys.argv[2] != '--run':
        print(f"Error: Unknown flag '{sys.argv[2]}'")
        print(f"Usage: {sys.argv[0]} <xml_file> [--run]")
        sys.exit(1)

    # Validate XML file exists
    if not xml_path.is_file():
        print(f"Error: File not found: {xml_path}")
        sys.exit(1)

    if not xml_path.suffix == '.xml':
        print(f"Warning: File does not have .xml extension: {xml_path}")

    print("=" * 70)
    print("IRON Code Generation System")
    print("=" * 70)
    print()

    # Step 0: Detect and expand GUI XML if needed
    working_xml_path = xml_path
    if _is_gui_xml(xml_path):
        print(f"[0/3] Detected GUI XML - Expanding to complete XML...")
        print(f"      Input: {xml_path}")

        try:
            # Generate complete XML path
            complete_xml_path = xml_path.parent / xml_path.name.replace('_gui.xml', '_complete.xml')
            if complete_xml_path == xml_path:
                # If name doesn't have _gui, add _complete before .xml
                complete_xml_path = xml_path.with_suffix('') / f"{xml_path.stem}_complete.xml"
                complete_xml_path = xml_path.parent / f"{xml_path.stem}_complete.xml"

            # Transform GUI XML to complete XML
            transformer = XMLTransformer(xml_path)
            transformer.save(complete_xml_path)

            working_xml_path = complete_xml_path
            print(f"      Output: {complete_xml_path}")
            print()

        except Exception as e:
            print(f"      ERROR: Failed to expand GUI XML")
            print(f"      {type(e).__name__}: {e}")
            sys.exit(1)

    # Step 1: Build semantic graph from XML
    print(f"[1/3] Building semantic graph from XML...")
    print(f"      Input: {working_xml_path}")

    try:
        builder = GraphBuilder(working_xml_path)
        graph = builder.build()
        
        # Save graph to GraphML in same directory as XML
        # Strip _gui suffix if present for output filenames
        base_name = xml_path.stem.replace('_gui', '')
        graphml_path = xml_path.parent / f"{base_name}.graphml"
        import networkx as nx
        nx.write_graphml(graph, graphml_path)
        
        print(f"      Output: {graphml_path}")
        print(f"      Graph: {graph.number_of_nodes()} nodes, {graph.number_of_edges()} edges")
        
        # Show node type distribution
        node_kinds = {}
        for _, data in graph.nodes(data=True):
            kind = data.get('kind', '?')
            node_kinds[kind] = node_kinds.get(kind, 0) + 1
        
        print(f"      Node types: {len(node_kinds)} unique types")
        print()
        
    except Exception as e:
        print(f"      ERROR: Failed to build graph")
        print(f"      {type(e).__name__}: {e}")
        sys.exit(1)
    
    # Step 2: Generate Python code from graph
    print(f"[2/3] Generating Python code from graph...")
    print(f"      Input: {graphml_path}")
    
    try:
        generator = CodeGenerator(graphml_path)
        code = generator.generate()
        
        # Save generated code in same directory as XML
        # Strip _gui suffix if present for output filenames
        base_name = xml_path.stem.replace('_gui', '')
        output_path = xml_path.parent / f"generated_{base_name}.py"
        
        with open(output_path, 'w') as f:
            f.write(code)
        
        line_count = len(code.splitlines())
        print(f"      Output: {output_path}")
        print(f"      Generated: {line_count} lines of Python code")
        print()
        
    except Exception as e:
        print(f"      ERROR: Failed to generate code")
        print(f"      {type(e).__name__}: {e}")
        sys.exit(1)
    
    # Success summary
    print("=" * 70)
    print("[SUCCESS] Code generation completed successfully!")
    print("=" * 70)
    print()
    print("Generated files:")
    print(f"  1. {graphml_path}")
    print(f"     Semantic graph (for debugging/visualization)")
    print(f"  2. {output_path}")
    print(f"     Python code (ready to run)")
    print()
    
    # Execute generated code if --run flag provided
    if run_after:
        print("=" * 70)
        print("[3/3] Executing generated code...")
        print("=" * 70)
        print()
        
        try:
            # Run the generated Python file
            result = subprocess.run(
                [sys.executable, str(output_path)],
                capture_output=True,
                text=True,
                timeout=30  # 30 second timeout
            )
            
            # Display output
            if result.stdout:
                print("Output:")
                print(result.stdout)
            
            if result.stderr:
                print("Errors/Warnings:")
                print(result.stderr)
            
            if result.returncode == 0:
                print()
                print("=" * 70)
                print("[SUCCESS] Code executed successfully!")
                print("=" * 70)
            else:
                print()
                print("=" * 70)
                print(f"[ERROR] Code execution failed with exit code {result.returncode}")
                print("=" * 70)
                sys.exit(result.returncode)
                
        except subprocess.TimeoutExpired:
            print()
            print("=" * 70)
            print("[ERROR] Code execution timed out (30 seconds)")
            print("=" * 70)
            sys.exit(1)
        except Exception as e:
            print()
            print("=" * 70)
            print(f"[ERROR] Failed to execute code: {type(e).__name__}: {e}")
            print("=" * 70)
            sys.exit(1)
    else:
        print("Next steps:")
        print(f"  - Review the generated code")
        print(f"  - Run: python {output_path}")
        print(f"  - Or use: python {sys.argv[0]} {xml_path} --run")
        print(f"  - Visualize graph with yEd or similar tool")
    
    print()


if __name__ == "__main__":
    main()
