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
cmake_minimum_required(VERSION 2.8)

#ensure we link against our dependencies
include(module.cmake)

find_package(Dax REQUIRED)

# Setting the configuration file to include the appropriate driver.
if(${Dax_ENABLE_CUDA})
message ("CUDA_ENABLED")
elsif(${Dax_ENABLE_TBB})
message ("TBB_ENABLED")
elsif(${Dax_ENABLE_OPENMP})
message ("OPENMP_ENABLED")
else()
message ("SERIAL_ENABLED")
endif()



#needed for our export macros
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${Dax_INCLUDE_DIRS})
include_directories(${${vtk-module}_DEPENDS_INCLUDE_DIRS})

set(headers
  daxToVtk/CellTypeToType.h
  daxToVtk/DataSetConverters.h
  vtkToDax/Allocators.h
  vtkToDax/CellTypeAndDataType.h
  vtkToDax/CellTypeToType.h
  vtkToDax/Containers.h
  vtkToDax/DataSetConverters.h
  vtkToDax/DataSetTypeToType.h
  vtkToDax/FieldTypeToType.h
  vtkToDax/MarchingCubes.h
  vtkToDax/Portals.h
  vtkToDax/Threshold.h
  )
set(Module_SRCS
  DaxObjectFactory.h
  #vtkDaxMarchingCubes.cxx
  #DaxMarchingCubes.cxx
  vtkDaxThreshold.cxx
  DaxThreshold.cxx
  ${headers}
  )

set_source_files_properties(
  ${headers}
  WRAP_EXCLUDE
  )

vtk_module_library(vtkAcceleratorsDax ${Module_SRCS})
