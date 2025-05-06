#include <atomic>

namespace diy
{
    struct Master::IExchangeInfoCollective: public IExchangeInfo
    {
                        IExchangeInfoCollective(mpi::communicator c, stats::Profiler& p):
                            IExchangeInfo(c, p)
      {
          local_work_ = 0;
          dirty = 0;
          state = 0;
      }

      inline bool       all_done() override;                    // get global all done status
      inline void       add_work(int work) override;            // add work to global work counter
      inline void       control() override;

      std::atomic<int>  local_work_;
      std::atomic<int>  dirty;
      int               local_dirty, all_dirty;

      std::atomic<int>  state;
      mpi::request      r;

      // debug
      bool              first_ibarrier = true;

      using IExchangeInfo::prof;
    };
}

bool
diy::Master::IExchangeInfoCollective::
all_done()
{
    return state == 3;
}

void
diy::Master::IExchangeInfoCollective::
add_work(int work)
{
    local_work_ += work;
    if (local_work_ > 0)
        dirty = 1;
}

void
diy::Master::IExchangeInfoCollective::
control()
{
    if (state == 0 && local_work_ == 0)
    {
        // debug
        if (first_ibarrier)
        {
            prof >> "iexchange-control";        // consensus-time cannot nest in iexchange-control
            prof << "consensus-time";
            prof << "iexchange-control";
            first_ibarrier = false;
        }

        r = ibarrier(comm);
        dirty = 0;
        state = 1;
    } else if (state == 1)
    {
        mpi::optional<mpi::status> ostatus = r.test();
        if (ostatus)
        {
            local_dirty = dirty;
            r = mpi::iall_reduce(comm, local_dirty, all_dirty, std::logical_or<int>());
            state = 2;
        }
    } else if (state == 2)
    {
        mpi::optional<mpi::status> ostatus = r.test();
        if (ostatus)
        {
            if (all_dirty == 0)     // done
                state = 3;
            else
                state = 0;          // reset
        }
    }
}

