PROJECT(vtkMFC)

# This INCLUDE is to allow developers with both MSVC and Borland compilers installed
# to manually change the FindVTK results to point to the correct (MSVC) vtk build directory
INCLUDE ($ENV{VTK_SOURCE_DIR}/Examples/GUI/Win32/vtkMFC/LocalUserOptions.cmake OPTIONAL)

IF (NOT USE_VTK_FILE)
  INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
ENDIF (NOT USE_VTK_FILE)

IF (USE_VTK_FILE)
  INCLUDE(${USE_VTK_FILE})
ENDIF (USE_VTK_FILE)

SUBDIRS(  
  vtkMDI
  vtkSDI
)
