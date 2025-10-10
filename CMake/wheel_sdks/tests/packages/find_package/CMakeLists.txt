cmake_minimum_required(VERSION 3.15...4.0)

project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

# Both Python and VTK will be found thanks to scikit build core CMake prefix path injections 
find_package(Python COMPONENTS Interpreter Development.Module)
find_package(VTK CONFIG)

set(skipped_components
    # Not VTK modules, from
    # https://gitlab.kitware.com/vtk/vtk/-/blob/master/CMake/vtkInstallCMakePackage.cmake#L19
    WrapHierarchy
    vtkbuild
    vtkplatform
    vtkpython
    pvtkpython
    WrapPython
    WrapPythonInit
    vtkjava
    ParseJava
    WrapJava
    vtkWebAssemblyTestLinkOptions

    # Public dependency to OpenXR loader makes these hard to import, ignore them
    RenderingOpenXR
    RenderingOpenXRRemoting)

foreach(comp IN LISTS VTK_AVAILABLE_COMPONENTS)
  if(comp IN_LIST skipped_components)
    continue()
  endif()
  if(NOT VTK_${comp}_FOUND)
    message(SEND_ERROR "Expected VTK component \"${comp}\" could not be found!")
  endif()
endforeach()
