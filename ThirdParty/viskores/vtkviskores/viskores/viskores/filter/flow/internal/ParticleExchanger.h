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

#ifndef viskores_filter_flow_internal_ParticleExchanger_h
#define viskores_filter_flow_internal_ParticleExchanger_h

namespace viskores
{
namespace filter
{
namespace flow
{
namespace internal
{

template <typename ParticleType>
class ParticleExchanger
{
public:
#ifdef VISKORES_ENABLE_MPI
  ParticleExchanger(viskoresdiy::mpi::communicator& comm)
    : MPIComm(viskoresdiy::mpi::mpi_cast(comm.handle()))
    , NumRanks(comm.size())
    , Rank(comm.rank())
#else
  ParticleExchanger(viskoresdiy::mpi::communicator& viskoresNotUsed(comm))
#endif
  {
  }
#ifdef VISKORES_ENABLE_MPI
  ~ParticleExchanger() {} //{ this->CleanupSendBuffers(false); }
#endif

  bool HaveWork() const { return !this->SendBuffers.empty(); }

  void Exchange(const std::vector<ParticleType>& outData,
                const std::vector<viskores::Id>& outRanks,
                const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& outBlockIDsMap,
                std::vector<ParticleType>& inData,
                std::unordered_map<viskores::Id, std::vector<viskores::Id>>& inDataBlockIDsMap)
  {
    VISKORES_ASSERT(outData.size() == outRanks.size());

    if (this->NumRanks == 1)
      this->SerialExchange(outData, outBlockIDsMap, inData, inDataBlockIDsMap);
#ifdef VISKORES_ENABLE_MPI
    else
    {
      this->CleanupSendBuffers(true);
      this->SendParticles(outData, outRanks, outBlockIDsMap);
      this->RecvParticles(inData, inDataBlockIDsMap);
    }
#endif
  }

private:
  void SerialExchange(
    const std::vector<ParticleType>& outData,
    const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& outBlockIDsMap,
    std::vector<ParticleType>& inData,
    std::unordered_map<viskores::Id, std::vector<viskores::Id>>& inDataBlockIDsMap)
  {
    //Copy output to input.
    for (const auto& p : outData)
    {
      const auto& bids = outBlockIDsMap.find(p.GetID())->second;
      inData.emplace_back(p);
      inDataBlockIDsMap[p.GetID()] = bids;
    }
  }

#ifdef VISKORES_ENABLE_MPI
  using ParticleCommType = std::pair<ParticleType, std::vector<viskores::Id>>;

  void CleanupSendBuffers(bool checkRequests)
  {
    if (!checkRequests)
    {
      for (auto& entry : this->SendBuffers)
        delete entry.second;
      this->SendBuffers.clear();
      return;
    }

    if (this->SendBuffers.empty())
      return;

    std::vector<MPI_Request> requests;
    for (auto& req : this->SendBuffers)
      requests.emplace_back(req.first);

    //MPI_Testsome will update the complete requests to MPI_REQUEST_NULL.
    //Because we are using the MPI_Request as a key in SendBuffers, we need
    //to make a copy.
    auto requestsOrig = requests;

    std::vector<MPI_Status> status(requests.size());
    std::vector<int> indices(requests.size());
    int num = 0;
    int err = MPI_Testsome(requests.size(), requests.data(), &num, indices.data(), status.data());

    if (err != MPI_SUCCESS)
      throw viskores::cont::ErrorFilterExecution(
        "Error with MPI_Testsome in ParticleExchanger::CleanupSendBuffers");

    if (num > 0)
    {
      for (int i = 0; i < num; i++)
      {
        std::size_t idx = static_cast<std::size_t>(indices[i]);
        const auto& req = requestsOrig[idx];
        //const auto& stat = status[idx];
        auto it = this->SendBuffers.find(req);
        if (it == this->SendBuffers.end())
          throw viskores::cont::ErrorFilterExecution(
            "Missing request in ParticleExchanger::CleanupSendBuffers");

        //Delete the buffer and remove from SendBuffers.
        delete it->second;
        this->SendBuffers.erase(it);
        //std::cout<<this->Rank<<" SendBuffer: Delete"<<std::endl;
      }
    }
  }

  void SendParticles(
    const std::vector<ParticleType>& outData,
    const std::vector<viskores::Id>& outRanks,
    const std::unordered_map<viskores::Id, std::vector<viskores::Id>>& outBlockIDsMap)
  {
    if (outData.empty())
      return;

    //create the send data: vector of particles, vector of vector of blockIds.
    std::size_t n = outData.size();
    std::unordered_map<int, std::vector<ParticleCommType>> sendData;

    // dst, vector of pair(particles, blockIds)
    for (std::size_t i = 0; i < n; i++)
    {
      const auto& bids = outBlockIDsMap.find(outData[i].GetID())->second;
      //sendData[outRanks[i]].emplace_back(std::make_pair(std::move(outData[i]), std::move(bids)));
      sendData[outRanks[i]].emplace_back(std::make_pair(outData[i], bids));
    }

    //Send to dst, vector<pair<particle, bids>>
    for (auto& si : sendData)
      this->SendParticlesToDst(si.first, si.second);
  }

  void SendParticlesToDst(int dst, const std::vector<ParticleCommType>& data)
  {
    if (dst == this->Rank)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error, "Error. Sending a particle to yourself.");
      return;
    }

    //Serialize vector(pair(particle, bids)) and send.
    viskoresdiy::MemoryBuffer* bb = new viskoresdiy::MemoryBuffer();
    viskoresdiy::save(*bb, data);
    bb->reset();

    MPI_Request req;
    int err =
      MPI_Isend(bb->buffer.data(), bb->size(), MPI_BYTE, dst, this->Tag, this->MPIComm, &req);
    if (err != MPI_SUCCESS)
      throw viskores::cont::ErrorFilterExecution("Error in MPI_Isend inside Messenger::SendData");
    this->SendBuffers[req] = bb;
  }

  void RecvParticles(
    std::vector<ParticleType>& inData,
    std::unordered_map<viskores::Id, std::vector<viskores::Id>>& inDataBlockIDsMap) const
  {
    inData.resize(0);
    inDataBlockIDsMap.clear();

    std::vector<viskoresdiy::MemoryBuffer> buffers;

    MPI_Status status;
    while (true)
    {
      int flag = 0;
      int err = MPI_Iprobe(MPI_ANY_SOURCE, this->Tag, this->MPIComm, &flag, &status);
      if (err != MPI_SUCCESS)
        throw viskores::cont::ErrorFilterExecution(
          "Error in MPI_Probe in ParticleExchanger::RecvParticles");

      if (flag == 0) //no message arrived we are done.
        break;

      //Otherwise, recv the incoming data
      int incomingSize;
      err = MPI_Get_count(&status, MPI_BYTE, &incomingSize);
      if (err != MPI_SUCCESS)
        throw viskores::cont::ErrorFilterExecution(
          "Error in MPI_Probe in ParticleExchanger::RecvParticles");

      std::vector<char> recvBuff;
      recvBuff.resize(incomingSize);
      MPI_Status recvStatus;

      err = MPI_Recv(recvBuff.data(),
                     incomingSize,
                     MPI_BYTE,
                     status.MPI_SOURCE,
                     status.MPI_TAG,
                     this->MPIComm,
                     &recvStatus);
      if (err != MPI_SUCCESS)
        throw viskores::cont::ErrorFilterExecution(
          "Error in MPI_Probe in ParticleExchanger::RecvParticles");

      //Add incoming data to inData and inDataBlockIds.
      viskoresdiy::MemoryBuffer memBuff;
      memBuff.save_binary(recvBuff.data(), incomingSize);
      memBuff.reset();

      std::vector<ParticleCommType> data;
      viskoresdiy::load(memBuff, data);
      memBuff.reset();
      for (const auto& d : data)
      {
        const auto& particle = d.first;
        const auto& bids = d.second;
        inDataBlockIDsMap[particle.GetID()] = bids;
        inData.emplace_back(particle);
      }

      //Note, we don't terminate the while loop here. We want to go back and
      //check if any messages came in while buffers were being processed.
    }
  }

  MPI_Comm MPIComm;
  viskores::Id NumRanks;
  viskores::Id Rank;
  std::unordered_map<MPI_Request, viskoresdiy::MemoryBuffer*> SendBuffers;
  int Tag = 100;
#else
  viskores::Id NumRanks = 1;
  viskores::Id Rank = 0;
#endif
};

}
}
}
} //viskores::filter::flow::internal


#endif //viskores_filter_flow_internal_ParticleExchanger_h
