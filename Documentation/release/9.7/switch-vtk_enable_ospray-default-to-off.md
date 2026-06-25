## Switch `VTK_ENABLE_OSPRAY` default value to `OFF`

The `VTK_ENABLE_OSPRAY` option is now switched to `OFF` by default. Therefore, this options requires to be activated manually when enabling `VTK_MODULE_ENABLE_VTK_RenderingRaytracing` as multiple backends are now available: `OSPRay` and `ANARI`.
