namespace diy
{
    struct Master::IExchangeInfo
    {
      using   Clock   = std::chrono::high_resolution_clock;
      using   Time    = Clock::time_point;

                        IExchangeInfo(mpi::communicator comm_, size_t min_queue_size, size_t max_hold_time, bool fine, stats::Profiler& prof_):
                            comm(comm_),
                            fine_(fine),
                            min_queue_size_(min_queue_size),
                            max_hold_time_(max_hold_time),
                            prof(prof_)                         { time_stamp_send(); }
      virtual           ~IExchangeInfo()                        {}

      void              not_done(int gid)                       { update_done(gid, false); }
      inline void       update_done(int gid, bool done_);

      virtual bool      all_done() =0;                             // get global all done status
      virtual void      add_work(int work) =0;                     // add work to global work counter
      virtual void      control() =0;

      void              inc_work()                              { add_work(1); }   // increment work counter
      void              dec_work()                              { add_work(-1); }  // decremnent work counter

      // shortcut
      void              time_stamp_send()                       { time_last_send = Clock::now(); }
      bool              hold(size_t queue_size)                 { return queue_size < min_queue_size_ && hold_time() < max_hold_time_; }
      size_t            hold_time()                             { return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - time_last_send).count(); }
      bool              fine() const                            { return fine_; }

      mpi::communicator                   comm;
      std::unordered_map<int, bool>       done;                 // gid -> done

      bool                                fine_ = false;

      std::shared_ptr<spd::logger>        log = get_logger();
      Time                                time_last_send;       // time of last send from any queue in send_outgoing_queues()

      size_t                              min_queue_size_;      // minimum short message size (bytes)
      size_t                              max_hold_time_;       // maximum short message hold time (milliseconds)

      int                                 from_gid = -1;        // gid of current block, for shortcut sending of only this block's queues

      stats::Profiler&                    prof;
    };
}

void
diy::Master::IExchangeInfo::
update_done(int gid, bool done_)
{
    if (done[gid] != done_)
    {
        done[gid] = done_;
        if (done_)
            dec_work();
        else
            inc_work();
    }
}


#include "iexchange-dud.hpp"
#include "iexchange-collective.hpp"
