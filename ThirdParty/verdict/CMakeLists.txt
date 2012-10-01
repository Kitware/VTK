set(vtkverdict_THIRD_PARTY 1)
set(vtkverdict_LIBRARIES verdict)
vtk_module_export_info()

set(verdict_INSTALL_RUNTIME_DIR ${VTK_INSTALL_RUNTIME_DIR})
set(verdict_INSTALL_LIBRARY_DIR ${VTK_INSTALL_LIBRARY_DIR})
set(verdict_INSTALL_ARCHIVE_DIR ${VTK_INSTALL_ARCHIVE_DIR})

set(verdict_INSTALL_DOC_DIR ${VTK_INSTALL_DOC_DIR})
set(verdict_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})
set(verdict_INSTALL_INCLUDE_DIR ${VTK_INSTALL_INCLUDE_DIR}/vtkverdict)
set(verdict_INSTALL_NO_DEVELOPMENT ${VTK_INSTALL_NO_DEVELOPMENT})

set(VERDICT_NO_LIBRARY_VERSION 1)
set(VERDICT_BUILD_DOC OFF CACHE INTERNAL "Build the 2007 Verdict User Manual")
set(VERDICT_MANGLE OFF CACHE INTERNAL "Mangle verdict names for inclusion in a larger library?")
set(VERDICT_MANGLE_PREFIX "" CACHE INTERNAL "A string to prepend to all verdict function names and classes." )
set(VERDICT_ENABLE_TESTING OFF CACHE INTERNAL  "Should tests of the VERDICT library be built?")

add_subdirectory(vtkverdict)
vtk_target(verdict NO_INSTALL)
