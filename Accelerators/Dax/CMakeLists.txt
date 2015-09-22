##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2012 Sandia Corporation.
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##=============================================================================

#ensure we link against our dependencies
include(module.cmake)

find_package(Dax REQUIRED)

DaxConfigureSerial(REQUIRED)
DaxConfigureCuda()
DaxConfigureTBB()
DaxConfigureOpenMP()

# Collect a list of available backends.
set(DAX_BACKENDS)
if(${Dax_ENABLE_CUDA})
  list(APPEND DAX_BACKENDS CUDA)
endif()
if(${Dax_ENABLE_TBB})
  list(APPEND DAX_BACKENDS TBB)
endif()
if(${Dax_ENABLE_OPENMP})
  list(APPEND DAX_BACKENDS OPENMP)
endif()
list(APPEND DAX_BACKENDS SERIAL)

# Pick the first backed found in list as the default backend
list(GET DAX_BACKENDS 0 DEFAULT_BACKEND)

# Display available options for user selection.
set(VTK_DAX_BACKEND ${DEFAULT_BACKEND} CACHE STRING
  "Choose from available backend acclerators")
set_property(CACHE VTK_DAX_BACKEND PROPERTY STRINGS ${DAX_BACKENDS})

# Turn the picked option on and turn off the others.
foreach(backend ${DAX_BACKENDS})
  if(${backend} STREQUAL ${VTK_DAX_BACKEND})
    set(VTK_DAX_PICKED_${backend} TRUE)
  else()
    set(VTK_DAX_PICKED_${backend} FALSE)
  endif()
endforeach()

# Setting the configuration file to include the appropriate driver.
#we prefer cuda over everything else, than tbb and openmp
set (DAX_DISABLE_BOOST_SP_THREADS 0)
if(${VTK_DAX_PICKED_CUDA})
  set (DAX_DISABLE_BOOST_SP_THREADS 1)
  set (DAX_DEVICE_ADAPTER "DAX_DEVICE_ADAPTER_CUDA")
elseif(${VTK_DAX_PICKED_TBB})
  set (DAX_DEVICE_ADAPTER "DAX_DEVICE_ADAPTER_TBB")
elseif(${VTK_DAX_PICKED_OPENMP})
  set (DAX_DEVICE_ADAPTER "DAX_DEVICE_ADAPTER_OPENMP")
else()
  set (DAX_DEVICE_ADAPTER "DAX_DEVICE_ADAPTER_SERIAL")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vtkDaxConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkDaxConfig.h" @ONLY)

#needed for our export macros
include_directories(${${vtk-module}_DEPENDS_INCLUDE_DIRS}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${Dax_INCLUDE_DIRS}
                   )

set(headers
  vtkDaxDetailCommon.h
  vtkDaxContourImpl.h
  vtkDaxThresholdImpl.h
  vtkDaxConfig.h
  )

#implementation of the algorithms for cpu accelerators
set(cpu_accelerator_srcs
  vtkDaxContourImpl.cxx
  vtkDaxThresholdImpl.cxx
  )

set(vtk_srcs
  vtkDaxContour.cxx
  vtkDaxThreshold.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkAcceleratorsDaxObjectFactory.cxx
  )

set_source_files_properties(
  ${headers}
  ${cpu_accelerator_srcs}
  ${CMAKE_CURRENT_BINARY_DIR}/vtkAcceleratorsDaxObjectFactory.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

# Now for the object factory.
set(opengl_overrides
  Contour
  Threshold
  )
# Now we need to generate the object factory for this module. Create a list of
# overrides, and then generate the class that overrides those classes.
set(vtk_module_overrides)
foreach(_override ${opengl_overrides})
  list(APPEND vtk_module_overrides vtk${_override})
  set(vtk_module_vtk${_override}_override vtkDax${_override})
endforeach()

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"rride for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  WRAP_EXCLUDE
  )

#we are building with CUDA support
if(${VTK_DAX_PICKED_CUDA})

  #implementation of the algorithms for gpu accelerators
  set(cuda_accelerator_srcs
    vtkDaxContourImpl.cu
    vtkDaxThresholdImpl.cu
    )


  #follow pistons example on how to build a subsection of the cuda with nvcc
  if(BUILD_SHARED_LIBS)
    set(GPGPU_BUILD_TYPE SHARED)
  endif()
  set(CUDA_ATTACH_VS_BUILD_RULE_TO_CUDA_FILE OFF) #otherwise C_SOURCES is empty in VS
  vtk_module_impl()

  cuda_compile(cuda_compiled_srcs
               ${cuda_accelerator_srcs}
               ${headers}
               ${GPGPU_BUILD_TYPE})

  set_source_files_properties(
    ${cuda_compiled_srcs}
    ${cuda_accelerator_srcs}
    WRAP_EXCLUDE
    )

  vtk_module_library(vtkAcceleratorsDax ${vtk_srcs} ${cuda_compiled_srcs}
                                        ${headers})
  vtk_module_link_libraries(vtkAcceleratorsDax LINK_PRIVATE ${CUDA_LIBRARIES})

elseif(${VTK_DAX_PICKED_TBB})
  vtk_module_library(vtkAcceleratorsDax ${vtk_srcs}
                                        ${cpu_accelerator_srcs} ${headers})
  vtk_module_link_libraries(vtkAcceleratorsDax LINK_PRIVATE ${TBB_LIBRARIES})
else()

  vtk_module_library(vtkAcceleratorsDax ${vtk_srcs}
                                        ${cpu_accelerator_srcs} ${headers})

endif()

#install the required headers to make your own dax vtk filter
if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkToDax
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development)
  install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/daxToVtk
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
    COMPONENT Development)
endif()
