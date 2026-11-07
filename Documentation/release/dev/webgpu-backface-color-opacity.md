## WebGPU backface materials and opaque occlusion

The WebGPU backend now uses an actor's backface material colors when rendering
the backs of triangles, including glyphs. Changes to the backface colors are
reflected in subsequent frames.

Transparent fragments behind opaque geometry are now excluded during compositing
and selection. The transparency resolve pass also refreshes its depth resources
when the render window is resized.
