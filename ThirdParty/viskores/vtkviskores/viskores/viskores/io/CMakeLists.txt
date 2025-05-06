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
  BOVDataSetReader.h
  DecodePNG.h
  EncodePNG.h
  ErrorIO.h
  FileUtils.h
  ImageReaderBase.h
  ImageReaderPNG.h
  ImageReaderPNM.h
  ImageUtils.h
  ImageWriterBase.h
  ImageWriterPNG.h
  ImageWriterPNM.h
  PixelTypes.h
  VTKDataSetReader.h
  VTKDataSetReaderBase.h
  VTKDataSetWriter.h
  VTKPolyDataReader.h
  VTKRectilinearGridReader.h
  VTKStructuredGridReader.h
  VTKStructuredPointsReader.h
  VTKUnstructuredGridReader.h
  VTKVisItFileReader.h
  )

set(template_sources
  PixelTypes.hxx
  )

set(sources
  BOVDataSetReader.cxx
  DecodePNG.cxx
  EncodePNG.cxx
  FileUtils.cxx
  ImageReaderBase.cxx
  ImageReaderPNG.cxx
  ImageReaderPNM.cxx
  ImageUtils.cxx
  ImageWriterBase.cxx
  ImageWriterPNG.cxx
  ImageWriterPNM.cxx
  PixelTypes.cxx
  VTKDataSetReader.cxx
  VTKDataSetReaderBase.cxx
  VTKDataSetWriter.cxx
  VTKPolyDataReader.cxx
  VTKRectilinearGridReader.cxx
  VTKStructuredGridReader.cxx
  VTKStructuredPointsReader.cxx
  VTKUnstructuredGridReader.cxx
  VTKVisItFileReader.cxx
  )

if (Viskores_ENABLE_HDF5_IO)
  set(headers
    ${headers}
    ImageReaderHDF5.h
    ImageWriterHDF5.h)
  set(sources
    ${sources}
    ImageReaderHDF5.cxx
    ImageWriterHDF5.cxx)
endif ()

viskores_declare_headers(
  ${headers}
  ${template_sources}
)

viskores_library(
  NAME viskores_io
  SOURCES ${sources}
  DEVICE_SOURCES ${device_sources}
  HEADERS ${headers}
  TEMPLATE_SOURCES ${template_sources}
)

if (Viskores_ENABLE_HDF5_IO)
  target_include_directories(viskores_io PRIVATE $<BUILD_INTERFACE:${HDF5_INCLUDE_DIR}>)
  target_link_libraries(viskores_io PRIVATE ${HDF5_HL_LIBRARIES} ${HDF5_LIBRARIES})
  if (Viskores_HDF5_IS_PARALLEL)
    target_link_libraries(viskores_io PRIVATE MPI::MPI_CXX)
  endif()
endif()

add_subdirectory(internal)
