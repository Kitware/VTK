# Default code to handle VTK module groups. The module.cmake files specify
# which groups the modules are in. We can specify some more specific
# documentation for groups in this file that will be displayed in cmake-gui and
# ccmake.

# The StandAlone group is a special group of all modules that need no
# external dependencies, such as Boost, MPI, etc. It does include
# modules that rely on third party libraries VTK can build (by
# default). It DOES NOT include modules that depend on OpenGL. Those
# modules are in the Rendering group.

set(VTK_Group_StandAlone_DOCS "Request building of all stand alone modules (no external dependencies required)")

foreach(group ${VTK_GROUPS})
  message(STATUS "Group ${group} modules: ${VTK_GROUP_${group}_MODULES}")
  # Set the default group option - Rendering ON, and all others OFF.
  if(${group} MATCHES "^Rendering|^StandAlone")
    set(_default ON)
  else()
    set(_default OFF)
  endif()
  # Is there any custom documentation for the group?
  if(VTK_Group_${group}_DOCS)
    set(_group_docs ${VTK_Group_${group}_DOCS})
  else()
    set(_group_docs "Request building ${group} modules")
  endif()
  option(VTK_Group_${group} "${_group_docs}" ${_default})
  # Now iterate through the modules, and request those that are depended on.
  if(VTK_Group_${group})
    foreach(module ${VTK_GROUP_${group}_MODULES})
      list(APPEND ${module}_REQUEST_BY VTK_Group_${group})
    endforeach()
  endif()
  # Hide the group options if building all modules.
  if(VTK_BUILD_ALL_MODULES)
    set_property(CACHE VTK_Group_${group} PROPERTY TYPE INTERNAL)
  else()
    set_property(CACHE VTK_Group_${group} PROPERTY TYPE BOOL)
  endif()
endforeach()
