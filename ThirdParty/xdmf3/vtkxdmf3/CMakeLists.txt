project(Xdmf)
cmake_minimum_required(VERSION 2.6)

#Needed to build DSM because relative paths are used to add its libraries
if (POLICY CMP0015)
	cmake_policy(SET CMP0015 NEW)
endif (POLICY CMP0015)

if(MSVC10)
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
	SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
	SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
endif(MSVC10)

set(BUILD_SHARED_LIBS true)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/CMake
  ${CMAKE_SOURCE_DIR}/CMake/TestingSuite
  ${CMAKE_SOURCE_DIR}/CMake/VersionSuite)

include(XdmfFunctions)

# RPath
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Should we build with documentation
option(XDMF_BUILD_DOCUMENTATION OFF)
if(XDMF_BUILD_DOCUMENTATION)
  add_subdirectory(doc)
endif(XDMF_BUILD_DOCUMENTATION)

# Enable CMake testing
option(BUILD_TESTING "Build Tests" OFF)
if(BUILD_TESTING)
  enable_testing()
  include(CTest)
endif(BUILD_TESTING)

# Wrapper Setup
option(XDMF_WRAP_PYTHON OFF)
option(XDMF_WRAP_JAVA OFF)

# Test for DSM
option(XDMF_BUILD_DSM OFF)

if(XDMF_BUILD_DSM)
  option(XDMF_BUILD_DSM_THREADS ON)
  if (XDMF_WRAP_PYTHON)
    mark_as_advanced(FORCE PYTHON_INCLUDE_MPI4PY_DIR)
    set(PYTHON_INCLUDE_MPI4PY_DIR "" CACHE STRING "Directory containing mpi4py/mpi4py.i")
  endif (XDMF_WRAP_PYTHON)
endif(XDMF_BUILD_DSM)

# If we are wrapping either, we need swig
if(XDMF_WRAP_PYTHON OR XDMF_WRAP_JAVA)
  find_package(SWIG REQUIRED)
  include(${SWIG_USE_FILE})
  if(${SWIG_VERSION} LESS 2.0.0)
    message(SEND_ERROR "Swig must be version 2.0.0 or greater")
  endif(${SWIG_VERSION} LESS 2.0.0)
endif(XDMF_WRAP_PYTHON OR XDMF_WRAP_JAVA)

# If we are wrapping python, let's include it in the top level
if(XDMF_WRAP_PYTHON)
  find_package(PythonInterp REQUIRED)
  find_package(PythonLibs REQUIRED)
  if (XDMF_BUILD_DSM)
    include_directories(${PYTHON_INCLUDE_DIRS} ${MPI_CXX_INCLUDE_PATH} ${PYTHON_INCLUDE_MPI4PY_DIR})
  else (XDMF_BUILD_DSM)
    include_directories(${PYTHON_INCLUDE_DIRS})
  endif (XDMF_BUILD_DSM)

  # A macro to swig and create the python files
  # Since we essentually do the same thing inside and outside core, it
  # would be easier as a macro
  # Parameters:
  #     python_name = the name of the intended target to be wrapped
  #     ${ARGN} = any dependencies needed by the target
  # Output:
  #     Target Name = the output target name will have ${python_name} as its name
  macro(XDMF_SWIG_PYTHON python_name)
    set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR})
    # optimize swig generation --- these are all flags corresponding to -O
    # except -fvirtual which breaks visitor operation
    set(CMAKE_SWIG_FLAGS ${XDMF_SWIG_FLAGS} -modern -fastdispatch -nosafecstrings -noproxydel -fastproxy -fastinit -fastunpack -fastquery -modernargs -nobuildnone)
    #Enables DSM
    if (XDMF_BUILD_DSM)
      set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} -DXDMF_BUILD_DSM)
    endif (XDMF_BUILD_DSM)
    set_source_files_properties(${python_name}.i PROPERTIES CPLUSPLUS ON)
    set(swig_extra_generated_files "")
    swig_add_module(${python_name} python ${python_name}.i)
    swig_link_libraries(${python_name} ${python_name} ${PYTHON_LIBRARIES})
    set_property(DIRECTORY APPEND PROPERTY
      ADDITIONAL_MAKE_CLEAN_FILES
      ${CMAKE_BINARY_DIR}/${python_name}.pyc
    )

    if("${ARGN}" STRGREATER "")
      add_dependencies(${python_name} "${ARGN}")
    endif("${ARGN}" STRGREATER "")

    if(WIN32)
	  if(NOT MSVC10)
        set_target_properties(${SWIG_MODULE_${python_name}_REAL_NAME}
          PROPERTIES
          PREFIX ../
          IMPORT_PREFIX ../
          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
          ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
	  endif(NOT MSVC10)
    endif(WIN32)

    string(TOUPPER ${python_name} python_name_upper)
    set(${python_name_upper}_PYTHON
      ${CMAKE_INSTALL_PREFIX}/lib/python/${python_name}.py)
    get_directory_property(${python_name}Parent PARENT_DIRECTORY)
    if(NOT "${${python_name}Parent}" STREQUAL "")
      set(${python_name_upper}_PYTHON ${${python_name_upper}_PYTHON}
        PARENT_SCOPE)
    endif(NOT "${${python_name}Parent}" STREQUAL "")

    install(FILES ${CMAKE_BINARY_DIR}/${python_name}.py
      DESTINATION lib/python)
    install(TARGETS ${SWIG_MODULE_${python_name}_REAL_NAME}
      DESTINATION lib/python)
  endmacro(XDMF_SWIG_PYTHON)
endif(XDMF_WRAP_PYTHON)

# If we are wrapping java, let's include it in the top level
if(XDMF_WRAP_JAVA)
  find_package(Java REQUIRED)
  find_package(JNI REQUIRED)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ${JAVA_INCLUDE_PATH}
    ${JAVA_INCLUDE_PATH2})

  # A macro to swig and create the jar files
  # Since we essentually do the same thing inside and outside core, it
  # would be easier as a macro
  # Parameters:
  #     java_name = the name of the intended target to be wrapped
  #     ${ARGN} = any dependencies needed by the target
  # Output:
  #     Target Name     = the output target name will have ${java_name}Java as its name
  #     Target Jar      = the output target jar will have ${java_name}Java.jar as its name
  macro(XDMF_SWIG_JAVA java_name)
    set(${java_name}_JAVA_JAR ${CMAKE_BINARY_DIR}/${java_name}.jar)
    set(XDMF_JAVA_PACKAGE_DIR mil/army/arl/xdmf)
    set(XDMF_JAVA_DIR ${CMAKE_CURRENT_BINARY_DIR}/${XDMF_JAVA_PACKAGE_DIR})
    set(CMAKE_SWIG_OUTDIR ${XDMF_JAVA_DIR})
    set(CMAKE_SWIG_FLAGS ${XDMF_SWIG_FLAGS} -v -make_default -package mil.army.arl.xdmf)
    set_source_files_properties(${java_name}.i PROPERTIES CPLUSPLUS ON)
    set(swig_extra_generated_files "")
    swig_add_module(${java_name}Java java ${java_name}.i)
    swig_link_libraries(${java_name}Java ${java_name})

    if(WIN32)
	  if(NOT MSVC10)
        set_target_properties(${SWIG_MODULE_${java_name}Java_REAL_NAME}
          PROPERTIES
          PREFIX ../
          IMPORT_PREFIX ../
          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
          LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
          ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
	  endif(NOT MSVC10)
      set(java_files *.java)
      set(java_dest "bin")
      set(separator "\;")
    else(WIN32)
      set(java_files ${XDMF_JAVA_DIR}/*.java)
      set(java_dest "lib/java")
      set(separator ":")
    endif(WIN32)

    add_custom_target(${java_name}_Java_Dir ALL DEPENDS ${XDMF_JAVA_DIR})
    add_custom_command(OUTPUT ${XDMF_JAVA_DIR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${XDMF_JAVA_DIR})
    add_custom_target(Compiled_${java_name}_Jar ALL DEPENDS
      ${${java_name}_JAVA_JAR})
    add_custom_command(OUTPUT ${${java_name}_JAVA_JAR}
      COMMAND ${CMAKE_COMMAND} -E chdir ${XDMF_JAVA_PACKAGE_DIR}
              ${JAVA_COMPILE} -cp \".${java_cp_jars}\" ${java_files}
      COMMAND ${JAVA_ARCHIVE} -cvf ${${java_name}_JAVA_JAR}
             "${XDMF_JAVA_PACKAGE_DIR}/*.class")
    add_dependencies(${java_name}Java ${java_name}_Java_Dir)
    add_dependencies(Compiled_${java_name}_Jar ${java_name}Java)

    if(NOT ${ARGN} EQUAL "")
      add_dependencies(Compiled_${java_name}_Jar "${ARGN}")
    endif(NOT ${ARGN} EQUAL "")

    set_property(DIRECTORY APPEND PROPERTY
      ADDITIONAL_MAKE_CLEAN_FILES
      ${CMAKE_BINARY_DIR}/${java_name}.jar
      ${XDMF_JAVA_DIR})

    set(java_cp_jars "${java_cp_jars}${separator}${${java_name}_JAVA_JAR}")
    string(TOUPPER ${java_name} java_name_upper)
    set(${java_name_upper}_JAVA_JAR
      ${CMAKE_INSTALL_PREFIX}/lib/java/${java_name}.jar)
    get_directory_property(${java_name}Parent PARENT_DIRECTORY)
    if(NOT "${${java_name}Parent}" STREQUAL "")
      set(${java_name_upper}_JAVA_JAR ${${java_name_upper}_JAVA_JAR}
        PARENT_SCOPE)
      set(java_cp_jars "${java_cp_jars}" PARENT_SCOPE)
    endif(NOT "${${java_name}Parent}" STREQUAL "")

    install(FILES ${CMAKE_BINARY_DIR}/${java_name}.jar
      DESTINATION lib/java)
    install(TARGETS ${SWIG_MODULE_${java_name}Java_REAL_NAME}
      DESTINATION ${java_dest})
  endmacro(XDMF_SWIG_JAVA)
endif(XDMF_WRAP_JAVA)

add_subdirectory(core)
include_directories(${XdmfCore_INCLUDE_DIRS} ${XdmfDSM_INCLUDE_DIRS})
link_directories(${XDMF_LIBRARY_DIRS})

option(XDMF_BUILD_CORE_ONLY OFF)
if(NOT XDMF_BUILD_CORE_ONLY)
  set(XdmfSources
    XdmfAttribute
    XdmfAttributeCenter
    XdmfAttributeType
    XdmfCurvilinearGrid
    XdmfDomain
    XdmfGeometry
    XdmfGeometryType
    XdmfGraph
    XdmfGrid
    XdmfGridCollection
    XdmfGridCollectionType
    XdmfItemFactory
    XdmfMap
    XdmfReader
    XdmfRectilinearGrid
    XdmfRegularGrid
    XdmfSet
    XdmfSetType
    XdmfTime
    XdmfTopology
    XdmfTopologyType
    XdmfUnstructuredGrid)

  add_library(Xdmf ${XdmfSources})
  if(XDMF_BUILD_DSM)
    target_link_libraries(Xdmf XdmfCore XdmfDSM)
  else(XDMF_BUILD_DSM)
    target_link_libraries(Xdmf XdmfCore)
  endif(XDMF_BUILD_DSM)

  if(WIN32)
	set_target_properties(Xdmf PROPERTIES
      DEFINE_SYMBOL Xdmf_EXPORTS)
    if(NOT MSVC10)
	  set_target_properties(Xdmf PROPERTIES
        PREFIX ../
        IMPORT_PREFIX ../
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
	endif(NOT MSVC10)
   endif(WIN32)

  if(XDMF_WRAP_JAVA)
    XDMF_SWIG_JAVA(Xdmf Compiled_XdmfCore_Jar)
  endif(XDMF_WRAP_JAVA)

  if(XDMF_WRAP_PYTHON)
    XDMF_SWIG_PYTHON(Xdmf XdmfCore XdmfDSM)
  endif(XDMF_WRAP_PYTHON)

  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif(BUILD_TESTING)

  option(XDMF_BUILD_UTILS OFF)
  if(XDMF_BUILD_UTILS)
    add_subdirectory(utils)
  endif(XDMF_BUILD_UTILS)

  if(WIN32)
    set(XDMF_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/Xdmf.lib)
  endif(WIN32)
  
  if(UNIX)
    set(XDMF_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libXdmf.so)
  endif(UNIX)

  if(APPLE)
    set(XDMF_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libXdmf.dylib)
  endif(APPLE)
  
  file(GLOB XdmfHeaders 
    "*.hpp" 
    "*.tpp" 
    "*.i" 
    "CMake/VersionSuite/*.hpp"
    "${CMAKE_BINARY_DIR}/*.hpp"
  )
  install(FILES ${XdmfHeaders} DESTINATION include)
  install(TARGETS Xdmf
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
endif(NOT XDMF_BUILD_CORE_ONLY)

xdmf_create_config_file(${PROJECT_NAME})
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  DESTINATION ${CMAKE_INSTALL_PREFIX})
