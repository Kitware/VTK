# utf8cpp fork for ParaView

This branch contains changes required to embed utf8cpp into VTK. This includes
changes made primarily to the build system to allow it to be embedded into
another source tree.

  * Add `.gitattributes` to pass VTK's commit checks.
  * Add a `CMakeLists.txt` to install the headers.
  * Rename the namespace to not conflict with external copies.
