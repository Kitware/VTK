//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_DeviceAdapter_h
#define viskores_cont_DeviceAdapter_h

// These are listed in non-alphabetical order because this is the conceptual
// order in which the sub-files are loaded.  (But the compile should still
// succeed if the order is changed.)  Turn off formatting to keep the order.

// clang-format off
#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/cont/kokkos/DeviceAdapterKokkos.h>
#include <viskores/cont/openmp/DeviceAdapterOpenMP.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/cont/tbb/DeviceAdapterTBB.h>

#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>

// clang-format on

namespace viskores
{
namespace cont
{

#ifdef VISKORES_DOXYGEN_ONLY
/// \brief A tag specifying the interface between the control and execution environments.
///
/// A DeviceAdapter tag specifies a set of functions and classes that provide
/// mechanisms to run algorithms on a type of parallel device. The tag
/// DeviceAdapterTag___ does not actually exist. Rather, this documentation is
/// provided to describe the interface for a DeviceAdapter. Loading the
/// viskores/cont/DeviceAdapter.h header file will import all device adapters
/// appropriate for the current compile environment.
///
/// \li \c viskores::cont::DeviceAdapterTagSerial Runs all algorithms in serial. Can be
/// helpful for debugging.
/// \li \c viskores::cont::DeviceAdapterTagCuda Dispatches and runs algorithms on a GPU
/// using CUDA.  Must be compiling with a CUDA compiler (nvcc).
/// \li \c viskores::cont::DeviceAdapterTagKokkos Dispatches and runs algorithms using
/// the Kokkos library.
/// \li \c viskores::cont::DeviceAdapterTagOpenMP Dispatches an algorithm over multiple
/// CPU cores using OpenMP compiler directives.  Must be compiling with an
/// OpenMP-compliant compiler with OpenMP pragmas enabled.
/// \li \c viskores::cont::DeviceAdapterTagTBB Dispatches and runs algorithms on multiple
/// threads using the Intel Threading Building Blocks (TBB) libraries. Must
/// have the TBB headers available and the resulting code must be linked with
/// the TBB libraries.
///
/// To execute algorithms on any device, see Algorithm.h which allows
/// for abitrary device execution.
///
struct DeviceAdapterTag___
{
};
#endif //VISKORES_DOXYGEN_ONLY

namespace internal
{

} // namespace internal
}
} // namespace viskores::cont

#endif //viskores_cont_DeviceAdapter_h
