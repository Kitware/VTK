# Create a setup.py file.
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in 
               ${VTK_BINARY_DIR}/Wrapping/Python/setup.py)

# create the VTK/Python  executable
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h.in
               ${VTK_BINARY_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h)

ADD_EXECUTABLE(vtkpython vtkPythonAppInit.cxx)
IF(CMAKE_SYSTEM_NAME MATCHES "AIX")
  GET_FILENAME_COMPONENT(CMAKE_PYTHON_LIB_PREFIX "${PYTHON_LIBRARY}" PATH)
  FIND_FILE(CMAKE_PYTHON_LIBRARY_EXPORT python.exp "${CMAKE_PYTHON_LIB_PREFIX}")
  SET_TARGET_PROPERTIES( vtkpython PROPERTIES LINK_FLAGS
    "-Wl,-bE:${CMAKE_PYTHON_LIBRARY_EXPORT}")
ENDIF(CMAKE_SYSTEM_NAME MATCHES "AIX")

IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      SOURCE vtkpython
      COMMAND ${VTK_APPLE_RESOURCE}
      ARGS Carbon.r -o ${VTK_EXECUTABLE_DIR}/vtkpython
      TARGET vtkpython
      )
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(VTK_USE_CARBON)


FIND_LIBRARY(PYTHON_UTIL_LIBRARY
  NAMES util
  PATHS /usr/lib
  DOC "Utility library needed for vtkpython"
)
MARK_AS_ADVANCED(PYTHON_UTIL_LIBRARY)
IF(PYTHON_UTIL_LIBRARY)
  SET(PYTHON_UTIL_LIBRARY_LIB ${PYTHON_UTIL_LIBRARY})
ENDIF(PYTHON_UTIL_LIBRARY)


IF (APPLE)
  SET_TARGET_PROPERTIES(vtkpython PROPERTIES LINK_FLAGS "-flat_namespace -undefined suppress -u _PyMac_Error")
ENDIF (APPLE)

# Link against all the kit wrappers.
TARGET_LINK_LIBRARIES(vtkpython 
  ${PYTHON_LIBRARY} 
  vtkCommon 
  vtkFiltering
  vtkIO
  vtkGraphics
  vtkImaging
  ${PYTHON_UTIL_LIBRARY_LIB})
IF(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES(vtkpython ${TCL_LIBRARY} ${TK_LIBRARY})
ENDIF(VTK_WRAP_TCL)

IF(BORLAND)
  SET(KITS Common Filtering Graphics IO Imaging)
  IF (VTK_USE_HYBRID)
    SET(KITS ${KITS} Hybrid)
  ENDIF(VTK_USE_HYBRID)
  IF (VTK_USE_PARALLEL)
    SET(KITS ${KITS} Parallel)
  ENDIF(VTK_USE_PARALLEL)
  IF (VTK_USE_PATENTED)
    SET(KITS ${KITS} Patented)
  ENDIF(VTK_USE_PATENTED)
  IF (VTK_USE_RENDERING)
    SET(KITS ${KITS} Rendering)
  ENDIF(VTK_USE_RENDERING)
  FOREACH(KIT ${KITS})
    WRITE_FILE(${LIBRARY_OUTPUT_PATH}/vtk${KIT}Python.def
               "EXPORTS\ninitvtk${KIT}Python=_initvtk${KIT}Python\n")
  ENDFOREACH(KIT)
ENDIF(BORLAND)


IF (VTK_USE_RENDERING)
  TARGET_LINK_LIBRARIES(vtkpython vtkRendering)
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkpython vtkPatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_HYBRID)
  TARGET_LINK_LIBRARIES(vtkpython vtkHybrid)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PARALLEL)
  TARGET_LINK_LIBRARIES(vtkpython vtkParallel)
ENDIF (VTK_USE_PARALLEL)

# Create the pvtkpython Python wrapper executable with MPI support.
IF (VTK_USE_PARALLEL)
  IF (VTK_USE_MPI)
    ADD_EXECUTABLE(pvtkpython vtkParaPythonAppInit.cxx)
    IF(CMAKE_SYSTEM_NAME MATCHES "AIX")
      SET_TARGET_PROPERTIES( pvtkpython PROPERTIES LINK_FLAGS
        "-Wl,-bE:${CMAKE_PYTHON_LIBRARY_EXPORT}")
    ENDIF(CMAKE_SYSTEM_NAME MATCHES "AIX")
    IF(VTK_USE_CARBON)
      FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
      IF(VTK_APPLE_RESOURCE)
        ADD_CUSTOM_COMMAND(
          SOURCE pvtkpython
          COMMAND ${VTK_APPLE_RESOURCE}
          ARGS Carbon.r -o ${VTK_EXECUTABLE_DIR}/pvtkpython
          TARGET pvtkpython
          )
      ENDIF(VTK_APPLE_RESOURCE)
    ENDIF(VTK_USE_CARBON)

    TARGET_LINK_LIBRARIES (pvtkpython
      ${PYTHON_LIBRARY}
      vtkCommon
      vtkFiltering
      vtkIO
      vtkGraphics
      vtkImaging
      vtkParallel
      ${PYTHON_UTIL_LIBRARY_LIB}
    )
    IF (VTK_USE_PATENTED)
      TARGET_LINK_LIBRARIES(pvtkpython vtkPatented)
    ENDIF (VTK_USE_PATENTED)
    IF (VTK_USE_HYBRID)
      TARGET_LINK_LIBRARIES(pvtkpython vtkHybrid)
    ENDIF (VTK_USE_HYBRID)
    INSTALL_TARGETS(/bin pvtkpython)
  ENDIF (VTK_USE_MPI)
ENDIF (VTK_USE_PARALLEL)

INSTALL_TARGETS(/bin vtkpython)    

# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to VTK_BINARY_DIR if it is different from VTK_SOURCE_DIR.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

IF(PYTHON_EXECUTABLE)
  ADD_CUSTOM_TARGET(vtkpython_pyc ALL echo "...")

# Make the necessary directories.
  FOREACH(dir
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/gtk
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/qt
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/tk
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/util
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/wx
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/test
    )
    MAKE_DIRECTORY(${dir})
  ENDFOREACH(dir)

# Now create a list of Python files.

# Files in Wrapping/Python (these are actually for backwards compatibility)
  SET(VTK_PYTHON_FILES
    QVTKRenderWidget
    vtkRenderWidget
    vtkTkImageViewerWidget
    vtkTkRenderWidget
    vtkpython
    vtkpythontk
    wxVTKRenderWindow
    )

# Wrapping/Python/vtk/*.py
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/__helper
    vtk/__init__
    vtk/common
    vtk/filtering
    vtk/graphics
    vtk/hybrid
    vtk/imaging
    vtk/io
    vtk/parallel
    vtk/patented
    vtk/rendering
    vtk/required
    )

# vtk.util package
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/util/__init__
    vtk/util/vtkMethodParser
    vtk/util/misc
    vtk/util/vtkConstants
    vtk/util/vtkImageExportToArray
    vtk/util/vtkImageImportFromArray
    vtk/util/colors
    )

# vtk.test package
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/test/BlackBox
    vtk/test/Testing
    vtk/test/__init__
    )

# Tk related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/tk/__init__
    vtk/tk/vtkLoadPythonTkWidgets
    vtk/tk/vtkTkImageViewerWidget
    vtk/tk/vtkTkRenderWidget
    vtk/tk/vtkTkRenderWindowInteractor
    vtk/tk/vtkTkPhotoImage
    )

# QT related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/qt/QVTKRenderWidget
    vtk/qt/__init__
    vtk/qt/QVTKRenderWindowInteractor
    )

# wxPython related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/wx/__init__
    vtk/wx/wxVTKRenderWindow
    vtk/wx/wxVTKRenderWindowInteractor
    )

# pyGTK related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/gtk/GtkVTKRenderWindow
    vtk/gtk/__init__
    vtk/gtk/GtkVTKRenderWindowInteractor
    )

# Done listing of files.

# Now copy these files if necessary.
  SET(VTK_PYTHON_SOURCE_FILES)
  SET(VTK_PYTHON_OUTPUT_FILES)
  FOREACH(file ${VTK_PYTHON_FILES})
    SET(src "${VTK_BINARY_DIR}/Wrapping/Python/${file}.py")
    SET(VTK_PYTHON_SOURCE_FILES ${VTK_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  IF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    #MESSAGE("In source build -- no need to copy Python files.")
  ELSE ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    FOREACH(file ${VTK_PYTHON_FILES})
      SET(src "${VTK_SOURCE_DIR}/Wrapping/Python/${file}.py")
      SET(tgt "${VTK_BINARY_DIR}/Wrapping/Python/${file}.py")
      ADD_CUSTOM_COMMAND(SOURCE ${src}
		         COMMAND ${CMAKE_COMMAND}
                         ARGS -E copy ${src} ${tgt}
                         OUTPUTS ${tgt}
			 TARGET vtkpython_pyc
                         COMMENT "source copy")
    ENDFOREACH(file)
    CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/vtk.pth
                   ${VTK_BINARY_DIR}/Wrapping/Python/vtk.pth COPYONLY)
  ENDIF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

# Byte compile the Python files.
  WRITE_FILE(${VTK_BINARY_DIR}/Wrapping/Python/compile_all_vtk
        "import compileall\n"
        "compileall.compile_dir('${VTK_BINARY_DIR}/Wrapping/Python')\n"
	"file = open('${VTK_BINARY_DIR}/Wrapping/Python/vtk_compile_complete', 'w')\n"
	"file.write('Done')\n")

  ADD_CUSTOM_COMMAND(
    SOURCE ${VTK_BINARY_DIR}/Wrapping/Python/compile_all_vtk
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${VTK_BINARY_DIR}/Wrapping/Python/compile_all_vtk
    DEPENDS ${VTK_PYTHON_SOURCE_FILES}
    OUTPUTS "${VTK_BINARY_DIR}/Wrapping/Python/vtk_compile_complete"
    TARGET vtkpython_pyc
    )

  ADD_CUSTOM_COMMAND(
    SOURCE vtkpython_pyc
    DEPENDS "${VTK_BINARY_DIR}/Wrapping/Python/vtk_compile_complete"
    TARGET vtkpython_pyc
    )

ENDIF(PYTHON_EXECUTABLE)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
