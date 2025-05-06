##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

find_package(anari 0.8.0 REQUIRED)

set(headers
  ANARIActor.h
  ANARIMapper.h
  ANARIMapperGlyphs.h
  ANARIMapperPoints.h
  ANARIMapperTriangles.h
  ANARIMapperVolume.h
  ANARIScene.h
  ViskoresANARITypes.h
  )

set(sources
  ANARIActor.cxx
  ANARIMapper.cxx
  ANARIScene.cxx
  ViskoresANARITypes.cxx
  )

set(device_sources
  ANARIMapperGlyphs.cxx
  ANARIMapperPoints.cxx
  ANARIMapperTriangles.cxx
  ANARIMapperVolume.cxx
  ../../rendering/raytracing/SphereExtractor.cxx
  )


viskores_library(
  NAME viskores_anari
  SOURCES ${sources}
  HEADERS ${headers}
  DEVICE_SOURCES ${device_sources}
  )

viskores_add_target_information(viskores_anari
  DROP_UNUSED_SYMBOLS
  MODIFY_CUDA_FLAGS
  DEVICE_SOURCES ANARIMapperVolume.cxx
  )

target_link_libraries(viskores_anari PUBLIC anari::anari)
