if (WIN32)
  if (building_vtkpythonrc)
    # XXX: With long paths, this command line can get too long because of all
    # the include directories required for vtkpython to work. The thing is that
    # CMake's RC rule in Ninja does not support response files, so the path can
    # end up way too long. Since this doesn't need those include paths, act as
    # if it is in a separate directory to avoid all the include_directories
    # used here.
    add_library(vtkpythonrc STATIC dummy.cxx vtkpython.rc)
    return()
  elseif (CMAKE_GENERATOR MATCHES "Visual Studio")
    set(extra_srcs vtkpython.rc)
  else ()
    set(building_vtkpythonrc TRUE)
    set(extra_links vtkpythonrc)
    # Make a separate directory scope for building vtkpythonrc.
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/vtkpythonrc")
  endif ()
endif ()

# This is where we actually wrap the modules that have requested it.
include(vtkPythonWrapping)

get_property(VTK_PYTHON_MODULES GLOBAL PROPERTY VTK_PYTHON_WRAPPED)

# Get the include directories for the module and all its dependencies.
macro(vtk_include_recurse module)
  _vtk_module_config_recurse("${module}_PYTHON" ${module})
  include_directories(${${module}_PYTHON_INCLUDE_DIRS})
endmacro()


set(VTK_PYTHON_MODULES_AND_KITS ${VTK_PYTHON_MODULES})
set(_vtk_python_modules_only ${VTK_PYTHON_MODULES})

if(VTK_ENABLE_KITS)
  set(_vtk_kits_with_suffix)
  set(VTK_KIT_SUFFIX "Kit") # Required to avoid conflict with targets like vtkFiltersPython
  # Create list of module that do not belong to any kits
  foreach(kit IN LISTS vtk_kits)
    # XXX Variable '_${kit}_modules' is set in vtkModuleTop.cmake
    list(REMOVE_ITEM _vtk_python_modules_only ${_${kit}_modules})
    list(APPEND _vtk_kits_with_suffix ${kit}${VTK_KIT_SUFFIX})
  endforeach()
  set(VTK_PYTHON_MODULES_AND_KITS ${_vtk_kits_with_suffix} ${_vtk_python_modules_only})

  # Create dependency lists for kits (suffix: _DEPENDS). The function
  # vtk_add_python_wrapping_libary uses these lists.
  #
  # Additionally, create subsets of the full dependency lists that contain only
  # Python-wrapped kits and modules (suffix: _PYTHON_DEPENDS). These lists are
  # used to topologically sort the dependency graph.
  foreach(kit IN LISTS vtk_kits)
    set(_module_depends)
    foreach(module IN LISTS _${kit}_modules)
      vtk_module_load(${module})
      list(APPEND _module_depends ${${module}_DEPENDS})
    endforeach()

    set(_kit_depends)
    foreach(dep IN LISTS _module_depends)
      if(${dep}_KIT)
        list(APPEND _kit_depends ${${dep}_KIT}${VTK_KIT_SUFFIX})
      else()
        list(APPEND _kit_depends ${dep})
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _kit_depends)
    set(${kit}${VTK_KIT_SUFFIX}_DEPENDS ${_kit_depends})

    set(_kit_python_depends)
    foreach(module IN LISTS ${kit}${VTK_KIT_SUFFIX}_DEPENDS)
      list(FIND VTK_PYTHON_MODULES_AND_KITS ${module} _module_index)
      if (_module_index GREATER -1)
        list(APPEND _kit_python_depends ${module})
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _kit_python_depends)
    set(${kit}${VTK_KIT_SUFFIX}_PYTHON_DEPENDS ${_kit_python_depends})
  endforeach()

  # Create dependency lists for modules that also consider any dependent kits
  # (suffix: _DEPENDS_WITH_KITS). These lists are used to override
  # <module>_DEPENDS when calling vtk_add_python_wrapping_library.
  #
  # Additionally, create subsets of the full dependency lists that contain only
  # Python-wrapped kits and modules (suffix: _PYTHON_DEPENDS). These lists are
  # used to topologically sort the dependency graph.
  foreach(module IN LISTS _vtk_python_modules_only)
    vtk_module_load(${module})
    set(_saved_${module}_DEPENDS ${${module}_DEPENDS})

    set(_module_depends_with_kits)
    foreach(dep IN LISTS ${module}_DEPENDS)
      if(${dep}_KIT)
        list(APPEND _module_depends_with_kits ${${dep}_KIT}${VTK_KIT_SUFFIX})
      else()
        list(APPEND _module_depends_with_kits ${dep})
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _module_depends_with_kits)
    set(${module}_DEPENDS_WITH_KITS ${_module_depends_with_kits})

    set(_module_python_depends)
    foreach(module IN LISTS ${module}_DEPENDS_WITH_KITS)
      list(FIND VTK_PYTHON_MODULES_AND_KITS ${module} _module_index)
      if (_module_index GREATER -1)
        list(APPEND _module_python_depends ${module})
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _module_python_depends)
    set(${module}_PYTHON_DEPENDS ${_module_python_depends})
  endforeach()

  # Create list of kits and modules to wrap, ordered to satisfy dependencies.
  include(${VTK_CMAKE_DIR}/TopologicalSort.cmake)
  set(_vtk_python_wrapping_work_list ${VTK_PYTHON_MODULES_AND_KITS})
  topological_sort(_vtk_python_wrapping_work_list "" _PYTHON_DEPENDS)

  # Wrap kits and modules.
  foreach(target IN LISTS _vtk_python_wrapping_work_list)
    # Determine whether target is a kit or module
    string(REGEX REPLACE "(.+)${VTK_KIT_SUFFIX}\$" "\\1" _stripped_target ${target})
    if(_${_stripped_target}_is_kit)
      # Wrap kit
      set(kit ${_stripped_target})
      set(kit_srcs)
      foreach(module IN LISTS _${kit}_modules)
        vtk_module_headers_load(${module})
        vtk_include_recurse(${module})
      endforeach()
      vtk_add_python_wrapping("${_${kit}_modules}" kit_srcs ${kit}${VTK_KIT_SUFFIX})
      vtk_add_python_wrapping_library(${kit}${VTK_KIT_SUFFIX} kit_srcs ${_${kit}_modules})
    else()
      # Wrap module
      set(module ${_stripped_target})
      vtk_module_headers_load(${module})
      vtk_include_recurse(${module})
      vtk_add_python_wrapping(${module} module_srcs)
      # Override module dependency list for vtk_add_python_wrapping_library
      set(${module}_DEPENDS ${${module}_DEPENDS_WITH_KITS})
      vtk_add_python_wrapping_library(${module} module_srcs ${module})
      set(${module}_DEPENDS ${_saved_${module}_DEPENDS})
    endif()
  endforeach()

  # Ensure that original module dependency lists are restored
  foreach(module IN LISTS _vtk_python_modules_only)
    set(${module}_DEPENDS ${_saved_${module}_DEPENDS})
    unset(_saved_${module}_DEPENDS)
  endforeach()

else(VTK_ENABLE_KITS)
  # Loop through all modules that should be wrapped, and wrap them.
  foreach(module IN LISTS _vtk_python_modules_only)
    vtk_module_load(${module})
    vtk_module_headers_load(${module})
    vtk_include_recurse(${module})
    vtk_add_python_wrapping(${module} module_srcs)
    vtk_add_python_wrapping_library(${module} module_srcs ${module})
  endforeach()
endif(VTK_ENABLE_KITS)


option(VTK_ENABLE_VTKPYTHON "Enable vtkpython and pvtkpython binaries" ON)
mark_as_advanced(VTK_ENABLE_VTKPYTHON)
if(VTK_ENABLE_VTKPYTHON)

vtk_module_load(vtkWrappingPythonCore)
vtk_module_load(vtkPython)
include_directories(${CMAKE_CURRENT_BINARY_DIR}
  ${VTK_SOURCE_DIR}/Utilities
  ${vtkPython_INCLUDE_DIRS}
  ${vtkWrappingPythonCore_INCLUDE_DIRS})

if(UNIX)
  find_library(PYTHON_UTIL_LIBRARY
    NAMES util
    DOC "Utility library needed for vtkpython")
  mark_as_advanced(PYTHON_UTIL_LIBRARY)
endif()

# Generate the header which initializes Python modules when BUILD_SHARED_LIBS is
# OFF. The py_module_dependencies will be set to the libraries we should link
# against when we use the vtkpythonmodules.h file.
vtk_write_python_modules_header_for_wrapped_modules(
  "${CMAKE_CURRENT_BINARY_DIR}/vtkpythonmodules.h"
  py_module_dependencies)

# Create the VTK/Python  executable
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonAppInitConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonAppInitConfigure.h)

add_executable(vtkpython vtkPythonAppInit.cxx ${extra_srcs})

if (WIN32)
  target_link_libraries(vtkpython ${extra_links})
endif ()

unset(VTKPYTHON_LINK_FLAGS)
unset(VTKPYTHON_LINK_LIBS)

if(CMAKE_SYSTEM_NAME MATCHES "AIX")
  get_filename_component(CMAKE_PYTHON_LIB_PREFIX "${PYTHON_LIBRARY}" PATH)
  find_file(CMAKE_PYTHON_LIBRARY_EXPORT python.exp "${CMAKE_PYTHON_LIB_PREFIX}")
  if(CMAKE_PYTHON_LIBRARY_EXPORT)
    set(VTKPYTHON_LINK_FLAGS "-Wl,-bE:${CMAKE_PYTHON_LIBRARY_EXPORT}")
  endif()
endif()

# Link against all the kit wrappers.
list(APPEND VTKPYTHON_LINK_LIBS vtkWrappingPythonCore)

if(PYTHON_UTIL_LIBRARY)
  list(APPEND VTKPYTHON_LINK_LIBS ${PYTHON_UTIL_LIBRARY})
endif()

set(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${py_module_dependencies})

# Link to rt to prevent undefined symbol 'fdatasync'
if(CMAKE_SYSTEM MATCHES "SunOS.*" AND NOT CMAKE_COMPILER_IS_GNUCXX)
  find_library(VTK_SUNCC_RT_LIBRARY rt)
  if(VTK_SUNCC_RT_LIBRARY)
    set(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${VTK_SUNCC_RT_LIBRARY})
  endif()
endif()

# If we are wrapping Tcl then the Tk libraries are available and should be used
# in the Python wrapping too.
if(VTK_WRAP_TCL)
  list(APPEND VTKPYTHON_LINK_LIBS ${VTK_TK_LIBRARIES})
endif()

if(HAVE_PTHREAD_H AND CMAKE_USE_PTHREADS)
  list(APPEND VTKPYTHON_LINK_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

# FIXME: Is this needed here?
if(VTK_USE_FFMPEG_ENCODER)
  list(APPEND VTKPYTHON_LINK_LIBS ${FFMPEG_BASIC_LIBRARIES})
endif()

target_link_libraries(vtkpython ${VTKPYTHON_LINK_LIBS})

unset(PVTKPYTHON_EXECUTABLE)
# Create the pvtkpython Python wrapper executable with MPI support.
if(TARGET vtkParallelMPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_C_INCLUDE_PATH})
  set(MPI_LIBRARIES ${MPI_C_LIBRARIES})
  if(MPI_CXX_LIBRARIES)
    set(MPI_LIBRARIES ${MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
  endif()

  # Needed for mpich 2
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")

  set(PVTKPYTHON_EXECUTABLE pvtkpython)
  add_executable(pvtkpython vtkParaPythonAppInit.cxx)
  target_link_libraries(pvtkpython ${VTKPYTHON_LINK_LIBS} vtkParallelMPI
    ${MPI_LIBRARIES})
endif()

endif()

# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to VTK_BINARY_DIR if it is different from VTK_SOURCE_DIR.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

if(PYTHON_EXECUTABLE)
  # Make the necessary directories.
  file(MAKE_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/gtk
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/qt4
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/qt
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/tk
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/util
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/wx
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/test
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/numpy_interface)

# Now create a list of Python files.

# Wrapping/Python/vtk/*.py
  unset(VTK_PYTHON_FILES)
  unset(VTK_PYTHON_IMPORT_ALL)
  foreach(module IN LISTS VTK_PYTHON_MODULES_AND_KITS)
    set(VTK_PYTHON_IMPORT_ALL
      "${VTK_PYTHON_IMPORT_ALL}from .${module} import *\n")
    configure_file(vtk/module.py.in vtk/${module}.py @ONLY)
    list(APPEND VTK_PYTHON_FILES
      "${CMAKE_CURRENT_BINARY_DIR}/vtk/${module}.py")
  endforeach()
  configure_file(vtk/__init__.py.in vtk/__init__.py @ONLY)
  list(APPEND VTK_PYTHON_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/vtk/__init__.py")

# Kit module adapters
  foreach(kit IN LISTS vtk_kits)
    set(_module_kit ${kit}${VTK_KIT_SUFFIX})
    foreach(dep IN LISTS _${kit}_modules)
      configure_file(vtk/kit_module__init__.py.in vtk/${dep}.py @ONLY)
      list(APPEND VTK_PYTHON_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/vtk/${dep}.py")
    endforeach()
  endforeach()
  unset(_module_kit)

# vtk.util package
  list(APPEND VTK_PYTHON_FILES
    vtk/util/__init__
    vtk/util/_argparse
    vtk/util/vtkMethodParser
    vtk/util/misc
    vtk/util/vtkConstants
    vtk/util/vtkImageExportToArray
    vtk/util/vtkImageImportFromArray
    vtk/util/vtkVariant
    vtk/util/colors
    vtk/util/numpy_support
    vtk/util/vtkAlgorithm
    vtk/util/keys
    )

# vtk.test package
  list(APPEND VTK_PYTHON_FILES
    vtk/test/BlackBox
    vtk/test/Testing
    vtk/test/__init__
    )

# Tk related files
  list(APPEND VTK_PYTHON_FILES
    vtk/tk/__init__
    vtk/tk/vtkLoadPythonTkWidgets
    vtk/tk/vtkTkImageViewerWidget
    vtk/tk/vtkTkRenderWidget
    vtk/tk/vtkTkRenderWindowInteractor
    vtk/tk/vtkTkPhotoImage
    )

# PyQt related files
  list(APPEND VTK_PYTHON_FILES
    vtk/qt/__init__
    vtk/qt/QVTKRenderWindowInteractor
    vtk/qt4/__init__
    vtk/qt4/QVTKRenderWindowInteractor
    )

# wxPython related files
  list(APPEND VTK_PYTHON_FILES
    vtk/wx/__init__
    vtk/wx/wxVTKRenderWindow
    vtk/wx/wxVTKRenderWindowInteractor
    )

# pyGTK related files
  list(APPEND VTK_PYTHON_FILES
    vtk/gtk/GtkVTKRenderWindow
    vtk/gtk/__init__
    vtk/gtk/GtkVTKRenderWindowInteractor
    vtk/gtk/GtkGLExtVTKRenderWindow
    vtk/gtk/GtkGLExtVTKRenderWindowInteractor
    )

# vtk.numpy_interface related files
  list(APPEND VTK_PYTHON_FILES
    vtk/numpy_interface/__init__
    vtk/numpy_interface/algorithms
    vtk/numpy_interface/dataset_adapter
    vtk/numpy_interface/internal_algorithms
    )
# Done listing of files.

# Now copy these files if necessary.
  unset(VTK_PYTHON_SOURCE_FILES)
  unset(VTK_PYTHON_OUTPUT_FILES)
  foreach(file ${VTK_PYTHON_FILES})
    if (NOT IS_ABSOLUTE "${file}")
      set(src "${CMAKE_CURRENT_BINARY_DIR}/${file}.py")
      list(APPEND VTK_PYTHON_SOURCE_FILES "${src}")
    endif ()
  endforeach()

  if(NOT "${VTK_BINARY_DIR}" STREQUAL "${VTK_SOURCE_DIR}")
    foreach(file ${VTK_PYTHON_FILES})
      if (NOT IS_ABSOLUTE "${file}")
        set(src "${CMAKE_CURRENT_SOURCE_DIR}/${file}.py")
        set(tgt "${CMAKE_CURRENT_BINARY_DIR}/${file}.py")
        add_custom_command(DEPENDS ${src}
          COMMAND ${CMAKE_COMMAND} -E copy ${src} ${tgt}
          OUTPUT ${tgt}
          COMMENT "source copy")
      endif ()
    endforeach()
  endif()

  # Byte compile the Python files.
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/compile_all_vtk.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    @ONLY)
  add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    DEPENDS ${VTK_PYTHON_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vtk_compile_complete"
    )

  # Create a target on which custom commands can depend.  When this
  # is up-to-date, it is safe to run VTK python code.  Therefore
  # this should also depend on the vtkpython executable.
  add_custom_target(vtkpython_pyc ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/vtk_compile_complete")
  add_dependencies(vtkpython_pyc
    vtkpython
    )

  # If no runtime is to be installed then do not install python modules.
  if(NOT VTK_INSTALL_NO_RUNTIME)

    # Install python modules
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vtk"
      DESTINATION ${VTK_INSTALL_PYTHON_MODULE_DIR} COMPONENT RuntimeLibraries
      USE_SOURCE_PERMISSIONS)

    # Install python extension library that backs the modules
    if (BUILD_SHARED_LIBS AND NOT VTK_INSTALL_NO_LIBRARIES)
      foreach(module IN LISTS VTK_PYTHON_MODULES_AND_KITS)
        install(TARGETS ${module}Python
          RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
          LIBRARY DESTINATION ${VTK_INSTALL_PYTHON_MODULE_DIR}/vtk COMPONENT RuntimeLibraries
          ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
          )
      endforeach()
    endif()

    # Install the conveniently configured python interpretters
    if(NOT VTK_INSTALL_NO_PYTHON_EXES AND VTK_ENABLE_VTKPYTHON)
      # Install the vtkpython executable
      install(TARGETS vtkpython
        DESTINATION ${VTK_INSTALL_RUNTIME_DIR})

      if(PVTKPYTHON_EXECUTABLE)
        # Install the mpi enabled vtkpython executable
        install(TARGETS pvtkpython
          DESTINATION ${VTK_INSTALL_RUNTIME_DIR})
      endif()
    endif()

  endif()

endif()
