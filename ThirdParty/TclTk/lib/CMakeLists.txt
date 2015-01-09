IF (TCL_FOUND AND TK_FOUND)

  # Do we need to copy the Tcl/Tk support files to the build directory ?
  # Yes if we are building against  static Tcl/Tk, and also yes if we
  # are building against a shared Tcl/Tk which was provided with no
  # support files (turn-key apps like ParaViewComplete for example).
  # In the latter case, it's up to the user or the top CMakeLists.txt 
  # file to set VTK_TCL_TK_COPY_SUPPORT_LIBRARY to ON, since there is no
  # reliable way to know that the Tcl/Tk shared lib was provided with
  # no support files.

  SET (VTK_TCL_TK_COPY_SUPPORT_LIBRARY ${VTK_TCL_TK_STATIC} CACHE INTERNAL "Very few users should worry about this option. If VTK is built against a static Tcl/Tk lib (see VTK_TCL_TK_STATIC) or a shared Tcl/Tk bundled inside a project with no library support files (ex: ParaViewComplete), this variable should be set to ON and both VTK_TCL_SUPPORT_LIBRARY_PATH and VTK_TK_SUPPORT_LIBRARY_PATH should point to the directories that hold those files (typically, lib/tcl8.4 and lib/tk8.4 for a typical Tcl/Tk installation, or tcl8.4.5/library and tk8.4.5/library for a Tcl/Tk source repository). Once this variable is set to ON, support files will automatically be copied to the build directory and the executables will try to use that location to initialize Tcl/Tk.")

  IF (VTK_TCL_TK_COPY_SUPPORT_LIBRARY)
    VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
    SET (TCL_TK_VERSION "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")

    GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH ${TCL_LIBRARY} PATH)
    SET (TRY_DIRS
         "${TCL_LIBRARY_PATH}/tcl${TCL_TK_VERSION}"
         "${TCL_INCLUDE_PATH}/../lib/tcl${TCL_TK_VERSION}"
         "${TCL_LIBRARY_PATH}/../library"
         "${TCL_INCLUDE_PATH}/../library")
    FOREACH (dir ${TRY_DIRS})
      IF (EXISTS "${dir}/auto.tcl")
        SET (VTK_TCL_SUPPORT_LIBRARY_PATH ${dir} CACHE INTERNAL
             "Path to the Tcl support library files.")
      ENDIF ()
    ENDFOREACH ()

    GET_FILENAME_COMPONENT(TK_LIBRARY_PATH ${TK_LIBRARY} PATH)
    SET (TRY_DIRS
         "${TK_LIBRARY_PATH}/tk${TCL_TK_VERSION}"
         "${TK_INCLUDE_PATH}/../lib/tk${TCL_TK_VERSION}"
         "${TK_LIBRARY_PATH}/../library"
         "${TK_INCLUDE_PATH}/../library")
    FOREACH (dir ${TRY_DIRS})
      IF (EXISTS "${dir}/clrpick.tcl")
        SET (VTK_TK_SUPPORT_LIBRARY_PATH ${dir} CACHE INTERNAL
             "Path to the Tk support library files.")
      ENDIF ()
    ENDFOREACH ()

    IF(VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)

      # required for the vtk.exe interpreter to work
      IF(VTK_WRAP_TCL) 
        IF(NOT DEFINED TclTk_LIB_DIR)
          SET(TclTk_LIB_DIR "${PROJECT_BINARY_DIR}/lib")
        ENDIF()
        VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR (
          ${VTK_TCL_SUPPORT_LIBRARY_PATH}
          ${VTK_TK_SUPPORT_LIBRARY_PATH}
          "${TclTk_LIB_DIR}")
      ENDIF()

      IF(NOT VTK_INSTALL_NO_RUNTIME)
        VTK_COPY_TCL_TK_SUPPORT_FILES(
          "${VTK_TCL_SUPPORT_LIBRARY_PATH}"
          "${VTK_INSTALL_TCL_DIR}/tcl${TCL_TK_VERSION}"
          "${VTK_TK_SUPPORT_LIBRARY_PATH}"
          "${VTK_INSTALL_TCL_DIR}/tk${TCL_TK_VERSION}"
          INSTALL)
      ENDIF()

      IF(NOT VTK_INSTALL_NO_RUNTIME)
        VTK_GET_TCL_SUPPORT_FILES(
          ${VTK_TCL_SUPPORT_LIBRARY_PATH} "TCL_SUPPORT_FILES")
        FOREACH (file ${TCL_SUPPORT_FILES})
          STRING (REGEX REPLACE "${VTK_TCL_SUPPORT_LIBRARY_PATH}/" "" base ${file})
          GET_FILENAME_COMPONENT(dest ${base} PATH)
          INSTALL(FILES "${file}"
            DESTINATION "${VTK_INSTALL_TCL_DIR}/tcl${TCL_TK_VERSION}/${dest}"
            COMPONENT RuntimeLibraries
            )
        ENDFOREACH ()

        VTK_GET_TK_SUPPORT_FILES(${VTK_TK_SUPPORT_LIBRARY_PATH} "TK_SUPPORT_FILES")
        FOREACH (file ${TK_SUPPORT_FILES})
          STRING (REGEX REPLACE "${VTK_TK_SUPPORT_LIBRARY_PATH}/" "" base ${file})
          GET_FILENAME_COMPONENT(dest ${base} PATH)
          INSTALL(FILES "${file}"
            DESTINATION "${VTK_INSTALL_TCL_DIR}/tk${TCL_TK_VERSION}/${dest}"
            COMPONENT RuntimeLibraries
            )
        ENDFOREACH ()
      ENDIF()

    ENDIF()

  ENDIF ()

ENDIF ()
