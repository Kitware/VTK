# Create a setup.py file.
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in 
               ${VTK_BINARY_DIR}/Wrapping/Python/setup.py)

# create the VTK/Python  executable
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h.in
               ${VTK_BINARY_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h)
ADD_EXECUTABLE(vtkpython vtkPythonAppInit.cxx)
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


FIND_LIBRARY(UTIL_LIBRARY
  NAMES util
  PATHS /usr/lib
  DOC "Utility library needed for vtkpython"
)
IF(UTIL_LIBRARY)
  SET(UTIL_LIBRARY_LIB ${UTIL_LIBRARY})
ENDIF(UTIL_LIBRARY)


# Link against all the kit wrappers.
IF (APPLE)
  SET_TARGET_PROPERTIES(vtkpython PROPERTIES LINK_FLAGS "-flat_namespace -undefined suppress")
  OPTION(VTK_HAVE_FRAMEWORK_PYTHON "The system has Python installed by default (Mac OSX 10.2)" OFF)
  IF (VTK_HAVE_FRAMEWORK_PYTHON)
    SET(PYTHON_LIBRARY "-framework Python")
  ENDIF (VTK_HAVE_FRAMEWORK_PYTHON)
ENDIF (APPLE)

TARGET_LINK_LIBRARIES(vtkpython 
  ${PYTHON_LIBRARY} 
  vtkCommon 
  vtkFiltering
  vtkIO
  vtkGraphics
  vtkImaging
  ${UTIL_LIBRARY_LIB})

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
      ${UTIL_LIBRARY_LIB}
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
  IF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    #MESSAGE("In source build -- no need to copy Python files.")
  ELSE ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    FOREACH(file ${VTK_PYTHON_FILES})
      SET(src "${VTK_SOURCE_DIR}/Wrapping/Python/${file}.py")
      SET(tgt "${VTK_BINARY_DIR}/Wrapping/Python/${file}.py")
      CONFIGURE_FILE(${src} ${tgt} COPYONLY)
    ENDFOREACH(file)
    CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/vtk.pth
                   ${VTK_BINARY_DIR}/Wrapping/Python/vtk.pth COPYONLY)
  ENDIF ("${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

# Byte compile the Python files.
  SET(py_compile_quiet 1)
  IF (VERBOSE_BUILD)
    SET(py_compile_quiet 0)
  ENDIF (VERBOSE_BUILD)

  ADD_CUSTOM_COMMAND(
    SOURCE vtkpython_pyc
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS -c "\"import" "compileall;compileall.compile_dir('${VTK_BINARY_DIR}/Wrapping/Python',quiet=${py_compile_quiet})\""
    TARGET vtkpython_pyc
    )

ENDIF(PYTHON_EXECUTABLE)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
