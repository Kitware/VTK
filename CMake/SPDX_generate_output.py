"""Generate a SPDX file for a VTK module

This is a python script to generate a SPDX file for a VTK module
following 2.2 specification.

See https://spdx.github.io/spdx-spec/v2.2.2/.
"""

import argparse

from datetime import datetime, timezone
from pathlib import Path


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

def __main__():
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
    parser.add_argument("-o", "--output", help="SPDX output file", required=True)
    parser.add_argument("-s", "--source", help="Directory containing source files", required=True)
    parser.add_argument("inFiles", metavar="N", nargs="*")
    args = parser.parse_args()

    spdx_cpy_expr = "// SPDX-FileCopyrightText: "
    spdx_lic_expr = "// SPDX-License-Identifier: "
    copyrights = set()
    licenses = set()
    files_analyzed = bool(args.inFiles)

    # Check input files exists
    for filename in args.inFiles:
        path = Path(filename)
        if not path.exists():
            path = args.source / path
            if not path.exists():
                # A non existing file will be generated on build, ignore it
                continue

        if path.stem == args.module + "Module":
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
    licenses.add(args.license)  # Add declared license for simplicity sake
    licenses_from_files = ""
    if len(licenses) > 1:
        for lic in licenses:
            licenses_from_files += "PackageLicenseInfoFromFiles: %s\n" % lic

    # Create a string from the copyrights from files
    copyrights.add(args.copyright)
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
%(lic_from_files)s%(cpy_from_files)s

Relationship: SPDXRef-DOCUMENT DESCRIBES SPDXRef-Package-%(module)s
"""

    with open(args.output, "w+") as output:
        output.writelines(
            spdx_string
            % {
                "module": args.module,
                "namespace": args.namespace,
                "timestamp": timestamp,
                "download": args.download,
                "files_analyzed": files_analyzed,
                "lic_concluded": concluded_license,
                "lic_declared": args.license,
                "lic_from_files": licenses_from_files,
                "cpy_from_files": copyrights_from_files,
            }
        )


if __name__ == "__main__":
    __main__()
