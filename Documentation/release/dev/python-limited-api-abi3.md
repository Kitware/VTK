## Build Python wrappers against the CPython stable ABI (abi3)

You can now build VTK's Python wrappers against the CPython Limited API so
that a single set of compiled modules loads on any Python 3.x interpreter
at or above a chosen floor, instead of being tied to the exact version
they were built with.

Enable it with the advanced option `VTK_PYTHON_LIMITED_API` (default
`OFF`, so existing builds are unchanged). The floor is controlled by
`VTK_PYTHON_LIMITED_API_VERSION` (a `PY_VERSION_HEX` value, default
`0x030B0000` for Python 3.11, the lowest version that supports everything
the wrappers need).

When the option is on:

* The wrapped modules are stable-ABI binaries, named `<module>.abi3.so` on
  Unix and `<module>.pyd` on Windows, following the same convention as
  abi3 wheels produced by setuptools.
* The modules are staged and installed under a version-neutral
  `python3/site-packages` layout rather than `pythonX.Y/site-packages`,
  since one binary serves every interpreter above the floor.

This is intended for distributors who want to ship one set of Python
modules across several Python minor versions. Code that embeds and starts
a Python interpreter (`vtkPythonInterpreter` and the `vtkpython`
executables) necessarily uses the full CPython API, which is outside the
stable ABI; that code is built against the full API as before and is not
part of the version-independent module set.
