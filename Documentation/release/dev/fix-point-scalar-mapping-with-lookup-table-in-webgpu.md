## Fix point scalar mapping with lookup table in WebGPU

VTK now correctly passes the parent mapper's input data object to child batched mappers in WebGPU rendering. This fixes a bug where scalar mapping to textures would fail because `GetExecutive()->GetInputDataObject(0,0)` returned `nullptr` in `vtkMapper::MapScalarsToTexture`.

You can now use lookup tables for point scalar mapping in WebGPU without encountering null pointer errors.
