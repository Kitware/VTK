# Object manager

## Serialization
You can register objects with a `vtkObjectManager` instance and call
`UpdateStatesFromObjects`, `GetState(identifier)` to obtain a serialized state of
the registered objects and all their dependency objects that are
serializable.

## Deserialization
You can register a json state (stringified) with a `vtkObjectManager` instance
and call `UpdateObjectsFromStates`, `GetObjectAtId(identifier)` to deserialize and
retrieve the objects.

## Blobs
All `vtkDataArray` are hashed and stored as unique blobs to prevent
multiple copies of the same data within the state. The contents of a data array
within a state are represented with a hash string. You can fetch and register
blobs using `GetBlob` and `RegisterBlob`.

## Dependencies
You can retrieve all dependent object identifiers using
`vtkObjectManager::GetAllDependencies(identifier)`

## Extensions
You can also (de)serialize classes in custom VTK modules. Simply pass the registrar
function to `vtkObjectManager::InitializeExtensionModuleHandler(registrar)` where
the `registrar` argument is of type `vtkSessionObjectManagerRegistrarFunc`. In C/C++,
this function can be accessed from the `vtkModuleNameSerDes.h` header file in your build directory.
In Python, this function is available as an attribute on the module.

Here are examples:

```cpp
#include <vtkFooSerDes.h>

vtkNew<vtkObjectManager> manager;
manager->InitializeExtensionModuleHandler(RegisterClasses_vtkFoo);
```

```py
from vtkmodules import vtkInteractionWidgets
from vtkmodules.vtkSerializationManager import vtkObjectManager
object_manager = vtkObjectManager()
object_manager.InitializeExtensionModuleHandler(vtkInteractionWidgets.RegisterClasses_vtkInteractionWidgets)
```

### Serialization of addon in WASM

In WASM, you will need to create two directories that resemble this tree. In this illustration, we wish to
(de)serialize classes from the `Bar` and `Foo` modules in WASM using `vtkRemoteSession` or `vtkStandaloneSession`.

The `WebAssemblyAsync` module is optional. You can leave it out if you are not interested in having WebGPU support.


```sh
- Bar
 |- ...
 |- vtk.module
- Foo
 |- ...
 |- vtk.module
- WebAssembly
 |- vtk.module
 |- CMakeLists.txt
- WebAssemblyAsync
 |- vtk.module
 |- CMakeLists.txt
```

Declare your dependencies in `WebAssembly/vtk.module`

```sh
NAME
  FooBar::WebAssembly
LIBRARY_NAME
  fooBarWebAssembly
PRIVATE_DEPENDS
  VTK::SerializationManager
  VTK::WebAssemblySession
OPTIONAL_DEPENDS
  FooBar::Foo
  FooBar::Bar
  VTK::RenderingContextOpenGL2
  VTK::RenderingOpenGL2
  VTK::RenderingUI
  VTK::RenderingVolumeOpenGL2
```

Build the module in the `WebAssembly/CMakeLists.txt`

```cmake
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(FATAL_ERROR
    "The VTK::WebAssembly module requires Emscripten compiler.")
endif ()

vtk_module_add_serdes_wasm_package(
  MODULE FooBar::WebAssembly
  OUTPUT_NAME fooBarWebAssembly
)
```

Finally, ensure the `WebAssembly/vtk.module` is passed to `vtk_module_build` in your project's root `CMakeLists.txt`

If you are interested in WebGPU support, declare your dependencies in `WebAssemblyAsync/vtk.module` and repeat similar steps
as in the non-async section using the new module names.

```sh
NAME
  FooBar::WebAssemblyAsync
LIBRARY_NAME
  fooBarWebAssemblyAsync
PRIVATE_DEPENDS
  VTK::SerializationManager
  VTK::WebAssemblySession
OPTIONAL_DEPENDS
  FooBar::Foo
  FooBar::Bar
  VTK::RenderingContextOpenGL2
  VTK::RenderingOpenGL2
  VTK::RenderingUI
  VTK::RenderingVolumeOpenGL2
  VTK::RenderingWebGPU
```
