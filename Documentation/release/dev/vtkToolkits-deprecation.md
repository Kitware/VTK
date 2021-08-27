## vtkToolkits.h is deprecated

The `vtkToolkits.h` header provided preprocessor definitions indicating some
features for VTK's build as a whole. Most of these were never set and could be
removed unconditionally (as they were never set by the build system).

The others were not reliable in that they'd only work if VTK was configured
multiple times (this is not performed for CI or any release artifacts, so they
were wrong in these instances). These definitions are instead moved to the
relevant module where the values are always trustworthy.

  - `VTK_USE_VIDEO_FOR_WINDOWS` is now in `vtkIOMovieConfigure.h`
  - `VTK_USE_VFW_CAPTURE` is now in `vtkIOVideoConfigure.h` (now named
    `VTK_USE_VIDEO_FOR_WINDOWS_CAPTURE`, but the old name is provided for
    compatibility)
