#ifndef DIY_ASSIGNER_HPP
#define DIY_ASSIGNER_HPP

#include <vector>
#include <tuple>

#include "mpi.hpp"      // needed for DynamicAssigner

namespace diy
{
  // Derived types should define
  //   int rank(int gid) const
  // that converts a global block id to a rank that it's assigned to.
  class Assigner
  {
    public:
     /**
      * \ingroup Assignment
      * \brief Manages how blocks are assigned to processes
      */
                    Assigner(int size__,     //!< total number of processes
                             int nblocks__   //!< total (global) number of blocks
                             ):
                      size_(size__), nblocks_(nblocks__)  {}
      virtual       ~Assigner()                         {}

      //! returns the total number of process ranks
      int           size() const                        { return size_; }
      //! returns the total number of global blocks
      int           nblocks() const                     { return nblocks_; }
      //! sets the total number of global blocks
      virtual void  set_nblocks(int nblocks__)          { nblocks_ = nblocks__; }
      //! returns the process rank of the block with global id gid (need not be local)
      virtual int   rank(int gid) const     =0;
      //! batch lookup; returns the process ranks of the blocks with global id in the vector gids
      inline
      virtual std::vector<int>
                    ranks(const std::vector<int>& gids) const;


    private:
      int           size_;      // total number of ranks
      int           nblocks_;   // total number of blocks
  };

  class StaticAssigner: public Assigner
  {
    public:
     /**
      * \ingroup Assignment
      * \brief Intermediate type to express assignment that cannot change; adds `local_gids` query method
      */
      using Assigner::Assigner;

      //! gets the local gids for a given process rank
      virtual void  local_gids(int rank, std::vector<int>& gids) const   =0;
  };

  class ContiguousAssigner: public StaticAssigner
  {
    public:
     /**
      * \ingroup Assignment
      * \brief Assigns blocks to processes in contiguous gid (block global id) order
      */
      using StaticAssigner::StaticAssigner;

      using StaticAssigner::size;
      using StaticAssigner::nblocks;

      int   rank(int gid) const override
      {
          int div = nblocks() / size();
          int mod = nblocks() % size();
          int r = gid / (div + 1);
          if (r < mod)
          {
              return r;
          } else
          {
              return mod + (gid - (div + 1)*mod)/div;
          }
      }
      inline
      void  local_gids(int rank, std::vector<int>& gids) const override;
  };

  class RoundRobinAssigner: public StaticAssigner
  {
    public:
     /**
      * \ingroup Assignment
      * \brief Assigns blocks to processes in cyclic or round-robin gid (block global id) order
      */
      using StaticAssigner::StaticAssigner;

      using StaticAssigner::size;
      using StaticAssigner::nblocks;

      int   rank(int gid) const override        { return gid % size(); }
      inline
      void  local_gids(int rank, std::vector<int>& gids) const override;
  };

  class DynamicAssigner: public Assigner
  {
    public:
                    DynamicAssigner(const mpi::communicator& comm, int size__, int nblocks__):
                      Assigner(size__, nblocks__),
                      comm_(comm),
                      div_(nblocks__ / size__ + ((nblocks__ % size__) == 0 ? 0 : 1)),   // NB: same size window everywhere means the last rank may allocate extra space
                      rank_map_(comm_, div_)                                            { rank_map_.lock_all(mpi::nocheck); }
                    ~DynamicAssigner()                                                  { rank_map_.unlock_all(); }

      inline
      virtual void  set_nblocks(int nblocks__) override;
      inline
      virtual int   rank(int gid) const override;
      inline
      virtual std::vector<int>
                    ranks(const std::vector<int>& gids) const override;

      inline std::tuple<bool,int>
                    get_rank(int& rk, int gid) const;

      inline void   set_rank(const int& rk, int gid, bool flush = true);
      inline void   set_ranks(const std::vector<std::tuple<int,int>>& rank_gids);

      std::tuple<int,int>
                    rank_offset(int gid) const                                          { return std::make_tuple(gid / div_, gid % div_); }

    private:
      mpi::communicator         comm_;
      int                       div_;
      mutable mpi::window<int>  rank_map_;
  };
}

std::vector<int>
diy::Assigner::
ranks(const std::vector<int>& gids) const
{
    std::vector<int> result(gids.size());
    for (size_t i = 0; i < gids.size(); ++i)
        result[i] = rank(gids[i]);
    return result;
}

void
diy::ContiguousAssigner::
local_gids(int rank_, std::vector<int>& gids) const
{
  int div = nblocks() / size();
  int mod = nblocks() % size();

  int from, to;
  if (rank_ < mod)
      from = rank_ * (div + 1);
  else
      from = mod * (div + 1) + (rank_ - mod) * div;

  if (rank_ + 1 < mod)
      to = (rank_ + 1) * (div + 1);
  else
      to = mod * (div + 1) + (rank_ + 1 - mod) * div;

  for (int gid = from; gid < to; ++gid)
    gids.push_back(gid);
}

void
diy::RoundRobinAssigner::
local_gids(int rank_, std::vector<int>& gids) const
{
  int cur = rank_;
  while (cur < nblocks())
  {
    gids.push_back(cur);
    cur += size();
  }
}

void
diy::DynamicAssigner::
set_nblocks(int nblocks__)
{
    Assigner::set_nblocks(nblocks__);
    div_ = nblocks() / size() + ((nblocks() % size()) == 0 ? 0 : 1);

    rank_map_.unlock_all();
    rank_map_ = mpi::window<int>(comm_, div_);
    rank_map_.lock_all(mpi::nocheck);
}

std::tuple<bool,int>
diy::DynamicAssigner::
get_rank(int& rk, int gid) const
{
    // TODO: check if the gid is in cache

    int r,offset;
    std::tie(r,offset) = rank_offset(gid);

    rank_map_.get(rk, r, offset);

    return std::make_tuple(false, r);        // false indicates that the data wasn't read from cache
}

int
diy::DynamicAssigner::
rank(int gid) const
{
    int rk;
    auto cached_gidrk = get_rank(rk, gid);
    int gidrk = std::get<1>(cached_gidrk);

    rank_map_.flush_local(gidrk);

    return rk;
}


std::vector<int>
diy::DynamicAssigner::
ranks(const std::vector<int>& gids) const
{
    bool all_cached = true;
    std::vector<int> result(gids.size(), -1);
    for (size_t i = 0; i < gids.size(); ++i)
    {
        auto cached_gidrk = get_rank(result[i], gids[i]);
        bool cached = std::get<0>(cached_gidrk);
        all_cached  &= cached;
    }

    if (!all_cached)
        rank_map_.flush_local_all();

    return result;
}

void
diy::DynamicAssigner::
set_rank(const int& rk, int gid, bool flush)
{
    // TODO: update cache

    int r,offset;
    std::tie(r,offset) = rank_offset(gid);

    rank_map_.put(rk, r, offset);

    if (flush)
        rank_map_.flush(r);
}

void
diy::DynamicAssigner::
set_ranks(const std::vector<std::tuple<int,int>>& rank_gids)
{
    for (auto& rg : rank_gids)
        set_rank(std::get<0>(rg), std::get<1>(rg), false);
    rank_map_.flush_all();
}

#endif
