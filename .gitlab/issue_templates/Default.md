<!--
Thanks for taking the time to report an issue with VTK.

Before filing, please:
  * Search the issue tracker to see whether the problem is already reported:
    https://gitlab.kitware.com/vtk/vtk/-/issues
  * Ask usage ("how do I ...?") questions and propose new features on the VTK
    Discourse forum instead: https://discourse.vtk.org/
  * If you ran into the problem through an application built on VTK, report it
    to that project first. Once it has been established there that the bug is
    in VTK itself and not in the application, open an issue here and include a
    link to the report you filed with them along with any other relevant
    information. The following are the issue trackers for some of the most
    common VTK-based applications:
      ParaView   https://gitlab.kitware.com/paraview/paraview/-/issues
      3D Slicer  https://discourse.slicer.org/
      tomviz     https://github.com/OpenChemistry/tomviz/issues
      trame      https://github.com/Kitware/trame/issues
      VolView    https://github.com/Kitware/VolView/issues
      vtk-wasm   https://github.com/Kitware/vtk-wasm/issues
      vtk-js     https://github.com/Kitware/vtk-js/issues
  * Keep this template's headings so the issue is easy to triage.
-->

## Description
<!-- A clear and concise description of the problem, with screenshots if
applicable. What is the behavior you see and what behavior do you expect? -->

## Steps to reproduce
<!-- Steps to reproduce the behavior. Include a minimal example (C++ or
Python), and example data files that demonstrate the behavior. Screenshots may
be helpful. -->

## Configuration

* **Operating system and version:** <!-- e.g. Ubuntu 24.04, Windows 11 24H2,
  macOS 15.2 (arm64) -->
* **VTK version:** <!-- release number, e.g. 9.5.0; for a source build, the
  output of `git describe`, e.g. v9.7.1-1695-gb0a99f2669 -->
* **How VTK was obtained:** <!-- e.g. `pip install vtk==9.5.0`, conda-forge,
  built from source with CMake, distribution package, or bundled by another
  application (give its name and version) -->
* **Language and version:** <!-- e.g. C++17, Python 3.12.3, Java 21,
  JavaScript/WebAssembly in Chrome 142 -->
* **Compiler and version:** <!-- source builds only, e.g. GCC 14.2, Clang 19.1,
  MSVC 19.42 (VS 2022 17.12), Apple Clang 16 -->
* **Graphics backend / GPU and driver version:** <!-- rendering issues only,
  e.g. OpenGL on NVIDIA RTX A5000 driver 550.120, WebGPU/Dawn, OSMesa 24.2
  (software), EGL, ANARI -->
* **Related report in a downstream project:** <!-- if you got here from the
  note above, link the report you filed there, e.g.
  https://gitlab.kitware.com/paraview/paraview/-/issues/12345 -->

<!--
Finally, please label the issue so that it reaches the right developers. Use
the Labels dropdown on this page, or put a quick action on a line of its own in
this description, for example:

    /label ~"area:rendering" ~"lang:python"

The full list, with descriptions, is at
https://gitlab.kitware.com/vtk/vtk/-/labels

Reporters are asked for two kinds of label: an `area:` label for each part of
VTK affected, and a `lang:` label only when the problem is specific to a
language binding rather than to VTK itself. If nothing in the list fits, leave
the labels off rather than guessing. The `triage:`, `workflow:` and `project:`
labels are set by the VTK developers during triage.
-->
