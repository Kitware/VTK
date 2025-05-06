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
#include <viskores/cont/EnvironmentTracker.h>

#include <viskores/thirdparty/diy/diy.h>

#include <memory>

namespace viskores
{
namespace cont
{
namespace internal
{
static std::unique_ptr<viskoresdiy::mpi::communicator> GlobalCommuncator;
}

void EnvironmentTracker::SetCommunicator(const viskoresdiy::mpi::communicator& comm)
{
  if (!internal::GlobalCommuncator)
  {
    internal::GlobalCommuncator.reset(new viskoresdiy::mpi::communicator(comm));
  }
  else
  {
    *internal::GlobalCommuncator = comm;
  }
}

const viskoresdiy::mpi::communicator& EnvironmentTracker::GetCommunicator()
{
  if (!internal::GlobalCommuncator)
  {
    internal::GlobalCommuncator.reset(new viskoresdiy::mpi::communicator());
  }
  return *internal::GlobalCommuncator;
}
} // namespace viskores::cont
} // namespace viskores
