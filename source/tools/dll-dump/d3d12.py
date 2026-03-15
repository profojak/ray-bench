"""
Extracts VTable IDs of DirectX 12 interfaces by parsing C-style d3d12.h.
"""

import re

D3D12_HEADER_PATH = "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um/d3d12.h"
HEADER_OUTPUT_PATH = "../../../source/hooks/hook/d3d12_vtables.hpp"

def extract_vtable_indices(header_path):
    """
    Extracts VTable method names from a C-style header file.
    """
    with open(header_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Regex to find C-style VTable structs like
    # 'typedef struct INameVtbl { ... } INameVtbl;'.
    struct_pattern = re.compile(r'typedef struct (\w+Vtbl)\s*\{(.*?)\}\s*\1;', re.DOTALL)

    # Regex to find function pointers inside the struct like
    # 'ReturnType ( STDMETHODCALLTYPE *MethodName )( Args );'.
    method_pattern = re.compile(r'\(\s*STDMETHODCALLTYPE\s*\*\s*(\w+)\s*\)')

    vtables = {}
    for struct_match in struct_pattern.finditer(content):
        vtable_name = struct_match.group(1)
        struct_body = struct_match.group(2)
        
        interface_name = vtable_name.replace('Vtbl', '')
        methods = []
        
        for method_match in method_pattern.finditer(struct_body):
            method_name = method_match.group(1)
            if method_name not in methods:
                methods.append(method_name)
            
        if methods:
            vtables[interface_name] = methods

    return vtables

def generate_cpp_header(vtables):
    """
    Generates a C++ header file with enums for VTable method indices.
    """
    with open(HEADER_OUTPUT_PATH, 'w', encoding='utf-8') as f_out:
        f_out.write("// ============================================================================\n\n")
        f_out.write("/// @brief D3D12 VTable method indices\n\n")
        f_out.write("#pragma once\n\n")

        for interface, methods in vtables.items():
            f_out.write(f"enum class {interface}_VTable_ID : int {{\n")
            for index, method_name in enumerate(methods):
                f_out.write(f"    {method_name} = {index},\n")
            f_out.write(f"}};\n\n")
        f_out.write("// ----------------------------------------------------------------------------")

if __name__ == "__main__":
    parsed_interfaces = extract_vtable_indices(D3D12_HEADER_PATH)
    generate_cpp_header(parsed_interfaces)