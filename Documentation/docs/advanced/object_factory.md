# Object factory

## Introduction

`vtkObjectFactory` is a class in `VTK::CommonCore` module that automates the instantiation of VTK objects. This allows you to write VTK scripts with the same object name and having API specific classes being instantiated depending on the available classes. The instantiation occurs when calling the `::New()` static variable on any class deriving from `vtkObject`. Therefore, `vtkNew<>` or `vtkSmartPointer<>` also follow the same logic.

For example, after configuration, the following two lines will provide the same object:

```c++
// Creates the concrete class directly
vtkNew<vtkOpenGLRenderer> renderer1;

// Also a vtkOpenGLRenderer
vtkNew<vtkRenderer> renderer2;
```

## Requirements

CMake targets that use the VTK factory have to ensure that:
- The module providing the override is linked
- That `vtk_module_autoinit` is called on the target

In VTK Python, the module providing the implementation needs to be imported as well.

## Add factory support to new VTK classes

Adding support of API specific classes to the factory can be achieved in two steps. The first one is to add C++ code to configure the VTK class. Then some calls to CMake functions need to be done.

### Configure the VTK class in C++

- Make sure to use the `vtkStandardNewMacro` if you want to override a base class. Note that `vtkFactoryNewMacro` is the macro to use to make an overridable class.
- Create override attributes in the VTK class: [override attributes documentation](https://vtk.org/doc/nightly/html/classvtkOverrideAttribute.html#details)

### Configure the VTK class in CMake

- CMake override declaration: [](/advanced/migration_guides/ModuleMigration.md#object-factories)
- Module autoinit: [](/api/cmake/ModuleSystem.md#autoinit) and [](/api/cmake/ModuleSystem.md#module-metadata)
