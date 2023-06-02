## Add ABI Namespace around VTK

VTK is now wrapped in a customizable `inline namespace`. You can now
link different versions of VTK into the same runtime without generating
conflicts between VTK symbols.

A limitation to this change is it does _NOT_ prevent conflicts between
third-party symbols, including VTK-m. There is no enforcement in CMake
for third-party dependencies needing to be external when using the ABI
namespace, this is left to you to configure. A full known list of known
issues is listed below.

Where to put namespaces
* Around classes, functions, variables, typedefs (optional).
* Inner most named namespaces, there is no need to use the ABI namespace inside of an anonymous namespace.
  * ABI namespace should never be around a named namespace.
* Forward declarations of classes/functions/variables/typedefs require ABI namespace if their implementation/declarion was inside the ABI namespace.

Where not to put namespace
* Do not namespace around non-exported classes/functions/variables/typedefs (usually found in tests).
* Do not namespace around main functions.
* Python bindings cannot be namespaced.
* Most Utilities are not namespaced, including vtksys/vtkmeta/ksys.
* It doesn't hurt anything, but it is not required to namespace symbols that are compiled into a driver (such as Wrapping Tools).

A list of modules with symbol that do not use the ABI namespace:

Some VTK modules have C interfaces that cannot be mangled.
  * VTK::CommonCore (GetVTKVersion)
  * VTK::IOXML (Provides a C API, `vtkXMLWriterC_*`)
  * VTK::WrappingPythonCore (Python Wrapping cannot have mangling)

Thirdpary Libraries and their VTK module wrappers do not have mangling
  * VTK::metaio
  * VTK::xdmf2
  * VTK::vpic
  * All C libraries (ie. HDF5, netCDF, etc.)

VTKm CUDA Accelerators do not get mangled
  * VTK::AcceleratorsVTKmCore
  * VTK::AcceleratorsVTKmDataModel
  * VTK::AcceleratorsVTKmFilters
