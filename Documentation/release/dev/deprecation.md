# New deprecation mechanism

The old VTK deprecation mechanism, `VTK_LEGACY_REMOVE` is being deprecated.
Instead or removing outdated APIs based on a build flag, users may choose to
instead have them warn or not by setting `VTK_DEPRECATION_LEVEL` to an
appropriate value. Valid values may be created using the `VTK_VERSION_CHECK`
macro from `vtkVersion.h`:

```c++
#define VTK_DEPRECATION_LEVEL VTK_VERSION_CHECK(9, 0, 0)
```

If not set, it is set to the last unsupported release number (for instance, it
is 8.2.0 today because 9.0.0 is the last supported release).

## For developers

```c++
#include "vtkLegacy.h"

VTK_LEGACY(void someOldMethod());

// in the source

#ifndef VTK_LEGACY_REMOVE
void someOldMethod() {
  VTK_LEGACY_BODY(someOldMethod, "VTK next.version")
}
#endif
```

becomes:

```c++
#include "vtkDeprecation.h"

VTK_DEPRECATED_IN_9_0_0("a reason for the deprecation and/or guidance on porting")
  void someOldMethod();

// in the source

void someOldMethod() {
  VTK_LEGACY_BODY(someOldMethod, "VTK next.version")
}
```

There are still a few use cases for `VTK_LEGACY_REMOVE` to exist:

  - Deprecating macros
  - Changing return types for methods (which should no longer be done; find a
    new method name)
