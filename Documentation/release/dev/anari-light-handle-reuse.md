## Reuse ANARI light handles and HDRI radiance

`vtkAnariLightNode` now reuses its ANARI light handle while the light subtype
remains unchanged. Camera movement and light parameter edits update that handle
in place, avoiding scene invalidation during light synchronization. Adding or
removing a light, or changing its subtype, still updates the scene structure.

HDRI radiance is cached until the environment texture or its image changes. This
avoids repeated image conversion and upload during camera movement. Changes to
light information keys and transform matrices are also tracked so these edits
reach the retained lights.
