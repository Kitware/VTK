Deprecation Process
===================

This page documents how to deprecate an API and mark it as no longer necessary
for downstream consumers of VTK.

Deprecating classes and functions
----------------------------------------

Classes, functions, member functions and member variables may be deprecated using the deprecation macros.

```c++
#include "vtkDeprecation.h" // Include the macros.

// A deprecated class.
class VTK_DEPRECATED_IN_X_Y_Z("reason for deprecation") OPT_EXPORT_MACRO oldClass {
public:
  // A deprecated member function.
  VTK_DEPRECATED_IN_X_Y_Z("reason for deprecation")
  void oldMemberFunction();
};

// A deprecated function.
VTK_DEPRECATED_IN_X_Y_Z("reason for deprecation")
void oldFunction();
```

The `X_Y_Z` should be the newest macro available in the `vtkDeprecation.h`
header when the API is added.

Note that, unlike, the old `VTK_LEGACY_REMOVE` mechanism, the APIs are not
deleted. This does interfere with various kinds of deprecations.

  - *Changing the return type*: Don't do this. Use a new name for the
    function.
  - *Deprecating macros*: Use `VTK_LEGACY_REMOVE`. New macro APIs should be
    highly discouraged.

### Lifetime of deprecated APIs

Deprecated APIs should exist for at least one release with the deprecation
warning active. This gives consumers of VTK at least one cycle to notice the
deprecation and move off of it.

Upon branching for a release, `master` will soon after have all instances of
deprecated symbols removed.

### Avoiding warnings within VTK

VTK is providing the deprecated symbols and as such may still use them in tests
or implementations. Since these generate warnings when compiling VTK itself,
classes which define deprecated symbols must suppress them.

Sources which continue to use the deprecated macros should add a comment to the
top of the source file to hide deprecation warnings in CI.

```c++
// Hide VTK_DEPRECATED_IN_X_Y_Z() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0
```

If one already exists, please add another comment to it so that when deprecated
symbols are removed, it shows up in the search.

### Deprecation policy

Ideally, any part of the public API of VTK, installed via its public headers should be deprecated
instead of being removed.

When deprecating anything, it is critical to ensure that they keep operating as expected,
if not, it would not be a deprecation but a bug.

In that regards, here is the deprecation policy:
- public structs/defines/enums/functions: **must** be deprecated
- public member variables: Should not exist but if they do, **must** be deprecated
- public member functions: **must** be deprecated
- public virtual member functions: **must** be deprecated unless in force majeure case, which must be discussed with the community
- protected members variables: **Can just be removed** and mentioned in markdown release notes
- protected member functions: **Should be deprecated** but removal and mention in markdown can be acceptable in hard cases
- protected virtual member functions: **Can be deprecated** but removal and mention in markdown is also ok

Using `VTK_DEPRECATION_LEVEL`
-----------------------------

When using VTK, the `VTK_DEPRECATION_LEVEL` macro may be set to a version
number. APIs which have been deprecated after this point will not fire (as the
API is not deprecated as of the level requested). It should be defined using
the `VTK_VERSION_CHECK(major, minor, patch)` macro.

Note that APIs on the verge of deletion (those deprecated in at least one
release) will always raise deprecation warnings.

If not set, its value defaults to the current level of VTK.
