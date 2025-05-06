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

#ifndef viskores_filter_flow_internal_AdvectAlgorithm_h
#define viskores_filter_flow_internal_AdvectAlgorithm_h


#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/flow/internal/BoundsMap.h>
#include <viskores/filter/flow/internal/DataSetIntegrator.h>
#ifdef VISKORES_ENABLE_MPI
#include <viskores/filter/flow/internal/AdvectAlgorithmTerminator.h>
#include <viskores/filter/flow/internal/ParticleExchanger.h>
#include <viskores/thirdparty/diy/diy.h>
#include <viskores/thirdparty/diy/mpi-cast.h>
#endif

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename DSIType>
class AdvectAlgorithm
{
public:
  using ParticleType = typename DSIType::PType;

  AdvectAlgorithm(const viskores::filter::flow::internal::BoundsMap& bm,
                  std::vector<DSIType>& blocks)
    : Blocks(blocks)
    , BoundsMap(bm)
#ifdef VISKORES_ENABLE_MPI
    , Exchanger(this->Comm)
    , Terminator(this->Comm)
#endif
    , NumRanks(this->Comm.size())
    , Rank(this->Comm.rank())
  {
  }

  void Execute(const viskores::cont::ArrayHandle<ParticleType>& seeds,
               viskores::FloatDefault stepSize)
  {
    this->SetStepSize(stepSize);
    this->SetSeeds(seeds);

    this->Go();
  }

  viskores::cont::PartitionedDataSet GetOutput() const
  {
    viskores::cont::PartitionedDataSet output;
    for (const auto& b : this->Blocks)
    {
      viskores::cont::DataSet ds;
      if (b.GetOutput(ds))
        output.AppendPartition(ds);
    }
    return output;
  }

  void SetStepSize(viskores::FloatDefault stepSize) { this->StepSize = stepSize; }

  void SetSeeds(const viskores::cont::ArrayHandle<ParticleType>& seeds)
  {
    this->ClearParticles();

    viskores::Id n = seeds.GetNumberOfValues();
    auto portal = seeds.ReadPortal();

    std::vector<std::vector<viskores::Id>> blockIDs;
    std::vector<ParticleType> particles;
    for (viskores::Id i = 0; i < n; i++)
    {
      const ParticleType p = portal.Get(i);
      std::vector<viskores::Id> ids = this->BoundsMap.FindBlocks(p.GetPosition());

      //Note: For duplicate blocks, this will give the seeds to the rank that are first in the list.
      if (!ids.empty())
      {
        auto ranks = this->BoundsMap.FindRank(ids[0]);
        if (!ranks.empty() && this->Rank == ranks[0])
        {
          particles.emplace_back(p);
          blockIDs.emplace_back(ids);
        }
      }
    }
    this->SetSeedArray(particles, blockIDs);
  }

  virtual bool HaveWork()
  {
    const bool haveParticles = !this->Active.empty() || !this->Inactive.empty();
#ifndef VISKORES_ENABLE_MPI
    return haveParticles;
#else
    return haveParticles || this->Exchanger.HaveWork();
#endif
  }

  virtual bool GetDone()
  {
#ifndef VISKORES_ENABLE_MPI
    return !this->HaveWork();
#else
    return this->Terminator.Done();
#endif
  }

  //Advect all the particles.
  virtual void Go()
  {
    while (!this->GetDone())
    {
      std::vector<ParticleType> v;
      viskores::Id blockId = -1;

      if (this->GetActiveParticles(v, blockId))
      {
        //make this a pointer to avoid the copy?
        auto& block = this->GetDataSet(blockId);
        DSIHelperInfo<ParticleType> bb(v, this->BoundsMap, this->ParticleBlockIDsMap);
        block.Advect(bb, this->StepSize);
        this->UpdateResult(bb);
      }

      this->ExchangeParticles();
    }
  }

  virtual void ClearParticles()
  {
    this->Active.clear();
    this->Inactive.clear();
    this->ParticleBlockIDsMap.clear();
  }

  DataSetIntegrator<DSIType, ParticleType>& GetDataSet(viskores::Id id)
  {
    for (auto& it : this->Blocks)
      if (it.GetID() == id)
        return it;

    throw viskores::cont::ErrorFilterExecution("Bad block");
  }

  virtual void SetSeedArray(const std::vector<ParticleType>& particles,
                            const std::vector<std::vector<viskores::Id>>& blockIds)
  {
    VISKORES_ASSERT(particles.size() == blockIds.size());

    auto pit = particles.begin();
    auto bit = blockIds.begin();
    while (pit != particles.end() && bit != blockIds.end())
    {
      viskores::Id blockId0 = (*bit)[0];
      this->ParticleBlockIDsMap[pit->GetID()] = *bit;
      if (this->Active.find(blockId0) == this->Active.end())
        this->Active[blockId0] = { *pit };
      else
        this->Active[blockId0].emplace_back(*pit);
      pit++;
      bit++;
    }
  }

  virtual bool GetActiveParticles(std::vector<ParticleType>& particles, viskores::Id& blockId)
  {
    particles.clear();
    blockId = -1;
    if (this->Active.empty())
      return false;

    //If only one, return it.
    if (this->Active.size() == 1)
    {
      blockId = this->Active.begin()->first;
      particles = std::move(this->Active.begin()->second);
      this->Active.clear();
    }
    else
    {
      //Find the blockId with the most particles.
      std::size_t maxNum = 0;
      auto maxIt = this->Active.end();
      for (auto it = this->Active.begin(); it != this->Active.end(); it++)
      {
        auto sz = it->second.size();
        if (sz > maxNum)
        {
          maxNum = sz;
          maxIt = it;
        }
      }

      if (maxNum == 0)
      {
        this->Active.clear();
        return false;
      }

      blockId = maxIt->first;
      particles = std::move(maxIt->second);
      this->Active.erase(maxIt);
    }

    return !particles.empty();
  }

  void ExchangeParticles()
  {
#ifndef VISKORES_ENABLE_MPI
    this->SerialExchange();
#else
    // MPI with only 1 rank.
    if (this->NumRanks == 1)
      this->SerialExchange();
    else
    {
      std::vector<ParticleType> outgoing;
      std::vector<viskores::Id> outgoingRanks;

      this->GetOutgoingParticles(outgoing, outgoingRanks);

      std::vector<ParticleType> incoming;
      std::unordered_map<viskores::Id, std::vector<viskores::Id>> incomingBlockIDs;

      this->Exchanger.Exchange(
        outgoing, outgoingRanks, this->ParticleBlockIDsMap, incoming, incomingBlockIDs);

      //Cleanup what was sent.
      for (const auto& p : outgoing)
        this->ParticleBlockIDsMap.erase(p.GetID());

      this->UpdateActive(incoming, incomingBlockIDs);
    }

    this->Terminator.Control(this->HaveWork());
#endif
  }

  void SerialExchange()
  {
    for (const auto& p : this->Inactive)
    {
      const auto& bid = this->ParticleBlockIDsMap[p.GetID()];
      VISKORES_ASSERT(!bid.empty());
      this->Active[bid[0]].emplace_back(std::move(p));
    }
    this->Inactive.clear();
  }

#ifdef VISKORES_ENABLE_MPI
  void GetOutgoingParticles(std::vector<ParticleType>& outgoing,
                            std::vector<viskores::Id>& outgoingRanks)
  {
    outgoing.clear();
    outgoingRanks.clear();

    outgoing.reserve(this->Inactive.size());
    outgoingRanks.reserve(this->Inactive.size());

    std::vector<ParticleType> particlesStaying;
    std::unordered_map<viskores::Id, std::vector<viskores::Id>> particlesStayingBlockIDs;
    //Send out Everything.
    for (const auto& p : this->Inactive)
    {
      const auto& bid = this->ParticleBlockIDsMap[p.GetID()];
      VISKORES_ASSERT(!bid.empty());

      auto ranks = this->BoundsMap.FindRank(bid[0]);
      VISKORES_ASSERT(!ranks.empty());

      //Only 1 rank has the block.
      if (ranks.size() == 1)
      {
        if (ranks[0] == this->Rank)
        {
          particlesStaying.emplace_back(p);
          particlesStayingBlockIDs[p.GetID()] = this->ParticleBlockIDsMap[p.GetID()];
        }
        else
        {
          outgoing.emplace_back(p);
          outgoingRanks.emplace_back(ranks[0]);
        }
      }
      else
      {
        //Multiple ranks have the block, decide where it should go...

        //Random selection:
        viskores::Id outRank = std::rand() % ranks.size();
        if (outRank == this->Rank)
        {
          particlesStayingBlockIDs[p.GetID()] = this->ParticleBlockIDsMap[p.GetID()];
          particlesStaying.emplace_back(p);
        }
        else
        {
          outgoing.emplace_back(p);
          outgoingRanks.emplace_back(outRank);
        }
      }
    }

    this->Inactive.clear();
    VISKORES_ASSERT(outgoing.size() == outgoingRanks.size());

    VISKORES_ASSERT(particlesStaying.size() == particlesStayingBlockIDs.size());
    if (!particlesStaying.empty())
      this->UpdateActive(particlesStaying, particlesStayingBlockIDs);
  }
#endif

  virtual void UpdateActive(
    const std::vector<ParticleType>& particles,
    const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& idsMap)
  {
    VISKORES_ASSERT(particles.size() == idsMap.size());

    if (!particles.empty())
    {
      for (auto pit = particles.begin(); pit != particles.end(); pit++)
      {
        viskores::Id particleID = pit->GetID();
        const auto& it = idsMap.find(particleID);
        VISKORES_ASSERT(it != idsMap.end() && !it->second.empty());
        viskores::Id blockId = it->second[0];
        this->Active[blockId].emplace_back(*pit);
      }

      for (const auto& it : idsMap)
        this->ParticleBlockIDsMap[it.first] = it.second;
    }
  }

  virtual void UpdateInactive(
    const std::vector<ParticleType>& particles,
    const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& idsMap)
  {
    VISKORES_ASSERT(particles.size() == idsMap.size());

    this->Inactive.insert(this->Inactive.end(), particles.begin(), particles.end());
    for (const auto& it : idsMap)
      this->ParticleBlockIDsMap[it.first] = it.second;
  }

  viskores::Id UpdateResult(const DSIHelperInfo<ParticleType>& stuff)
  {
    this->UpdateActive(stuff.InBounds.Particles, stuff.InBounds.BlockIDs);
    this->UpdateInactive(stuff.OutOfBounds.Particles, stuff.OutOfBounds.BlockIDs);

    viskores::Id numTerm = static_cast<viskores::Id>(stuff.TermID.size());
    //Update terminated particles.
    if (numTerm > 0)
    {
      for (const auto& id : stuff.TermID)
        this->ParticleBlockIDsMap.erase(id);
    }

    return numTerm;
  }

  //Member data
  // {blockId, std::vector of particles}
  std::unordered_map<viskores::Id, std::vector<ParticleType>> Active;
  std::vector<DSIType> Blocks;
  viskores::filter::flow::internal::BoundsMap BoundsMap;
  viskoresdiy::mpi::communicator Comm = viskores::cont::EnvironmentTracker::GetCommunicator();
#ifdef VISKORES_ENABLE_MPI
  ParticleExchanger<ParticleType> Exchanger;
  AdvectAlgorithmTerminator Terminator;
#endif
  std::vector<ParticleType> Inactive;
  viskores::Id MaxNumberOfSteps = 0;
  viskores::Id NumRanks;
  //{particleId : {block IDs}}
  std::unordered_map<viskores::Id, std::vector<viskores::Id>> ParticleBlockIDsMap;
  viskores::Id Rank;
  viskores::FloatDefault StepSize;
};

}
}
}
} //viskores::filter::flow::internal

#endif //viskores_filter_flow_internal_AdvectAlgorithm_h
