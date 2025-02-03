namespace diy
{
    struct Master::IExchangeInfo
    {
      using   Clock   = std::chrono::high_resolution_clock;
      using   Time    = Clock::time_point;

                        IExchangeInfo(mpi::communicator c, stats::Profiler& p):
                            comm(c),
                            prof(p)                             {}
      virtual           ~IExchangeInfo()                        {}

      virtual bool      all_done() =0;                             // get global all done status
      virtual void      add_work(int work) =0;                     // add work to global work counter
      virtual void      control() =0;

      void              inc_work()                              { add_work(1); }   // increment work counter
      void              dec_work()                              { add_work(-1); }  // decremnent work counter

      mpi::communicator                   comm;

      std::shared_ptr<spd::logger>        log = get_logger();
      stats::Profiler&                    prof;
    };
}


#include "iexchange-collective.hpp"
