## Python finds DLLs in multi-config build trees

On Windows, `import vtkmodules` from a build tree made with a multi-config
generator (e.g., Visual Studio) now finds VTK's DLLs in `bin/<config>`.
