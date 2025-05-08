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
  ArrayCopyUnknown.h
  ArrayHandleExecutionManager.h
  ArrayPortalFromIterators.h
  ArrayRangeComputeUtils.h
  ArrayTransfer.h
  Buffer.h
  CastInvalidValue.h
  ConnectivityExplicitInternals.h
  ConvertNumComponentsToOffsetsTemplate.h
  DeviceAdapterAlgorithmGeneral.h
  DeviceAdapterMemoryManager.h
  DeviceAdapterMemoryManagerShared.h
  DeviceAdapterListHelpers.h
  FieldCollection.h
  FunctorsGeneral.h
  Hints.h
  IteratorFromArrayPortal.h
  KXSort.h
  MapArrayPermutation.h
  OptionParser.h
  OptionParserArguments.h
  ParallelRadixSort.h
  ParallelRadixSortInterface.h
  ReverseConnectivityBuilder.h
  RuntimeDeviceConfiguration.h
  RuntimeDeviceConfigurationOptions.h
  RuntimeDeviceOption.h
  StorageError.h
  )

viskores_declare_headers(${headers})

# These source files are actually compiled in the parent directory.
# They are in a separate directory to highlight which objects are
# internal and which are part of the external interface.
#add_custom_target(viskoresContInternal ALL DEPENDS viskoresCont)
