IF (VTK_WRAP_TCL)

    # Do we need to copy the Tcl/Tk support files to the build directory ?
    # Yes if we are building against  static Tcl/Tk, and also yes if we
    # are building against a shared Tcl/Tk which was provided with no
    # support files (turn-key apps like ParaViewComplete for example).
    # In the latter case, it's up to the user or the top CMakeLists.txt 
    # file to set TCL_TK_COPY_SUPPORT_LIBRARY to ON, since there is no
    # reliable way to know that the Tcl/Tk shared lib was provided with
    # no support files.

    OPTION(TCL_TK_COPY_SUPPORT_LIBRARY "Very few users should worry about this option. If VTK is built against a static Tcl/Tk lib (see TCL_TK_STATIC) or a shared Tcl/Tk bundled inside a project with no library support files (ex: ParaViewComplete), this variable should be set to ON and both TCL_SUPPORT_LIBRARY_PATH and TK_SUPPORT_LIBRARY_PATH should point to the directories that hold those files (typically, lib/tcl8.4 and lib/tk8.4 for a typical Tcl/Tk installation, or tcl8.4.5/library and tk8.4.5/library for a Tcl/Tk source repository). Once this variable is set to ON, support files will automatically be copied to the build directory and the executables will try to use that location to initialize Tcl/Tk." ${TCL_TK_STATIC})
    MARK_AS_ADVANCED (TCL_TK_COPY_SUPPORT_LIBRARY)

    IF (TCL_TK_COPY_SUPPORT_LIBRARY)
      VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
      SET (TCL_TK_VERSION "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")

      GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH ${TCL_LIBRARY} PATH)
      FIND_PATH (TCL_SUPPORT_LIBRARY_PATH 
                 "ldAout.tcl"
                 PATH
                 "${TCL_LIBRARY_PATH}/tcl${TCL_TK_VERSION}"
                 "${TCL_INCLUDE_PATH}/../lib/tcl${TCL_TK_VERSION}"
                 "${TCL_LIBRARY_PATH}/../library"
                 "${TCL_INCLUDE_PATH}/../library"
                 DOC "Path to the Tcl support library files. See TCL_TK_COPY_SUPPORT_LIBRARY help.")
      MARK_AS_ADVANCED (TCL_SUPPORT_LIBRARY_PATH)

      GET_FILENAME_COMPONENT(TK_LIBRARY_PATH ${TK_LIBRARY} PATH)
      FIND_PATH (TK_SUPPORT_LIBRARY_PATH 
                 "clrpick.tcl"
                 PATH
                 "${TK_LIBRARY_PATH}/tk${TCL_TK_VERSION}"
                 "${TK_INCLUDE_PATH}/../lib/tk${TCL_TK_VERSION}"
                 "${TK_LIBRARY_PATH}/../library"
                 "${TK_INCLUDE_PATH}/../library"
                 DOC "Path to the Tk support library files. See TCL_TK_COPY_SUPPORT_LIBRARY help.")
      MARK_AS_ADVANCED (TK_SUPPORT_LIBRARY_PATH)

      IF (CXX_TEST_PATH)
        IF(TCL_SUPPORT_LIBRARY_PATH AND TK_SUPPORT_LIBRARY_PATH)
          VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR (
           ${TCL_SUPPORT_LIBRARY_PATH}
           ${TK_SUPPORT_LIBRARY_PATH}
           ${CXX_TEST_PATH} "TclTk/lib")
        ENDIF(TCL_SUPPORT_LIBRARY_PATH AND TK_SUPPORT_LIBRARY_PATH)
      ENDIF (CXX_TEST_PATH)

      # Install targets should be added here, so that the support files 
      # are installed somewhere. vtkTkAppInit.cxx will have to be updated
      # accordingly, in the same way vtkKWApplication was (KW_INSTALL_DIR).

    ENDIF (TCL_TK_COPY_SUPPORT_LIBRARY)

ENDIF (VTK_WRAP_TCL)
