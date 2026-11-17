# WebGPU background and backface textures

The WebGPU backend now renders mono and stereo background textures and correctly
composites textures with premultiplied alpha. It also honors
`vtkProperty::ShowTexturesOnBackface`, allowing textures to be hidden on back-facing
surfaces while preserving their material colors. Texture input and property changes
take effect after the first render.
