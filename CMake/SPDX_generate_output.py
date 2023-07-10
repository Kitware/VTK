"""Generate a SPDX file for a VTK module

This is a python script to generate a SPDX file for a VTK module
following 2.2 specification.

See https://spdx.github.io/spdx-spec/v2.2.2/.
"""

import argparse

from datetime import datetime, timezone
from pathlib import Path
from typing import List


EPILOG = """

VTK module system integration
-----------------------------

See ``vtkModule.cmake`` for usage inside VTK module system.

List of sources may also be specified using a response file where
each line corresponds to a different source file.

Example of standalone usage
---------------------------

::

    python SPDX_generate_output.py \\
        -m ModuleName \\
        -n https://mynamespace.org \\
        -d https://github.com/Awesome/VTKModule \\
        -l LicenseID \\
        -c "Copyright Text" \\
        -o output.spdx \\
        -s /path/to/sources/dir \\
        source1.cxx source2.cxx source1.h source2.h
"""


def generate_spdx_file(
    module_name: str,
    namespace: str,
    download_location: str,
    declared_license: str,
    module_copyright: str,
    custom_license_file: str,
    custom_license_name: str,
    output_file: str,
    source_dir: str,
    input_files: List[str],
):
    """Generate a SPDX file for a VTK module.

    :param module_name: VTK module name.
    :param namespace: SPDX Document namespace.
    :param download_location: VTK module download location.
    :param declared_license: VTK module declared license identifier.
    :param module_copyright: VTK module copyright.
    :param custom_license_file: A custom license file to include.
    :param custom_license_name: SPDX name of the custom license to include.
    :param output_file: SPDX output file.
    :param source_dir: Directory containing source files.
    :param input_files: List of source files.
    """
    spdx_cpy_expr = "// SPDX-FileCopyrightText: "
    spdx_lic_expr = "// SPDX-License-Identifier: "
    copyrights = set()
    licenses = set()
    files_analyzed = bool(input_files)

    # Check input files exists
    for filename in input_files:
        path = Path(filename)
        if not path.exists():
            path = source_dir / path
            if not path.exists():
                # A non existing file will be generated on build, ignore it
                continue

        if path.stem == f"{module_name}Module":
            # Skip module file, eg: vtkIOPLYModule.h
            continue

        # Parse copyrights and licenses in all files
        read_cpy = True
        found_cpy = False
        for line in path.open("r"):
            if read_cpy:
                index = line.find(spdx_cpy_expr)
                if index == 0:
                    copyrights.add(line[len(spdx_cpy_expr) : -1])
                    found_cpy = True
                elif not found_cpy:
                    print(
                        "Warning: File %s does not contain expected SPDX copyright text but contains instead: %s"
                        % (filename, line)
                    )
                    break
                else:
                    read_cpy = False

            # This is not an else
            if not read_cpy:
                index = line.find(spdx_lic_expr)
                if index == 0:
                    licenses.add(line[len(spdx_lic_expr) : -1])
                    break
                else:
                    print(
                        "Warning: File %s does not contain expected SPDX license id but contains instead: %s"
                        % (filename, line)
                    )
                    break

    # Create a string from the license from files
    licenses.add(declared_license)  # Add declared license for simplicity sake
    licenses_from_files = ""
    if len(licenses) > 1:
        for lic in licenses:
            licenses_from_files += "PackageLicenseInfoFromFiles: %s\n" % lic

    # Create a string from the copyrights from files
    copyrights.add(module_copyright)
    copyrights_from_files = "PackageCopyrightText: <text>\n"
    for cpy in copyrights:
        copyrights_from_files += "%s\n" % cpy
    copyrights_from_files += "</text>"

    # Conclude on license
    concluded_license = ""
    for lic in licenses:
        concluded_license += "%s AND " % lic
    concluded_license = concluded_license[: concluded_license.rfind(" AND ")]

    # Recover timestamp
    timestamp = datetime.now(timezone.utc)
    timestamp = timestamp.strftime("%Y-%m-%dT%H:%M:%SZ")

    custom_license_string = ""
    if bool(custom_license_file) ^ bool(custom_license_name):
      print(
        "Warning: custom_license_file (\"%s\") and custom_license_name (\"%s\") should both be defined or both not defined"
        % (custom_license_file, custom_license_name)
        )
    elif custom_license_file:
      with open(source_dir / Path(custom_license_file), "r") as file:
        custom_license = file.read()
      custom_license_string = """

LicenseID: LicenseRef-%(custom_license_name)s
ExtractedText: <text>%(custom_license)s</text>""" % { "custom_license_name": custom_license_name , "custom_license": custom_license }

    spdx_string = """SPDXVersion: SPDX-2.2
DataLicense: CC0-1.0
SPDXID: SPDXRef-DOCUMENT
DocumentName: %(module)s
DocumentNamespace: %(namespace)s
Creator: Tool: CMake
Created: %(timestamp)s

##### Package: %(module)s

PackageName: %(module)s
SPDXID: SPDXRef-Package-%(module)s
PackageDownloadLocation: %(download)s
FilesAnalyzed: %(files_analyzed)s
PackageLicenseConcluded: %(lic_concluded)s
PackageLicenseDeclared: %(lic_declared)s
%(lic_from_files)s%(cpy_from_files)s%(custom_license_string)s

Relationship: SPDXRef-DOCUMENT DESCRIBES SPDXRef-Package-%(module)s
"""

    with open(output_file, "w+") as output:
        output.writelines(
            spdx_string
            % {
                "module": module_name,
                "namespace": namespace,
                "timestamp": timestamp,
                "download": download_location,
                "files_analyzed": files_analyzed,
                "lic_concluded": concluded_license,
                "lic_declared": args.license,
                "lic_from_files": licenses_from_files,
                "cpy_from_files": copyrights_from_files,
                "custom_license_string": custom_license_string,
            }
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__ + "\nArguments\n---------\n",
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        fromfile_prefix_chars="@",
    )
    parser.add_argument("-m", "--module", help="VTK module name", required=True)
    parser.add_argument("-n", "--namespace", help="Document namespace", required=True)
    parser.add_argument("-d", "--download", help="VTK module download location", required=True)
    parser.add_argument("-l", "--license", help="VTK module declared license identifier", required=True)
    parser.add_argument("-c", "--copyright", help="VTK module copyright", required=True)
    parser.add_argument("-x", "--custom_license_file", help="VTK module custom license file", required=True)
    parser.add_argument("-y", "--custom_license_name", help="VTK module custom license name", required=True)
    parser.add_argument("-o", "--output", help="SPDX output file", required=True)
    parser.add_argument("-s", "--source", help="Directory containing source files", required=True)
    parser.add_argument("input_files", metavar="N", nargs="*")
    args = parser.parse_args()

    generate_spdx_file(
        args.module,
        args.namespace,
        args.download,
        args.license,
        args.copyright,
        args.custom_license_file,
        args.custom_license_name,
        args.output,
        args.source,
        args.input_files,
    )
