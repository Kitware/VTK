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

# Precompile python scripts
SET(VTK_COMPILE_PYTHON_SCRIPTS 0)
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
  SET(VTK_COMPILE_PYTHON_SCRIPTS 1)
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
IF(NOT PYTHON_EXECUTABLE)
  SET(VTK_COMPILE_PYTHON_SCRIPTS 0)
ENDIF(NOT PYTHON_EXECUTABLE)

IF(VTK_COMPILE_PYTHON_SCRIPTS)
  ADD_CUSTOM_TARGET(vtkpython_pyc ALL echo "...")
  FOREACH(dir
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/gtk
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/qt
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/tk
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/util
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/wx
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/testing
    ${VTK_BINARY_DIR}/Wrapping/Python/vtk/test
    )
    MAKE_DIRECTORY(${dir})
  ENDFOREACH(dir)
  SET(VTK_PYTHON_LIBRARY_FILES "")

  SET(VTK_PYTHON_FILES
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
    vtk/tk/__init__
    vtk/tk/vtkLoadPythonTkWidgets
    vtk/tk/vtkTkImageViewerWidget
    vtk/tk/vtkTkRenderWidget
    vtk/tk/vtkTkRenderWindowInteractor
    vtk/tk/vtkTkPhotoImage
    vtk/util/__init__
    vtk/util/vtkMethodParser
    vtk/util/misc
    vtk/util/vtkConstants
    vtk/util/vtkImageExportToArray
    vtk/util/vtkImageImportFromArray
    vtk/util/colors
    vtk/test/BlackBox
    vtk/test/Testing
    vtk/test/__init__
    vtkRenderWidget
    vtkTkImageViewerWidget
    vtkTkRenderWidget
    vtkpython
    vtkpythontk
    )

  IF(VTK_PYTHON_QT)
    SET(VTK_PYTHON_FILES 
        ${VTK_PYTHON_FILES}
        vtk/qt/QVTKRenderWidget
        vtk/qt/__init__
        vtk/qt/QVTKRenderWindowInteractor
        QVTKRenderWidget
        )
  ENDIF(VTK_PYTHON_QT)

  IF(VTK_PYTHON_WX)
    SET(VTK_PYTHON_FILES 
        ${VTK_PYTHON_FILES}
        vtk/wx/__init__
        vtk/wx/wxVTKRenderWindow
        vtk/wx/wxVTKRenderWindowInteractor
        wxVTKRenderWindow
        )
  ENDIF(VTK_PYTHON_WX)

  IF(VTK_PYTHON_GTK)
    SET(VTK_PYTHON_FILES 
        ${VTK_PYTHON_FILES}
        vtk/gtk/GtkVTKRenderWindow
        vtk/gtk/__init__
        vtk/gtk/GtkVTKRenderWindowInteractor
        )
  ENDIF(VTK_PYTHON_GTK)

  FOREACH(file
    ${VTK_PYTHON_FILES}
    )
    SET(src "${VTK_SOURCE_DIR}/Wrapping/Python/${file}.py")
    SET(tgt "${VTK_BINARY_DIR}/Wrapping/Python/${file}.pyc")
    ADD_CUSTOM_COMMAND(SOURCE ${src}
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS -c "\"import" "py_compile;py_compile.compile('${src}','${tgt}','${tgt}')\""
      TARGET vtkpython_pyc
      OUTPUTS ${tgt}
      )
    SET(VTK_PYTHON_LIBRARY_FILES ${VTK_PYTHON_LIBRARY_FILES} ${tgt})
  ENDFOREACH(file)

  ADD_CUSTOM_COMMAND(
    SOURCE vtkpython_pyc
    COMMAND echo 
    ARGS ".."
    DEPENDS ${VTK_PYTHON_LIBRARY_FILES}
    TARGET vtkpython_pyc
    )
ENDIF(VTK_COMPILE_PYTHON_SCRIPTS)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
