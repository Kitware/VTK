## Reorder OpenGL shader replacement

Location of `//VTK::Normal::Impl` as been moved higher in the shader in order to allow user to customize the color based on the normal value.
In `ReplaceShaderValues` function, `ReplaceShaderNormal` is now called before `ReplaceShaderColor`.
