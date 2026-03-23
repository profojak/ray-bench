
"""
Extracts VTable IDs of DirectX 12 interfaces by parsing C-style dxgi.h.
"""

import re
import os

DXGI_HEADER_PATH = "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared/"
DXGI_HEADERS = [ "dxgi.h", "dxgi1_2.h", "dxgi1_3.h", "dxgi1_4.h", "dxgi1_5.h", "dxgi1_6.h" ]
HEADER_OUTPUT_PATH = "../../../source/hooks/hook/dxgi_vtables.hpp"

def extract_vtable_indices(header_path):
    """
    Extracts VTable method names from a C-style header file.
    """
    all_vtables = {}
    new_vtables = {}

    # Regex to find C-style VTable structs like
    # 'typedef struct INameVtbl { ... } INameVtbl;'.
    struct_pattern = re.compile(r'typedef struct (\w+Vtbl)\s*\{(.*?)\}\s*\1;', re.DOTALL)

    # Regex to find function pointers inside the struct like
    # 'ReturnType ( STDMETHODCALLTYPE *MethodName )( Args );'.
    method_pattern = re.compile(r'\(\s*STDMETHODCALLTYPE\s*\*\s*(\w+)\s*\)')

    for header_name in DXGI_HEADERS:
        full_path = os.path.join(DXGI_HEADER_PATH, header_name)

        with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        for struct_match in struct_pattern.finditer(content):
            vtable_name = struct_match.group(1)
            struct_body = struct_match.group(2)
        
            interface_name = vtable_name.replace('Vtbl', '')
            full_methods = method_pattern.findall(struct_body)

            all_vtables[interface_name] = full_methods
        
            best_prefix_len = 0
            for potential_parent, parent_methods in all_vtables.items():
                if potential_parent == interface_name:
                    continue

                prefix_len = len(parent_methods)
                if prefix_len <= len(full_methods) and full_methods[:prefix_len] == parent_methods:
                    if prefix_len > best_prefix_len:
                        best_prefix_len = prefix_len

            interface_new_methods = []
            for i in range(best_prefix_len, len(full_methods)):
                interface_new_methods.append((i,full_methods[i]))
            new_vtables[interface_name] = interface_new_methods

    return new_vtables

def generate_cpp_header(vtables):
    """
    Generates a C++ header file with enums for VTable method indices.
    """
    with open(HEADER_OUTPUT_PATH, 'w', encoding='utf-8') as f_out:
        f_out.write("// ============================================================================\n\n")
        f_out.write("/// @brief DXGI VTable method indices\n\n")
        f_out.write("#pragma once\n\n")

        for interface, methods in vtables.items():
            f_out.write(f"enum class {interface}_VTable_ID : int {{\n")
            for index, method_name in methods:
                f_out.write(f"    {method_name} = {index},\n")
            f_out.write(f"}};\n\n")
        f_out.write("// ----------------------------------------------------------------------------")

if __name__ == "__main__":
    parsed_interfaces = extract_vtable_indices(DXGI_HEADER_PATH)
    generate_cpp_header(parsed_interfaces)