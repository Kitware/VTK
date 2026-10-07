# WebGPU: WGSL compilation messages with source lines

Errors and warnings reported by the WebGPU implementation while compiling a WGSL shader are now
logged with their line and column, followed by the offending line and its neighbors from the shader
source, which makes errors in generated shaders easier to locate.
