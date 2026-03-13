"""
Extracts Magic IDs from nvapi64.lib by dumping it to assembly and parsing the output.
"""

import subprocess
import shutil
import re
from pathlib import Path

NVAPI_LIB_PATH = "../../../external/nvapi/amd64/nvapi64.lib"
NVAPI_HEADER_PATH = "../../../source/hooks/hook/nvapi_ids.h"

def extract_nvapi_magic_ids(lib_path: str | Path, output_file: str | Path = "nvapi.txt"):
    """
    Dumps nvapi64.lib to assembly and extracts the Magic IDs for all functions.
    """
    if not shutil.which("dumpbin.exe"):
        print("Error: dumpbin.exe not found! Please run this from a Visual Studio Developer Command Prompt.")
        return

    lib_path = Path(lib_path).resolve()
    log_path = Path(output_file).resolve()
    header_path = Path(NVAPI_HEADER_PATH).resolve()
    asm_path = log_path.with_suffix('.asm')

    if not lib_path.exists():
        print(f"Error: Library file not found at {lib_path}!")
        return

    # Dump the .lib to an assembly file
    print(f"Dumping '{lib_path.name}' to assembly...")
    try:
        with open(asm_path, "w", encoding="utf-8", errors="ignore") as asm_file:
            subprocess.run(
                ["dumpbin.exe", "/DISASM", str(lib_path)],
                stdout=asm_file,
                check=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
    except subprocess.CalledProcessError as e:
        print(f"Error running dumpbin: {e}!")
        return

    print("Parsing assembly for Magic IDs...")

    # Pre-fill the core static functions, heir IDs do not appear
    # in the standard thunk format inside the .asm dump
    extracted_functions = {
        "NvAPI_Initialize": "0x0150E828",
        "NvAPI_Unload": "0xD22BDD7E",
        "NvAPI_GetErrorMessage": "0x6C2D048C",
        "NvAPI_GetInterfaceVersionString": "0x01053FA5"
    }

    # Parse the .asm file
    id_pattern = re.compile(r"(?:ecx|edx|r\d+d),\s*([0-9A-Fa-f]+)h", re.IGNORECASE)

    with open(asm_path, "r", encoding="utf-8", errors="ignore") as f_in:
        current_func = None

        for line in f_in:
            line = line.strip()

            if not line:
                current_func = None
                continue

            # Detect the start of an NvAPI function label
            if line.endswith(":") and line.startswith("NvAPI_"):
                current_func = line.rstrip(":")
                continue

            if current_func and current_func not in extracted_functions:
                match = id_pattern.search(line)
                if match:
                    magic_id = match.group(1).upper()
                    extracted_functions[current_func] = f"0x{magic_id}"
                    current_func = None

    # Write the final sorted list to the text file
    with open(log_path, "w", encoding="utf-8") as f_out:
        for func_name, magic_id in sorted(extracted_functions.items()):
            f_out.write(f"{func_name} = {magic_id},\n")

    # Write the final sorted list to a C++ header file
    print(f"Generating C++ header '{header_path.name}'...")
    with open(header_path, "w", encoding="utf-8") as f_out:
        f_out.write("// ============================================================================\n\n")
        f_out.write("/// @brief NVAPI function Magic IDs\n\n")
        f_out.write("#pragma once\n\n")
        f_out.write("#include <nvapi/nvapi.h>\n\n")
        f_out.write("enum class NvAPI_pfn_ID : NvU32 {\n")
        for func_name, magic_id in sorted(extracted_functions.items()):
            f_out.write(f"    {func_name} = {magic_id},\n")
        f_out.write("};\n\n")
        f_out.write("// ----------------------------------------------------------------------------")

    print(f"Extraction complete! Found {len(extracted_functions)} functions.")

    if asm_path.exists():
        asm_path.unlink()

if __name__ == "__main__":
    extract_nvapi_magic_ids(NVAPI_LIB_PATH)
