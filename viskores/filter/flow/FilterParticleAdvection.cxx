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


#include <viskores/Particle.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/filter/flow/FilterParticleAdvection.h>

#include <viskores/thirdparty/diy/diy.h>

namespace viskores
{
namespace filter
{
namespace flow
{

VISKORES_CONT viskores::cont::DataSet FilterParticleAdvection::DoExecute(
  const viskores::cont::DataSet& inData)
{
  auto out = this->DoExecutePartitions(inData);
  if (out.GetNumberOfPartitions() != 1)
    throw viskores::cont::ErrorFilterExecution("Wrong number of results");

  return out.GetPartition(0);
}

VISKORES_CONT void FilterParticleAdvection::ValidateOptions() const
{
  if (this->GetUseCoordinateSystemAsField())
    throw viskores::cont::ErrorFilterExecution("Coordinate system as field not supported");

  viskores::Id numSeeds = this->Seeds.GetNumberOfValues();
#ifdef VISKORES_ENABLE_MPI
  viskoresdiy::mpi::communicator comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::Id totalNumSeeds = 0;
  viskoresdiy::mpi::all_reduce(comm, numSeeds, totalNumSeeds, std::plus<viskores::Id>{});
  numSeeds = totalNumSeeds;
#endif
  if (numSeeds == 0)
    throw viskores::cont::ErrorFilterExecution("No seeds provided.");
  if (!this->Seeds.IsBaseComponentType<viskores::Particle>() &&
      !this->Seeds.IsBaseComponentType<viskores::ChargedParticle>())
    throw viskores::cont::ErrorFilterExecution("Unsupported particle type in seed array.");
  if (this->NumberOfSteps == 0)
    throw viskores::cont::ErrorFilterExecution("Number of steps not specified.");
  if (this->StepSize == 0)
    throw viskores::cont::ErrorFilterExecution("Step size not specified.");
  if (this->NumberOfSteps < 0)
    throw viskores::cont::ErrorFilterExecution("NumberOfSteps cannot be negative");
  if (this->StepSize < 0)
    throw viskores::cont::ErrorFilterExecution("StepSize cannot be negative");
}

}
}
} // namespace viskores::filter::flow
