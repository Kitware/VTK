IF(NOT VTK_SHARED_LIBRARIES_SELECTED)
  SET(VTK_SHARED_LIBRARIES_SELECTED 1)

  # We need the VTK_DEPENDENT_OPTION macro.
  INCLUDE(${VTK_CMAKE_DIR}/vtkDependentOption.cmake)

  # Choose static or shared libraries.
  OPTION(BUILD_SHARED_LIBS "Build VTK with shared libraries." OFF)
  SET(VTK_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

  # On platforms that support rpath users may want to use them to make
  # running VTK from the build tree easy.  It is not safe to install
  # programs built with an rpath pointing at the build tree, so we must
  # disable install support when using the rpath feature with CMake
  # versions prior to 2.4.
  IF(NOT WIN32)
    # Choose whether to use the rpath feature.  Parent projects may
    # set VTK_FORCE_RPATH to force the value on or off without providing
    # the option.
    IF(VTK_FORCE_RPATH)
      SET(VTK_USE_RPATH ${VTK_FORCE_RPATH})
    ELSE(VTK_FORCE_RPATH)
      IF("VTK_USE_RPATH_DEFAULT" MATCHES "^VTK_USE_RPATH_DEFAULT$")
        # Choosing a default value for this option is tricky.  Dashboard
        # scripts need to have this ON or set the LD_LIBRARY_PATH, which at
        # the time of this writing none does.  Therefore the default must be
        # ON.  However, users that download a VTK release to build and install
        # it will not know to turn this OFF and will be confused when they get
        # an empty installation.  One solution to this dilema is to note that
        # users that do not know what they are doing will generally use a
        # release.  Dashboards use the latest development version.  Users that
        # checkout from the CVS head will usually run from the build tree or
        # at least understand this option.  Therefore a simple decision
        # criterion is whether VTK_MINOR_VERSION is odd or even.
        IF(VTK_MINOR_VERSION MATCHES "[02468]$")
          # This is a release version.  Default to not use rpath.
          SET(VTK_USE_RPATH_DEFAULT OFF)
        ELSE(VTK_MINOR_VERSION MATCHES "[02468]$")
          # This is a development version.  Default to use rpath.
          SET(VTK_USE_RPATH_DEFAULT ON)
        ENDIF(VTK_MINOR_VERSION MATCHES "[02468]$")
      ENDIF("VTK_USE_RPATH_DEFAULT" MATCHES "^VTK_USE_RPATH_DEFAULT$")
      VTK_DEPENDENT_OPTION(VTK_USE_RPATH "Build shared libraries with rpath.  This makes it easy to run executables from the build tree when using shared libraries, but removes install support."
                           ${VTK_USE_RPATH_DEFAULT}
                           "BUILD_SHARED_LIBS" OFF)
    ENDIF(VTK_FORCE_RPATH)

    # Configure VTK according to the rpath setting.
    IF(VTK_USE_RPATH)
      # We will use rpath support.  Tell CMake not to skip it.
      SET(CMAKE_SKIP_RPATH 0 CACHE INTERNAL "Whether to build with rpath." FORCE)

      # Disable installation for CMake earlier than 2.4.
      IF(NOT VTK_INSTALL_HAS_CMAKE_24)
        # If someone is trying to install do not do an entire build with
        # the wrong rpath feature setting just to report failed
        # installation.
        SET(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY 1)

        # Add a dummy target and attach an install rule that will always fail
        # and produce a message explaining why installation is disabled.
        ADD_CUSTOM_TARGET(vtk_install_disabled)
        SET_TARGET_PROPERTIES(vtk_install_disabled PROPERTIES
          PRE_INSTALL_SCRIPT ${VTK_CMAKE_DIR}/InstallDisabled.cmake)
      ENDIF(NOT VTK_INSTALL_HAS_CMAKE_24)
    ELSE(VTK_USE_RPATH)
      # We will not use rpath support.  Tell CMake to skip it.
      SET(CMAKE_SKIP_RPATH 1 CACHE INTERNAL "Whether to build with rpath." FORCE)
    ENDIF(VTK_USE_RPATH)
  ENDIF(NOT WIN32)
ENDIF(NOT VTK_SHARED_LIBRARIES_SELECTED)
