#ifndef DIY_IO_BLOCK_HPP
#define DIY_IO_BLOCK_HPP

#include <string>
#include <algorithm>
#include <stdexcept>

#include "../mpi.hpp"
#include "../assigner.hpp"
#include "../master.hpp"
#include "../storage.hpp"
#include "../log.hpp"
#include "utils.hpp"

// Read and write collections of blocks using MPI-IO
namespace diy
{
namespace io
{
  namespace detail
  {
    typedef mpi::io::offset                 offset_t;

    struct GidOffsetCount
    {
                    GidOffsetCount():                                   // need to initialize a vector of given size
                        gid(-1), offset(0), count(0)                    {}

                    GidOffsetCount(int gid_, offset_t offset_, offset_t count_):
                        gid(gid_), offset(offset_), count(count_)       {}

        bool        operator<(const GidOffsetCount& other) const        { return gid < other.gid; }

        int         gid;
        offset_t    offset;
        offset_t    count;
    };
  }
}

// Serialize GidOffsetCount explicitly, to avoid alignment and uninitialized data issues
// (to get identical output files given the same block input)
template<>
struct Serialization<io::detail::GidOffsetCount>
{
    typedef             io::detail::GidOffsetCount                  GidOffsetCount;

    static void         save(BinaryBuffer& bb, const GidOffsetCount& x)
    {
      diy::save(bb, x.gid);
      diy::save(bb, x.offset);
      diy::save(bb, x.count);
    }

    static void         load(BinaryBuffer& bb, GidOffsetCount& x)
    {
      diy::load(bb, x.gid);
      diy::load(bb, x.offset);
      diy::load(bb, x.count);
    }
};

namespace io
{
/**
 * \ingroup IO
 * \brief Write blocks to storage collectively in one shared file
 */
  inline
  void
  write_blocks(const std::string&           outfilename,           //!< output file name
               const mpi::communicator&     comm,                  //!< communicator
               Master&                      master,                //!< master object
               const MemoryBuffer&          extra = MemoryBuffer(),//!< user-defined metadata for file header; meaningful only on rank == 0
               Master::SaveBlock            save = 0)              //!< block save function in case different than or undefined in the master
  {
    if (!save) save = master.saver();       // save is likely to be different from master.save()

    typedef detail::offset_t                offset_t;
    typedef detail::GidOffsetCount          GidOffsetCount;

    unsigned size = master.size(),
             max_size, min_size;
    mpi::all_reduce(comm, size, max_size, mpi::maximum<unsigned>());
    mpi::all_reduce(comm, size, min_size, mpi::minimum<unsigned>());

    // truncate the file
    if (comm.rank() == 0)
        diy::io::utils::truncate(outfilename.c_str(), 0);

    mpi::io::file f(comm, outfilename, mpi::io::file::wronly | mpi::io::file::create);

    offset_t  start = 0, shift;
    std::vector<GidOffsetCount>     offset_counts;
    for (unsigned i = 0; i < max_size; ++i)
    {
      offset_t count = 0,
               offset;
      if (i < size)
      {
        // get the block from master and serialize it
        const void* block = master.get(i);
        MemoryBuffer bb;
        LinkFactory::save(bb, master.link(i));
        save(block, bb);
        count = bb.buffer.size();
        mpi::scan(comm, count, offset, std::plus<offset_t>());
        offset += start - count;
        mpi::all_reduce(comm, count, shift, std::plus<offset_t>());
        start += shift;

        if (i < min_size)       // up to min_size, we can do collective IO
          f.write_at_all(offset, bb.buffer);
        else
          f.write_at(offset, bb.buffer);

        offset_counts.push_back(GidOffsetCount(master.gid(i), offset, count));
      } else
      {
        // matching global operations
        mpi::scan(comm, count, offset, std::plus<offset_t>());
        mpi::all_reduce(comm, count, shift, std::plus<offset_t>());

        // -1 indicates that there is no block written here from this rank
        offset_counts.push_back(GidOffsetCount(-1, offset, count));
      }
    }

    if (comm.rank() == 0)
    {
      // round-about way of gather vector of vectors of GidOffsetCount to avoid registering a new mpi datatype
      std::vector< std::vector<char> > gathered_offset_count_buffers;
      MemoryBuffer oc_buffer; diy::save(oc_buffer, offset_counts);
      mpi::gather(comm, oc_buffer.buffer, gathered_offset_count_buffers, 0);

      std::vector<GidOffsetCount>  all_offset_counts;
      for (unsigned i = 0; i < gathered_offset_count_buffers.size(); ++i)
      {
        MemoryBuffer per_rank_oc_buffer; per_rank_oc_buffer.buffer.swap(gathered_offset_count_buffers[i]);
        std::vector<GidOffsetCount> per_rank_offset_counts;
        diy::load(per_rank_oc_buffer, per_rank_offset_counts);
        for (unsigned j = 0; j < per_rank_offset_counts.size(); ++j)
          if (per_rank_offset_counts[j].gid != -1)
            all_offset_counts.push_back(per_rank_offset_counts[j]);
      }
      std::sort(all_offset_counts.begin(), all_offset_counts.end());        // sorts by gid

      MemoryBuffer bb;
      diy::save(bb, all_offset_counts);
      diy::save(bb, extra);
      size_t footer_size = bb.size();
      diy::save(bb, footer_size);

      // find footer_offset as the max of (offset + count)
      offset_t footer_offset = 0;
      for (unsigned i = 0; i < all_offset_counts.size(); ++i)
      {
        offset_t end = all_offset_counts[i].offset + all_offset_counts[i].count;
        if (end > footer_offset)
            footer_offset = end;
      }
      f.write_at(footer_offset, bb.buffer);
    } else
    {
      MemoryBuffer oc_buffer; diy::save(oc_buffer, offset_counts);
      mpi::gather(comm, oc_buffer.buffer, 0);
    }
  }

/**
 * \ingroup IO
 * \brief Read blocks from storage collectively from one shared file
 */
    inline
    void
    read_blocks(const std::string&           infilename,     //!< input file name
                const mpi::communicator&     comm,           //!< communicator
                StaticAssigner&              assigner,       //!< assigner object
                Master&                      master,         //!< master object
                MemoryBuffer&                extra,          //!< user-defined metadata in file header
                Master::LoadBlock            load = 0)       //!< load block function in case different than or unefined in the master
    {
        if (!load) load = master.loader();      // load is likely to be different from master.load()

        typedef detail::offset_t                offset_t;
        typedef detail::GidOffsetCount          GidOffsetCount;

        mpi::io::file f(comm, infilename, mpi::io::file::rdonly);

        offset_t    footer_offset = f.size() - sizeof(size_t);
        size_t footer_size;

        // Read the size
        f.read_at_all(footer_offset, (char*) &footer_size, sizeof(footer_size));

        // Read all_offset_counts
        footer_offset -= footer_size;
        MemoryBuffer footer;
        footer.buffer.resize(footer_size);
        f.read_at_all(footer_offset, footer.buffer);

        std::vector<GidOffsetCount>  all_offset_counts;
        diy::load(footer, all_offset_counts);
        diy::load(footer, extra);
        extra.reset();

        // Get local gids from assigner
        assigner.set_nblocks(static_cast<int>(all_offset_counts.size()));
        std::vector<int> gids;
        assigner.local_gids(comm.rank(), gids);

        for (size_t i = 0; i < gids.size(); ++i)
        {
            if (gids[i] != all_offset_counts[gids[i]].gid)
                get_logger()->warn("gids don't match in diy::io::read_blocks(), {} vs {}",
                                   gids[i], all_offset_counts[gids[i]].gid);

            offset_t offset = all_offset_counts[gids[i]].offset,
                     count  = all_offset_counts[gids[i]].count;
            MemoryBuffer bb;
            bb.buffer.resize(count);
            f.read_at(offset, bb.buffer);
            Link* l = LinkFactory::load(bb);
            l->fix(assigner);
            void* b = master.create();
            load(b, bb);
            master.add(gids[i], b, l);
        }
    }


  // Functions without the extra buffer, for compatibility with the old code
  inline
  void
  write_blocks(const std::string&           outfilename,
               const mpi::communicator&     comm,
               Master&                      master,
               Master::SaveBlock            save)
  {
    MemoryBuffer extra;
    write_blocks(outfilename, comm, master, extra, save);
  }

  inline
  void
  read_blocks(const std::string&           infilename,
              const mpi::communicator&     comm,
              StaticAssigner&              assigner,
              Master&                      master,
              Master::LoadBlock            load = 0)
  {
    MemoryBuffer extra;     // dummy
    read_blocks(infilename, comm, assigner, master, extra, load);
  }

namespace split
{
/**
 * \ingroup IO
 * \brief Write blocks to storage independently in one file per process
 */
  inline
  void
  write_blocks(const std::string&           outfilename,           //!< output file name
               const mpi::communicator&     comm,                  //!< communicator
               Master&                      master,                //!< master object
               const MemoryBuffer&          extra = MemoryBuffer(),//!< user-defined metadata for file header; meaningful only on rank == 0
               Master::SaveBlock            save = 0)              //!< block save function in case different than or undefined in master
  {
    if (!save) save = master.saver();       // save is likely to be different from master.save()

    bool proceed = false;
    size_t size = 0;
    if (comm.rank() == 0)
    {
        if (diy::io::utils::is_directory(outfilename))
          proceed = true;
        else if (diy::io::utils::make_directory(outfilename))
          proceed = true;

        mpi::broadcast(comm, proceed, 0);
        mpi::reduce(comm, (size_t) master.size(), size, 0, std::plus<size_t>());
    } else
    {
        mpi::broadcast(comm, proceed, 0);
        mpi::reduce(comm, (size_t) master.size(), 0, std::plus<size_t>());
    }

    if (!proceed)
        throw std::runtime_error("Cannot access or create directory: " + outfilename);

    for (int i = 0; i < (int)master.size(); ++i)
    {
        const void* block = master.get(i);

        std::string filename = fmt::format("{}/{}", outfilename, master.gid(i));

        ::diy::detail::FileBuffer bb(fopen(filename.c_str(), "w"));

        LinkFactory::save(bb, master.link(i));
        save(block, bb);

        fclose(bb.file);
    }

    if (comm.rank() == 0)
    {
        // save the extra buffer
        std::string filename = outfilename + "/extra";
        ::diy::detail::FileBuffer bb(fopen(filename.c_str(), "w"));
        ::diy::save(bb, size);
        ::diy::save(bb, extra);
        fclose(bb.file);
    }
  }

/**
 * \ingroup IO
 * \brief Read blocks from storage independently from one file per process
 */
  inline
  void
  read_blocks(const std::string&           infilename,  //!< input file name
              const mpi::communicator&     comm,        //!< communicator
              StaticAssigner&              assigner,    //!< assigner object
              Master&                      master,      //!< master object
              MemoryBuffer&                extra,       //!< user-defined metadata in file header
              Master::LoadBlock            load = 0)    //!< block load function in case different than or undefined in master
  {
    if (!load) load = master.loader();      // load is likely to be different from master.load()

    // load the extra buffer and size
    size_t          size;
    {
        std::string filename = infilename + "/extra";
        ::diy::detail::FileBuffer bb(fopen(filename.c_str(), "r"));
        ::diy::load(bb, size);
        ::diy::load(bb, extra);
        extra.reset();
        fclose(bb.file);
    }

    // Get local gids from assigner
    assigner.set_nblocks(static_cast<int>(size));
    std::vector<int> gids;
    assigner.local_gids(comm.rank(), gids);

    // Read our blocks;
    for (unsigned i = 0; i < gids.size(); ++i)
    {
        std::string filename = fmt::format("{}/{}", infilename, gids[i]);

        ::diy::detail::FileBuffer bb(fopen(filename.c_str(), "r"));
        Link* l = LinkFactory::load(bb);
        l->fix(assigner);
        void* b = master.create();
        load(b, bb);
        master.add(gids[i], b, l);

        fclose(bb.file);
    }
  }

  // Functions without the extra buffer, for compatibility with the old code
  inline
  void
  write_blocks(const std::string&           outfilename,
               const mpi::communicator&     comm,
               Master&                      master,
               Master::SaveBlock            save)
  {
    MemoryBuffer extra;
    write_blocks(outfilename, comm, master, extra, save);
  }

  inline
  void
  read_blocks(const std::string&           infilename,
              const mpi::communicator&     comm,
              StaticAssigner&              assigner,
              Master&                      master,
              Master::LoadBlock            load = 0)
  {
    MemoryBuffer extra;     // dummy
    read_blocks(infilename, comm, assigner, master, extra, load);
  }
} // split
} // io
} // diy

#endif
