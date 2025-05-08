# Patch state that refers to OSMesa OpenGL window into a WASM OpenGL window class.

The `vtkWasmSceneManager` now automatically patches the state provided to the `RegisterState` and
`UpdateObjectFromState` methods to replace 'ClassName' entry of `vtkOSOpenGLRenderWindow` with a
value of `vtkWebAssemblyOpenGLRenderWindow`.
