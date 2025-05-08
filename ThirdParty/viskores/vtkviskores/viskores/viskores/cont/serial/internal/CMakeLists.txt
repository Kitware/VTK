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
  DeviceAdapterAlgorithmSerial.h
  DeviceAdapterMemoryManagerSerial.h
  DeviceAdapterRuntimeDetectorSerial.h
  DeviceAdapterTagSerial.h
  RuntimeDeviceConfigurationSerial.h
  )

viskores_declare_headers(${headers})

target_sources(viskores_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmSerial.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorSerial.cxx
)
