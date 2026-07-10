## OpenGL ES HDRI texture formats

When baking the specular cubemap when using PBR with an environment texture,
VTK was always disabling mipmapping and the cubemap was forced to RGB8 format.
Now, it only disable mipmapping for RGB32F because mipmapping is indeed not supported.
Moreover, the specular cubemap format is now RGBA32F or RGBA16F depending on HalfPrecision instead of RGB8.
It allows support of HDR input texture.
