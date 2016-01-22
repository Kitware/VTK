#ifndef DIY_ASSIGNER_HPP
#define DIY_ASSIGNER_HPP

#include <vector>

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
                    Assigner(int size,     //!< total number of processes
                             int nblocks   //!< total (global) number of blocks
                             ):
                      size_(size), nblocks_(nblocks)    {}

      //! returns the total number of process ranks
      int           size() const                        { return size_; }
      //! returns the total number of global blocks
      int           nblocks() const                     { return nblocks_; }
      //! sets the total number of global blocks
      void          set_nblocks(int nblocks)            { nblocks_ = nblocks; }
      //! gets the local gids for a given process rank
      virtual void  local_gids(int rank, std::vector<int>& gids) const   =0;
      //! returns the process rank of the block with global id gid (need not be local)
      virtual int   rank(int gid) const     =0;

    private:
      int           size_;      // total number of ranks
      int           nblocks_;   // total number of blocks
  };

  class ContiguousAssigner: public Assigner
  {
    public:
     /**
      * \ingroup Assignment
      * \brief Assigns blocks to processes in contiguous gid (block global id) order
      */
            ContiguousAssigner(int size,     //!< total number of processes
                               int nblocks   //!< total (global) number of blocks
                               ):
              Assigner(size, nblocks)           {}

      using Assigner::size;
      using Assigner::nblocks;

      int   rank(int gid) const
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
      void  local_gids(int rank, std::vector<int>& gids) const;
  };

  class RoundRobinAssigner: public Assigner
  {
    public:
     /**
      * \ingroup Assignment
      * \brief Assigns blocks to processes in cyclic or round-robin gid (block global id) order
      */
            RoundRobinAssigner(int size,     //!< total number of processes
                               int nblocks   //!< total (global) number of blocks
                               ):
              Assigner(size, nblocks)           {}

      using Assigner::size;
      using Assigner::nblocks;

      int   rank(int gid) const                 { return gid % size(); }
      inline
      void  local_gids(int rank, std::vector<int>& gids) const;
  };
}

void
diy::ContiguousAssigner::
local_gids(int rank, std::vector<int>& gids) const
{
  int div = nblocks() / size();
  int mod = nblocks() % size();

  int from, to;
  if (rank < mod)
      from = rank * (div + 1);
  else
      from = mod * (div + 1) + (rank - mod) * div;

  if (rank + 1 < mod)
      to = (rank + 1) * (div + 1);
  else
      to = mod * (div + 1) + (rank + 1 - mod) * div;

  for (int gid = from; gid < to; ++gid)
    gids.push_back(gid);
}

void
diy::RoundRobinAssigner::
local_gids(int rank, std::vector<int>& gids) const
{
  int cur = rank;
  while (cur < nblocks())
  {
    gids.push_back(cur);
    cur += size();
  }
}

#endif
