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

#ifndef viskores_filter_flow_internal_DataSetIntegrator_h
#define viskores_filter_flow_internal_DataSetIntegrator_h

#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/ParticleArrayCopy.h>
#include <viskores/filter/flow/FlowTypes.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/worklet/EulerIntegrator.h>
#include <viskores/filter/flow/worklet/IntegratorStatus.h>
#include <viskores/filter/flow/worklet/ParticleAdvection.h>
#include <viskores/filter/flow/worklet/RK4Integrator.h>
#include <viskores/filter/flow/worklet/Stepper.h>

#include <viskores/cont/Variant.h>

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class DSIHelperInfo
{
public:
  DSIHelperInfo(
    const std::vector<ParticleType>& v,
    const viskores::filter::flow::internal::BoundsMap& boundsMap,
    const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& particleBlockIDsMap)
    : BoundsMap(boundsMap)
    , ParticleBlockIDsMap(particleBlockIDsMap)
    , Particles(v)
  {
  }

  struct ParticleBlockIds
  {
    void Clear()
    {
      this->Particles.clear();
      this->BlockIDs.clear();
    }

    void Add(const ParticleType& p, const std::vector<viskores::Id>& bids)
    {
      this->Particles.emplace_back(p);
      this->BlockIDs[p.GetID()] = std::move(bids);
    }

    std::vector<ParticleType> Particles;
    std::unordered_map<viskores::Id, std::vector<viskores::Id>> BlockIDs;
  };

  void Clear()
  {
    this->InBounds.Clear();
    this->OutOfBounds.Clear();
    this->TermIdx.clear();
    this->TermID.clear();
  }

  void Validate(viskores::Id num)
  {
    //Make sure we didn't miss anything. Every particle goes into a single bucket.
    if ((static_cast<std::size_t>(num) !=
         (this->InBounds.Particles.size() + this->OutOfBounds.Particles.size() +
          this->TermIdx.size())) ||
        (this->InBounds.Particles.size() != this->InBounds.BlockIDs.size()) ||
        (this->OutOfBounds.Particles.size() != this->OutOfBounds.BlockIDs.size()) ||
        (this->TermIdx.size() != this->TermID.size()))
    {
      throw viskores::cont::ErrorFilterExecution("Particle count mismatch after classification");
    }
  }

  void AddTerminated(viskores::Id idx, viskores::Id pID)
  {
    this->TermIdx.emplace_back(idx);
    this->TermID.emplace_back(pID);
  }

  viskores::filter::flow::internal::BoundsMap BoundsMap;
  std::unordered_map<viskores::Id, std::vector<viskores::Id>> ParticleBlockIDsMap;

  ParticleBlockIds InBounds;
  ParticleBlockIds OutOfBounds;
  std::vector<ParticleType> Particles;
  std::vector<viskores::Id> TermID;
  std::vector<viskores::Id> TermIdx;
};

template <typename Derived, typename ParticleType>
class DataSetIntegrator
{
public:
  DataSetIntegrator(viskores::Id id, viskores::filter::flow::IntegrationSolverType solverType)
    : Id(id)
    , SolverType(solverType)
    , Rank(this->Comm.rank())
  {
    //check that things are valid.
  }

  VISKORES_CONT viskores::Id GetID() const { return this->Id; }
  VISKORES_CONT void SetCopySeedFlag(bool val) { this->CopySeedArray = val; }

  VISKORES_CONT
  void Advect(DSIHelperInfo<ParticleType>& b,
              viskores::FloatDefault stepSize) //move these to member data(?)
  {
    Derived* inst = static_cast<Derived*>(this);
    inst->DoAdvect(b, stepSize);
  }

  VISKORES_CONT bool GetOutput(viskores::cont::DataSet& dataset) const
  {
    Derived* inst = static_cast<Derived*>(this);
    return inst->GetOutput(dataset);
  }

protected:
  VISKORES_CONT inline void ClassifyParticles(
    const viskores::cont::ArrayHandle<ParticleType>& particles,
    DSIHelperInfo<ParticleType>& dsiInfo) const;

  //Data members.
  viskores::Id Id;
  viskores::filter::flow::IntegrationSolverType SolverType;
  viskoresdiy::mpi::communicator Comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::Id Rank;
  bool CopySeedArray = false;
};

template <typename Derived, typename ParticleType>
VISKORES_CONT inline void DataSetIntegrator<Derived, ParticleType>::ClassifyParticles(
  const viskores::cont::ArrayHandle<ParticleType>& particles,
  DSIHelperInfo<ParticleType>& dsiInfo) const
{
  /*
 each particle: --> T,I,A
 if T: update TermIdx, TermID
 if A: update IdMapA;
 if I: update IdMapI;
   */
  dsiInfo.Clear();

  auto portal = particles.WritePortal();
  viskores::Id n = portal.GetNumberOfValues();

  for (viskores::Id i = 0; i < n; i++)
  {
    auto p = portal.Get(i);

    //Terminated.
    if (p.GetStatus().CheckTerminate())
    {
      dsiInfo.AddTerminated(i, p.GetID());
    }
    else
    {
      //Didn't terminate.
      //Get the blockIDs.
      const auto& it = dsiInfo.ParticleBlockIDsMap.find(p.GetID());
      VISKORES_ASSERT(it != dsiInfo.ParticleBlockIDsMap.end());
      auto currBIDs = it->second;
      VISKORES_ASSERT(!currBIDs.empty());

      std::vector<viskores::Id> newIDs;
      if (p.GetStatus().CheckSpatialBounds() && !p.GetStatus().CheckTookAnySteps())
      {
        //particle is OUTSIDE but didn't take any steps.
        //this means that the particle wasn't in the block.
        //assign newIDs to currBIDs[1:]
        newIDs.assign(std::next(currBIDs.begin(), 1), currBIDs.end());
      }
      else
      {
        //Otherwise, get new blocks from the current position.
        newIDs = dsiInfo.BoundsMap.FindBlocks(p.GetPosition(), currBIDs);
      }

      //reset the particle status.
      p.GetStatus() = viskores::ParticleStatus();

      if (newIDs.empty()) //No blocks, we're done.
      {
        p.GetStatus().SetTerminate();
        dsiInfo.AddTerminated(i, p.GetID());
      }
      else
      {
        //If we have more than one blockId, we want to minimize communication
        //and put any blocks owned by this rank first.
        if (newIDs.size() > 1)
        {
          for (auto idit = newIDs.begin(); idit != newIDs.end(); idit++)
          {
            viskores::Id bid = *idit;
            auto ranks = dsiInfo.BoundsMap.FindRank(bid);
            if (std::find(ranks.begin(), ranks.end(), this->Rank) != ranks.end())
            {
              newIDs.erase(idit);
              newIDs.insert(newIDs.begin(), bid);
              break;
            }
          }
        }

        dsiInfo.OutOfBounds.Add(p, newIDs);
      }
    }
    portal.Set(i, p);
  }

  //Make sure everything is copacetic.
  dsiInfo.Validate(n);
}

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_DataSetIntegrator_h
