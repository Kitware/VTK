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

#-----------------------------------------------------------------------------
# Build the configure file.
# need to set numerous Viskores cmake properties to the naming convention
# that we expect for our C++ defines.

set(VISKORES_NO_ASSERT ${Viskores_NO_ASSERT})
set(VISKORES_NO_ASSERT_CUDA ${Viskores_NO_ASSERT_CUDA})
set(VISKORES_NO_ASSERT_HIP ${Viskores_NO_ASSERT_HIP})
set(VISKORES_USE_DOUBLE_PRECISION ${Viskores_USE_DOUBLE_PRECISION})
set(VISKORES_USE_64BIT_IDS ${Viskores_USE_64BIT_IDS})

set(VISKORES_ENABLE_CUDA ${Viskores_ENABLE_CUDA})
set(VISKORES_ENABLE_KOKKOS ${Viskores_ENABLE_KOKKOS})
set(VISKORES_ENABLE_KOKKOS_THRUST ${Viskores_ENABLE_KOKKOS_THRUST})
set(VISKORES_ENABLE_OPENMP ${Viskores_ENABLE_OPENMP})
set(VISKORES_ENABLE_TBB ${Viskores_ENABLE_TBB})

set(VISKORES_ENABLE_MPI ${Viskores_ENABLE_MPI})
set(VISKORES_ENABLE_GPU_MPI ${Viskores_ENABLE_GPU_MPI})

if(VISKORES_ENABLE_CUDA)
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1" VISKORES_CUDA_VERSION_MAJOR ${CMAKE_CUDA_COMPILER_VERSION})
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" VISKORES_CUDA_VERSION_MINOR ${CMAKE_CUDA_COMPILER_VERSION})
endif()

if (TARGET viskores_kokkos_cuda)
  set(VISKORES_KOKKOS_CUDA ON)
elseif(TARGET viskores_kokkos_hip)
  set(VISKORES_KOKKOS_HIP ON)
endif()

set(VISKORES_ENABLE_LOGGING ${Viskores_ENABLE_LOGGING})

viskores_get_kit_name(kit_name kit_dir)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  @ONLY)
viskores_install_headers(viskores/internal
  ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  )


set(headers
  ArrayPortalBasic.h
  ArrayPortalDummy.h
  ArrayPortalHelpers.h
  ArrayPortalUniformPointCoordinates.h
  ArrayPortalValueReference.h
  Assume.h
  ConfigureFor32.h
  ConfigureFor64.h
  ConnectivityStructuredInternals.h
  DecayHelpers.h
  ExportMacros.h
  FunctionInterface.h
  FunctionInterfaceDetailPost.h
  FunctionInterfaceDetailPre.h
  IndexTag.h
  IndicesExtrude.h
  Instantiations.h
  Invocation.h
  Meta.h
  Unreachable.h
  VariantImpl.h
  VariantImplDetail.h
  VecOperators.h
  Windows.h
  )

viskores_declare_headers(${headers})

viskores_pyexpander_generated_file(FunctionInterfaceDetailPre.h)
viskores_pyexpander_generated_file(FunctionInterfaceDetailPost.h)
viskores_pyexpander_generated_file(VariantImplDetail.h)
viskores_pyexpander_generated_file(VecOperators.h)
