# STRICT macro must not breaks libproj headers

Previously, VTK defined the `STRICT` for `windows.h` as empty if it was not defined. This breaks the `Proj` project.

The patch implements a workaround by `#define STRICT STRICT` to ensure that no codes breaks.

```cpp
#ifdef STRICT
#undef STRICT
#endif
#define STRICT STRICT
```
