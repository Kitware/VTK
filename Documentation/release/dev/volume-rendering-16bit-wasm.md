## Support 16-bit Textures on GLES 3.0 and WebAssembly

VTK now provides comprehensive support for 16-bit textures (both signed and
unsigned) on GLES 3.0 (including WebAssembly/Emscripten) platforms, addressing
the limitation of missing normalized 16-bit formats in the OpenGL ES core
specification.

### Runtime Detection of GL_EXT_texture_norm16

`vtkOpenGLState` now detects the availability of the `GL_EXT_texture_norm16`
extension at runtime. When the extension is present, normalized 16-bit formats
(`GL_R16` for unsigned and signed variants) are used natively. When absent, VTK
automatically falls back to optimized conversion strategies.

### GPU-Assisted Conversion with Multiple Strategies

A new `vtkOpenGLTextureNormalizationHelper` class provides three conversion
strategies for platforms without native 16-bit support:

- **CPU Conversion**: Direct CPU-based normalization of 16-bit data to 32-bit
float before upload, useful for debugging and smaller datasets.
- **Compute Shader Conversion**: High-performance GPU-based conversion using
compute shaders for efficient parallel processing of large datasets.
- **Framebuffer-based Conversion**: Alternative GPU conversion using
framebuffer objects and rendering techniques, providing compatibility across
different hardware configurations.

The system automatically selects the most efficient strategy available on the
target platform.

### Zero-Copy and Performance Optimization

The implementation prioritizes zero-copy transfer of texture data whenever
possible. For 16-bit data on GLES 3.0 without `GL_EXT_texture_norm16`, the
conversion is delegated to the GPU when available, minimizing CPU involvement
and data movement. This significantly improves performance when loading large
16-bit texture datasets.

### Support for Both Unsigned and Signed 16-bit Data

- **VTK_UNSIGNED_SHORT**: Falls back to `GL_R32F` with values normalized to [0,
1] when native support is unavailable.
- **VTK_SHORT**: Extends the framework to handle signed 16-bit data,
normalizing values to [-1, 1] using `GL_R32F` on platforms without extension
support.

### Platform Coverage

This enhancement enables proper 16-bit texture handling across:

- **GLES 3.0**: Modern mobile and embedded systems
- **WebAssembly (Emscripten)**: Web-based VTK applications
- **Legacy Systems**: Fallback to CPU conversion ensures compatibility even on
older hardware

The `Create1DFromRaw`, `Create2DFromRaw`, `Create2DArrayFromRaw`, and
`Create3DFromRaw` methods in `vtkTextureObject` now transparently handle 16-bit
source data with automatic GPU-first conversion and CPU fallback strategies.
