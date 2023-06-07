## SPDX Generation in module

VTK now supports the Software Package Data Exchange (SPDX) standard for communicating
software bill of materials (SBOM) information. This standard allows for the accurate
identification of software components, explicit mapping of relationships between these
components, and the association of security and licensing information with each component.

See [](/advanced/spdx_and_sbom.md).

To support the standard, each VTK module may be described by a `.spdx` file. Configuring
VTK with the option `VTK_GENERATE_SPDX`, set to `ON` enables [](/api/cmake/ModuleSystem.md#spdx-files-generation)
for each VTK module.

Generated SPDX files are based on the SPDX 2.2 specification and are named after `<ModuleName>.spdx`.
The generation of SPDX files is considered experimental and both the VTK Module system API and
the `SPDXID` used in the generated files may change.

It's worth noting that there are some [limitations](/api/cmake/ModuleSystem.md#limitations) to
the generated SPDX files.

See [these examples](/advanced/spdx_and_sbom.md#examples) of generated files.
