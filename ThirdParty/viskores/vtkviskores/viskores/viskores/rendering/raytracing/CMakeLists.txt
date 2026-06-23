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

set(headers
  BoundingVolumeHierarchy.h
  BVHTraverser.h
  Camera.h
  CellIntersector.h
  CellSampler.h
  CellTables.h
  ChannelBuffer.h
  ChannelBufferOperations.h
  ConnectivityTracer.h
  CylinderExtractor.h
  CylinderIntersector.h
  GlyphExtractor.h
  GlyphExtractorVector.h
  GlyphIntersector.h
  GlyphIntersectorVector.h
  Logger.h
  MeshConnectivityBuilder.h
  MeshConnectivityContainers.h
  MeshConnectivity.h
  MortonCodes.h
  PartialComposite.h
  QuadExtractor.h
  QuadIntersector.h
  Ray.h
  RayOperations.h
  RayTracer.h
  RayTracingTypeDefs.h
  RunTriangulator.h
  Sampler.h
  ScalarRenderer.h
  ShapeIntersector.h
  SphereExtractor.h
  SphereIntersector.h
  TriangleExtractor.h
  TriangleIntersections.h
  TriangleIntersector.h
  Triangulator.h
  VolumeRendererStructured.h
  Worklets.h
  )

set(sources
  Logger.cxx
  MeshConnectivityContainers.cxx
  TriangleExtractor.cxx
  )

set(device_sources
  BoundingVolumeHierarchy.cxx
  Camera.cxx
  ChannelBuffer.cxx
  ConnectivityTracer.cxx
  CylinderExtractor.cxx
  CylinderIntersector.cxx
  GlyphExtractor.cxx
  GlyphExtractorVector.cxx
  GlyphIntersector.cxx
  GlyphIntersectorVector.cxx
  MeshConnectivityBuilder.cxx
  QuadExtractor.cxx
  QuadIntersector.cxx
  RayOperations.cxx
  RayTracer.cxx
  RunTriangulator.cxx
  ScalarRenderer.cxx
  ShapeIntersector.cxx
  SphereExtractor.cxx
  SphereIntersector.cxx
  TriangleIntersector.cxx
  VolumeRendererStructured.cxx
  )

set_source_files_properties(CellTables.h
                            PROPERTIES Viskores_CANT_BE_HEADER_TESTED TRUE)

viskores_library(
  NAME viskores_rendering_raytracing
  SOURCES ${sources}
  HEADERS ${headers}
  DEVICE_SOURCES ${device_sources}
  )
