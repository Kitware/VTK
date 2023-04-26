.. include:: macros.hrst

CMake
-----

For a thorough description of the module system see the :doc:`ModuleSystem` section.

The CMake API can be separated into several categories:

.. _module-bullet:

* **Module APIs** |module|

  This category includes functions to find and build VTK modules. A module is a set
  of related functionality. These are then compiled together into libraries at
  the "kit" level. Each module may be enabled or disabled individually and its
  dependencies will be built as needed.

  All functions strictly check their arguments. Any unrecognized or invalid
  values for a function cause errors to be raised.

.. _module-internal-bullet:

* **Internal APIs** |module-internal|

  The VTK module system provides some API functions for use by other code which
  consumes VTK modules (primarily language wrappers). This file documents these
  APIs. They may start with `_vtk_module`, but they are intended for use in cases
  of language wrappers or dealing with trickier third party packages.

.. _module-impl-bullet:

* **Implementation APIs** |module-impl|

  These functions are purely internal implementation details. No guarantees are
  made for them and they may change at any time (including wrapping code calls).
  Note that these functions are usually very lax in their argument parsing.

.. _module-wrapping-python:

* **Python Wrapping APIs** |module-wrapping-python|

  APIs for wrapping modules for Python.

.. _module-wrapping-java:

* **Java Wrapping APIs** |module-wrapping-java|

  APIs for wrapping modules for Java.

.. _module-support-bullet:

* **Support APIs**  |module-support|

  Miscellaneous utilities.



.. toctree::
   :maxdepth: 1
   :hidden:

   ./ModuleSystem.md
   ./vtkModule.rst
   ./vtkModuleTesting.rst
   ./vtkModuleWrapPython.rst
   ./vtkModuleWrapJava.rst
   ./vtkModuleJSON.rst
   ./vtkModuleGraphviz.rst
