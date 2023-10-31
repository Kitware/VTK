#ifndef DIY_MASTER_HPP
#define DIY_MASTER_HPP

#include <vector>
#include <map>
#include <list>
#include <deque>
#include <algorithm>
#include <functional>
#include <numeric>
#include <memory>
#include <chrono>

#include "link.hpp"
#include "collection.hpp"

// Communicator functionality
#include "mpi.hpp"
#include "serialization.hpp"
#include "time.hpp"

#include "thread.hpp"

#include "detail/block_traits.hpp"

#include "log.hpp"
#include "stats.hpp"

namespace diy
{
  // Stores and manages blocks; initiates serialization and communication when necessary.
  //
  // Provides a foreach function, which is meant as the main entry point.
  //
  // Provides a conversion between global and local block ids,
  // which is hidden from blocks via a communicator proxy.
  class Master
  {
    public:
      struct ProcessBlock;

      // Commands; forward declarations, defined in detail/master/commands.hpp
      struct BaseCommand;

      template<class Block>
      struct Command;

      using Commands = std::vector<std::unique_ptr<BaseCommand>>;

      // Skip
      using Skip = std::function<bool(int, const Master&)>;

      struct SkipNoIncoming;
      struct NeverSkip { bool    operator()(int, const Master&) const { return false; } };

      // Collection
      typedef Collection::Create            CreateBlock;
      typedef Collection::Destroy           DestroyBlock;
      typedef Collection::Save              SaveBlock;
      typedef Collection::Load              LoadBlock;

    public:
      // Communicator types, defined in proxy.hpp
      struct Proxy;
      struct ProxyWithLink;

      // foreach callback
      template<class Block>
      using Callback = std::function<void(Block*, const ProxyWithLink&)>;

      // iexchange callback
      template<class Block>
      using ICallback = std::function<bool(Block*, const ProxyWithLink&)>;

      struct QueuePolicy
      {
        virtual bool    unload_incoming(const Master& master, int from, int to, size_t size) const  =0;
        virtual bool    unload_outgoing(const Master& master, int from, size_t size) const          =0;
        virtual         ~QueuePolicy() {}
      };

      //! Move queues out of core if their size exceeds a parameter given in the constructor
      struct QueueSizePolicy: public QueuePolicy
      {
                QueueSizePolicy(size_t sz): size(sz)          {}
        bool    unload_incoming(const Master&, int, int, size_t sz) const         { return sz > size; }
        bool    unload_outgoing(const Master& master, int from, size_t sz) const  { return sz > size*master.outgoing_count(from); }

        size_t  size;
      };

      // forward declarations, defined in detail/master/communication.hpp
      struct MessageInfo;
      struct InFlightSend;
      struct InFlightRecv;

      struct GidSendOrder;
      struct IExchangeInfo;
      struct IExchangeInfoDUD;
      struct IExchangeInfoCollective;

      // forward declarations, defined in detail/master/collectives.hpp
      struct Collective;

      struct InFlightSendsList;     // std::list<InFlightSend>
      struct InFlightRecvsMap;      // std::map<int, InFlightRecv>          //
      struct CollectivesList;       // std::list<Collective>
      struct CollectivesMap;        // std::map<int, CollectivesList>       // gid -> [collectives]


      struct QueueRecord
      {
                        QueueRecord(size_t s = 0, int e = -1): size(s), external(e)     {}
        size_t          size;
        int             external;
      };

      typedef           std::map<int,     QueueRecord>      InQueueRecords;     //  gid         -> (size, external)
      typedef           std::map<int,     MemoryBuffer>     IncomingQueues;     //  gid         -> queue
      typedef           std::map<BlockID, MemoryBuffer>     OutgoingQueues;     // (gid, proc)  -> queue
      typedef           std::map<BlockID, QueueRecord>      OutQueueRecords;    // (gid, proc)  -> (size, external)
      struct IncomingQueuesRecords
      {
        InQueueRecords  records;
        IncomingQueues  queues;
      };
      struct OutgoingQueuesRecord
      {
                        OutgoingQueuesRecord(int e = -1): external(e)       {}
        int             external;
        OutQueueRecords external_local;
        OutgoingQueues  queues;
      };
      typedef           std::map<int,     IncomingQueuesRecords>    IncomingQueuesMap;  //  gid         -> {  gid       -> queue }
      typedef           std::map<int,     OutgoingQueuesRecord>     OutgoingQueuesMap;  //  gid         -> { (gid,proc) -> queue }

      struct IncomingRound
      {
        IncomingQueuesMap map;
        int received{0};
      };
      typedef std::map<int, IncomingRound> IncomingRoundMap;


    public:
     /**
      * \ingroup Initialization
      * \brief The main DIY object
      *
      * Helper functions specify how to:
           * create an empty block,
           * destroy a block (a function that's expected to upcast and delete),
           * serialize a block
      */
      inline        Master(mpi::communicator    comm,          //!< communicator
                           int                  threads__ = 1,  //!< number of threads DIY can use
                           int                  limit__   = -1, //!< number of blocks to store in memory
                           CreateBlock          create_   = 0,  //!< block create function; master manages creation if create != 0
                           DestroyBlock         destroy_  = 0,  //!< block destroy function; master manages destruction if destroy != 0
                           ExternalStorage*     storage   = 0,  //!< storage object (path, method, etc.) for storing temporary blocks being shuffled in/out of core
                           SaveBlock            save      = 0,  //!< block save function; master manages saving if save != 0
                           LoadBlock            load_     = 0,  //!< block load function; master manages loading if load != 0
                           QueuePolicy*         q_policy  = new QueueSizePolicy(4096)); //!< policy for managing message queues specifies maximum size of message queues to keep in memory
      inline        ~Master();

      inline void   clear();
      inline void   destroy(int i)                      { if (blocks_.own()) blocks_.destroy(i); }

      inline int    add(int gid, void* b, Link* l);     //!< add a block
      inline void*  release(int i);                     //!< release ownership of the block

      //!< return the `i`-th block
      inline void*  block(int i) const                  { return blocks_.find(i); }
      template<class Block>
      Block*        block(int i) const                  { return static_cast<Block*>(block(i)); }
      //! return the `i`-th block, loading it if necessary
      void*         get(int i)                          { return blocks_.get(i); }
      template<class Block>
      Block*        get(int i)                          { return static_cast<Block*>(get(i)); }

      inline Link*  link(int i) const                   { return links_[i]; }
      inline int    loaded_block() const                { return blocks_.available(); }

      inline void   unload(int i);
      inline void   load(int i);
      void          unload(std::vector<int>& loaded)    { for(unsigned i = 0; i < loaded.size(); ++i) unload(loaded[i]); loaded.clear(); }
      void          unload_all()                        { for(unsigned i = 0; i < size(); ++i) if (block(i) != 0) unload(i); }
      inline bool   has_incoming(int i) const;

      inline void   unload_queues(int i);
      inline void   unload_incoming(int gid);
      inline void   unload_outgoing(int gid);
      inline void   load_queues(int i);
      inline void   load_incoming(int gid);
      inline void   load_outgoing(int gid);

      //! return the MPI communicator
      const mpi::communicator&  communicator() const    { return comm_; }
      //! return the MPI communicator
      mpi::communicator&        communicator()          { return comm_; }

      //! return gid of the `i`-th block
      int           gid(int i) const                    { return gids_[i]; }
      //! return the local id of the local block with global id gid, or -1 if not local
      int           lid(int gid__) const                { return local(gid__) ?  lids_.find(gid__)->second : -1; }
      //! whether the block with global id gid is local
      bool          local(int gid__) const              { return lids_.find(gid__) != lids_.end(); }

      //! exchange the queues between all the blocks (collective operation)
      inline void   exchange(bool remote = false);

      //! nonblocking exchange of the queues between all the blocks
      template<class Block>
      void          iexchange_(const ICallback<Block>&  f,
                               size_t                   min_queue_size,
                               size_t                   max_hold_time,
                               bool                     fine);

      template<class F>
      void          iexchange(const     F&      f,
                              size_t            min_queue_size = 0,     // in bytes, queues smaller than min_queue_size will be held for up to max_hold_time
                              size_t            max_hold_time  = 0,     // in milliseconds
                              bool              fine           = false)
      {
          using Block = typename detail::block_traits<F>::type;
          iexchange_<Block>(f, min_queue_size, max_hold_time, fine);
      }

      inline void   process_collectives();

      inline
      ProxyWithLink proxy(int i) const;

      inline
      ProxyWithLink proxy(int i, IExchangeInfo* iexchange) const;

      //! return the number of local blocks
      unsigned int  size() const                        { return static_cast<unsigned int>(blocks_.size()); }
      void*         create() const                      { return blocks_.create(); }

      // accessors
      int           limit() const                       { return limit_; }
      int           threads() const                     { return threads_; }
      int           in_memory() const                   { return *blocks_.in_memory().const_access(); }

      void          set_threads(int threads__)          { threads_ = threads__; }

      CreateBlock   creator() const                     { return blocks_.creator(); }
      DestroyBlock  destroyer() const                   { return blocks_.destroyer(); }
      LoadBlock     loader() const                      { return blocks_.loader(); }
      SaveBlock     saver() const                       { return blocks_.saver(); }

      //! call `f` with every block
      template<class Block>
      void          foreach_(const Callback<Block>& f, const Skip& s = NeverSkip());

      template<class F>
      void          foreach(const F& f, const Skip& s = NeverSkip())
      {
          using Block = typename detail::block_traits<F>::type;
          foreach_<Block>(f, s);
      }

      inline void   execute();

      bool          immediate() const                   { return immediate_; }
      void          set_immediate(bool i)               { if (i && !immediate_) execute(); immediate_ = i; }

    public:
      // Communicator functionality
      IncomingQueues&   incoming(int gid__)             { return incoming_[exchange_round_].map[gid__].queues; }
      OutgoingQueues&   outgoing(int gid__)             { return outgoing_[gid__].queues; }
      size_t            outgoing_count(int gid__) const { OutgoingQueuesMap::const_iterator it = outgoing_.find(gid__); if (it == outgoing_.end()) return 0; return it->second.queues.size(); }
      inline CollectivesList&  collectives(int gid__);
      inline CollectivesMap&   collectives();

      void              set_expected(int expected)      { expected_ = expected; }
      void              add_expected(int i)             { expected_ += i; }
      int               expected() const                { return expected_; }
      void              replace_link(int i, Link* link__) { expected_ -= links_[i]->size_unique(); delete links_[i]; links_[i] = link__; expected_ += links_[i]->size_unique(); }

    public:
      // Communicator functionality
      inline void       flush(bool remote = false);     // makes sure all the serialized queues migrate to their target processors

    private:
      // Communicator functionality
      inline void       comm_exchange(GidSendOrder& gid_order, IExchangeInfo*    iexchange = 0);
      inline void       rcomm_exchange();    // possibly called in between block computations
      inline bool       nudge(IExchangeInfo* iexchange = 0);
      inline void       send_queue(int from_gid, int to_gid, int to_proc, MemoryBuffer& out_queue, bool remote, IExchangeInfo* iexchange);
      inline void       send_outgoing_queues(GidSendOrder&   gid_order,
                                             bool            remote,
                                             IExchangeInfo*  iexchange = 0);
      inline void       check_incoming_queues(IExchangeInfo* iexchange = 0);
      inline GidSendOrder
                        order_gids();
      inline void       touch_queues();
      inline void       move_external_local(int from);
      inline void       send_same_rank(int from, int to, MemoryBuffer& bb, IExchangeInfo* iexchange);
      inline void       send_different_rank(int from, int to, int proc, MemoryBuffer& bb, bool remote, IExchangeInfo* iexchange);

      inline InFlightRecv&         inflight_recv(int proc);
      inline InFlightSendsList&    inflight_sends();

      // iexchange commmunication
      inline void       icommunicate(IExchangeInfo* iexchange);     // async communication

      // debug
      inline void       show_incoming_records() const;

      struct tags       { enum {
                                    queue,
                                    iexchange
                                }; };

    private:
      std::vector<Link*>    links_;
      Collection            blocks_;
      std::vector<int>      gids_;
      std::map<int, int>    lids_;

      QueuePolicy*          queue_policy_;

      int                   limit_;
      int                   threads_;
      ExternalStorage*      storage_;

    private:
      // Communicator
      mpi::communicator     comm_;
      IncomingRoundMap      incoming_;
      OutgoingQueuesMap     outgoing_;

      std::unique_ptr<InFlightSendsList> inflight_sends_;
      std::unique_ptr<InFlightRecvsMap>  inflight_recvs_;
      std::unique_ptr<CollectivesMap>    collectives_;

      int                   expected_           = 0;
      int                   exchange_round_     = -1;
      bool                  immediate_          = true;
      Commands              commands_;

    private:
      fast_mutex            add_mutex_;

    public:
      std::shared_ptr<spd::logger>  log = get_logger();
      stats::Profiler               prof;
      stats::Annotation             exchange_round_annotation { "diy.exchange-round" };
  };

  struct Master::SkipNoIncoming
  { bool operator()(int i, const Master& master) const   { return !master.has_incoming(i); } };
}

#include "detail/master/iexchange.hpp"
#include "detail/master/communication.hpp"
#include "detail/master/collectives.hpp"
#include "detail/master/commands.hpp"
#include "proxy.hpp"
#include "detail/master/execution.hpp"

diy::Master::
Master(mpi::communicator    comm,
       int                  threads__,
       int                  limit__,
       CreateBlock          create_,
       DestroyBlock         destroy_,
       ExternalStorage*     storage,
       SaveBlock            save,
       LoadBlock            load_,
       QueuePolicy*         q_policy):
  blocks_(create_, destroy_, storage, save, load_),
  queue_policy_(q_policy),
  limit_(limit__),
  threads_(threads__ == -1 ? static_cast<int>(thread::hardware_concurrency()) : threads__),
  storage_(storage),
  // Communicator functionality
  inflight_sends_(new InFlightSendsList),
  inflight_recvs_(new InFlightRecvsMap),
  collectives_(new CollectivesMap)
{
    comm_.duplicate(comm);
}

diy::Master::
~Master()
{
    set_immediate(true);
    clear();
    delete queue_policy_;
}

void
diy::Master::
clear()
{
  for (unsigned i = 0; i < size(); ++i)
    delete links_[i];
  blocks_.clear();
  links_.clear();
  gids_.clear();
  lids_.clear();
  expected_ = 0;
}

void
diy::Master::
unload(int i)
{
  log->debug("Unloading block: {}", gid(i));

  blocks_.unload(i);
  unload_queues(i);
}

void
diy::Master::
unload_queues(int i)
{
  unload_incoming(gid(i));
  unload_outgoing(gid(i));
}

void
diy::Master::
unload_incoming(int gid__)
{
  for (IncomingRoundMap::iterator round_itr = incoming_.begin(); round_itr != incoming_.end(); ++round_itr)
  {
    IncomingQueuesMap::iterator qmap_itr = round_itr->second.map.find(gid__);
    if (qmap_itr == round_itr->second.map.end())
    {
      continue;
    }
    IncomingQueuesRecords& in_qrs = qmap_itr->second;
    for (InQueueRecords::iterator it = in_qrs.records.begin(); it != in_qrs.records.end(); ++it)
    {
      QueueRecord& qr = it->second;
      if (queue_policy_->unload_incoming(*this, it->first, gid__, qr.size))
      {
        log->debug("Unloading queue: {} <- {}", gid__, it->first);
        qr.external = storage_->put(in_qrs.queues[it->first]);
      }
    }
  }
}

void
diy::Master::
unload_outgoing(int gid__)
{
  OutgoingQueuesRecord& out_qr = outgoing_[gid__];

  size_t out_queues_size = sizeof(size_t);   // map size
  size_t count = 0;
  // count the size of the queues we need to pack
  for (auto& rec : out_qr.queues)
  {
    if (rec.first.proc == comm_.rank()) continue;

    out_queues_size += sizeof(BlockID);                                 // target
    out_queues_size += Serialization<MemoryBuffer>::size(rec.second);   // buffer contents
    ++count;
  }
  if (queue_policy_->unload_outgoing(*this, gid__, out_queues_size - sizeof(size_t)))
  {
      log->debug("Unloading outgoing queues: {} -> ...; size = {}\n", gid__, out_queues_size);
      MemoryBuffer  bb;     bb.reserve(out_queues_size);
      diy::save(bb, count);

      // pack queues going to a remote proc into bb; queues going to a
      // different block on our rank, stay separated, recorded in external_local
      for (auto it = out_qr.queues.begin(); it != out_qr.queues.end();)
      {
        auto  bid    = it->first;
        auto& buffer = it->second;
        if (bid.proc == comm_.rank())
        {
          // treat as incoming
          if (queue_policy_->unload_incoming(*this, gid__, bid.gid, buffer.size()))
          {
            QueueRecord& qr = out_qr.external_local[bid];
            qr.size     = buffer.size();
            qr.external = storage_->put(buffer);

            out_qr.queues.erase(it++);
          } ++it; // else keep in memory
        } else
        {
          diy::save(bb, bid);
          diy::save(bb, buffer);

          out_qr.queues.erase(it++);
        }
      }

      // TODO: this mechanism could be adjusted for direct saving to disk
      //       (without intermediate binary buffer serialization)
      out_qr.external = storage_->put(bb);
  }
}

void
diy::Master::
load(int i)
{
 log->debug("Loading block: {}", gid(i));

  blocks_.load(i);
  load_queues(i);
}

void
diy::Master::
load_queues(int i)
{
  load_incoming(gid(i));
  load_outgoing(gid(i));
}

void
diy::Master::
load_incoming(int gid__)
{
  IncomingQueuesRecords& in_qrs = incoming_[exchange_round_].map[gid__];
  for (InQueueRecords::iterator it = in_qrs.records.begin(); it != in_qrs.records.end(); ++it)
  {
    QueueRecord& qr = it->second;
    if (qr.external != -1)
    {
        log->debug("Loading queue: {} <- {}", gid__, it->first);
        storage_->get(qr.external, in_qrs.queues[it->first]);
        qr.external = -1;
    }
  }
}

void
diy::Master::
load_outgoing(int gid__)
{
  // TODO: we could adjust this mechanism to read directly from storage,
  //       bypassing an intermediate MemoryBuffer
  OutgoingQueuesRecord& out_qr = outgoing_[gid__];
  if (out_qr.external != -1)
  {
    MemoryBuffer bb;
    storage_->get(out_qr.external, bb);
    out_qr.external = -1;

    size_t count;
    diy::load(bb, count);
    for (size_t i = 0; i < count; ++i)
    {
      BlockID to;
      diy::load(bb, to);
      diy::load(bb, out_qr.queues[to]);
    }
  }
}

diy::Master::ProxyWithLink
diy::Master::
proxy(int i) const
{ return ProxyWithLink(Proxy(const_cast<Master*>(this), gid(i)), block(i), link(i)); }

diy::Master::ProxyWithLink
diy::Master::
proxy(int i, IExchangeInfo* iexchange) const
{ return ProxyWithLink(Proxy(const_cast<Master*>(this), gid(i), iexchange), block(i), link(i)); }

int
diy::Master::
add(int gid__, void* b, Link* l)
{
  if (*blocks_.in_memory().const_access() == limit_)
    unload_all();

  lock_guard<fast_mutex>    lock(add_mutex_);       // allow to add blocks from multiple threads

  blocks_.add(b);
  links_.push_back(l);
  gids_.push_back(gid__);

  int lid__ = static_cast<int>(gids_.size()) - 1;
  lids_[gid__] = lid__;
  add_expected(l->size_unique()); // NB: at every iteration we expect a message from each unique neighbor

  return lid__;
}

void*
diy::Master::
release(int i)
{
  void* b = blocks_.release(i);
  delete link(i);   links_[i] = 0;
  lids_.erase(gid(i));
  return b;
}

bool
diy::Master::
has_incoming(int i) const
{
  const IncomingQueuesRecords& in_qrs = const_cast<Master&>(*this).incoming_[exchange_round_].map[gid(i)];
  for (InQueueRecords::const_iterator it = in_qrs.records.begin(); it != in_qrs.records.end(); ++it)
  {
    const QueueRecord& qr = it->second;
    if (qr.size != 0)
        return true;
  }
  return false;
}

template<class Block>
void
diy::Master::
foreach_(const Callback<Block>& f, const Skip& skip)
{
    exchange_round_annotation.set(exchange_round_);

    auto scoped = prof.scoped("foreach");
    DIY_UNUSED(scoped);

    commands_.emplace_back(new Command<Block>(f, skip));

    if (immediate())
        execute();
}

void
diy::Master::
exchange(bool remote)
{
  auto scoped = prof.scoped("exchange");
  DIY_UNUSED(scoped);

  execute();

  log->debug("Starting exchange");

#ifdef DIY_NO_MPI
  // remote doesn't need to do anything special if there is no mpi, but we also
  // can't just use it because of the ibarrier
  remote = false;
#endif

  // make sure there is a queue for each neighbor
  if (!remote)
      touch_queues();

  flush(remote);
  log->debug("Finished exchange");
}

void
diy::Master::
touch_queues()
{
  for (int i = 0; i < (int)size(); ++i)
  {
      OutgoingQueues&  outgoing_queues  = outgoing_[gid(i)].queues;
      OutQueueRecords& external_local   = outgoing_[gid(i)].external_local;
      if (outgoing_queues.size() < (size_t)link(i)->size())
          for (unsigned j = 0; j < (unsigned)link(i)->size(); ++j)
          {
              if (external_local.find(link(i)->target(j)) == external_local.end())
                  outgoing_queues[link(i)->target(j)];        // touch the outgoing queue, creating it if necessary
          }
  }
}

// iexchange()
// {
//     while !all_done
//         for all blocks
//             icommunicate
//             iproxywithlink
//             f
//             icommunicate()
// }

template<class Block>
void
diy::Master::
iexchange_(const    ICallback<Block>&   f,
           size_t                       min_queue_size,
           size_t                       max_hold_time,
           bool                         fine)
{
    auto scoped = prof.scoped("iexchange");
    DIY_UNUSED(scoped);

    // prepare for next round
    incoming_.erase(exchange_round_);
    ++exchange_round_;
    exchange_round_annotation.set(exchange_round_);

    //IExchangeInfoDUD iexchange(comm_, min_queue_size, max_hold_time, fine, prof);
    IExchangeInfoCollective iexchange(comm_, min_queue_size, max_hold_time, fine, prof);
    iexchange.add_work(size());                 // start with one work unit for each block

    std::map<int, bool> done_result;
    do
    {
        for (size_t i = 0; i < size(); i++)     // for all blocks
        {
            iexchange.from_gid = gid(i);       // for shortcut sending only from current block during icommunicate
            stats::Annotation::Guard g( stats::Annotation("diy.block").set(iexchange.from_gid) );

            icommunicate(&iexchange);               // TODO: separate comm thread std::thread t(icommunicate);
            ProxyWithLink cp = proxy(i, &iexchange);

            bool done = done_result[cp.gid()];
            if (!done || !cp.empty_incoming_queues())
            {
                prof << "callback";
                done = f(block<Block>(i), cp);
                prof >> "callback";
            }
            done_result[cp.gid()] = done;

            done &= cp.empty_queues();

            log->debug("Done: {}", done);

            prof << "work-counting";
            iexchange.update_done(cp.gid(), done);
            prof >> "work-counting";
        }

        prof << "iexchange-control";
        iexchange.control();
        prof >> "iexchange-control";
    } while (!iexchange.all_done());
    log->info("[{}] ==== Leaving iexchange ====\n", iexchange.comm.rank());

    //comm_.barrier();        // TODO: this is only necessary for DUD
    prof >> "consensus-time";

    outgoing_.clear();
}

/* Communicator */
void
diy::Master::
comm_exchange(GidSendOrder& gid_order, IExchangeInfo* iexchange)
{
    auto scoped = prof.scoped("comm-exchange");
    DIY_UNUSED(scoped);

    send_outgoing_queues(gid_order, false, iexchange);

    while(nudge(iexchange))                         // kick requests
        ;

    check_incoming_queues(iexchange);
}

/* Remote communicator */

// pseudocode for rexchange protocol based on NBX algorithm of Hoefler et al.,
// Scalable Communication Protocols for Dynamic Sparse Data Exchange, 2010.
//
// rcomm_exchange()
// {
//      while (!done)
//          while (sends_in_flight < limit_on_queues_in_memory and there are unprocessed queues)
//              q = next outgoing queue (going over the in-memory queues first)
//              if (q not in memory)
//                  load q
//              issend(q)
//
//           test all requests
//           if (iprobe)
//               recv
//           if (barrier_active)
//               if (test barrier)
//                   done = true
//           else
//               if (all sends finished and all queues have been processed (including out-of-core queues))
//                   ibarrier
//                   barrier_active = true
// }
//
void
diy::Master::
rcomm_exchange()
{
    bool            done                = false;
    bool            ibarr_act           = false;
    mpi::request    ibarr_req;                      // mpi request associated with ibarrier

    // make a list of outgoing queues to send (the ones in memory come first)
    auto gid_order = order_gids();

    while (!done)
    {
        send_outgoing_queues(gid_order, true, 0);

        // kick requests
        nudge();

        check_incoming_queues();
        if (ibarr_act)
        {
            if (ibarr_req.test())
                done = true;
        }
        else
        {
            if (gid_order.empty() && inflight_sends().empty())
            {
                ibarr_req = comm_.ibarrier();
                ibarr_act = true;
            }
        }
    }                                                 // while !done
}

// fill list of outgoing queues to send (the ones in memory come first)
// for iexchange
diy::Master::GidSendOrder
diy::Master::
order_gids()
{
    auto scoped = prof.scoped("order-gids");

    GidSendOrder order;

    for (OutgoingQueuesMap::iterator it = outgoing_.begin(); it != outgoing_.end(); ++it)
    {
        OutgoingQueuesRecord& out = it->second;
        if (out.external == -1)
            order.list.push_front(it->first);
        else
            order.list.push_back(it->first);
    }
    log->debug("order.size(): {}", order.size());

    // compute maximum number of queues to keep in memory
    // first version just average number of queues per block * num_blocks in memory
    // for iexchange
    if (limit_ == -1 || size() == 0)
        order.limit = order.size();
    else
        // average number of queues per block * in-memory block limit
        order.limit = (std::max)((size_t) 1, order.size() / size() * limit_);

    return order;
}

// iexchange communicator
void
diy::Master::
icommunicate(IExchangeInfo* iexchange)
{
    auto scoped = prof.scoped("icommunicate");
    DIY_UNUSED(scoped);

    log->debug("Entering icommunicate()");

    // lock out other threads
    // TODO: not threaded yet
    // if (!CAS(comm_flag, 0, 1))
    //     return;

    // debug
//     log->info("out_queues_limit: {}", out_queues_limit);

    // order gids

    auto gid_order = order_gids();

    // exchange
    comm_exchange(gid_order, iexchange);

    // cleanup

    // NB: not doing outgoing_.clear() as in Master::flush() so that outgoing queues remain in place
    // TODO: consider having a flush function for a final cleanup if the user plans to move to
    // another part of the DIY program

    log->debug("Exiting icommunicate()");
}

// send a single queue, either to same rank or different rank
void
diy::Master::
send_queue(int              from_gid,
           int              to_gid,
           int              to_proc,
           MemoryBuffer&    out_queue,
           bool             remote,
           IExchangeInfo*   iexchange)
{
    stats::Annotation::Guard gb( stats::Annotation("diy.block").set(from_gid) );
    stats::Annotation::Guard gt( stats::Annotation("diy.to").set(to_gid) );
    stats::Annotation::Guard gq( stats::Annotation("diy.q-size").set(stats::Variant(static_cast<uint64_t>(out_queue.size()))) );

    // skip empty queues and hold queues shorter than some limit for some time
    if ( iexchange && (out_queue.size() == 0 || iexchange->hold(out_queue.size())) )
        return;
    log->debug("[{}] Sending queue: {} <- {} of size {}, iexchange = {}", comm_.rank(), to_gid, from_gid, out_queue.size(), iexchange ? 1 : 0);

    if (iexchange)
        iexchange->time_stamp_send();       // hold time begins counting from now

    if (to_proc == comm_.rank())            // sending to same rank, simply swap buffers
        send_same_rank(from_gid, to_gid, out_queue, iexchange);
    else                                    // sending to an actual message to a different rank
        send_different_rank(from_gid, to_gid, to_proc, out_queue, remote, iexchange);
}

void
diy::Master::
send_outgoing_queues(GidSendOrder&   gid_order,
                     bool            remote,            // TODO: are remote and iexchange mutually exclusive? If so, use single enum?
                     IExchangeInfo*  iexchange)
{
    auto scoped = prof.scoped("send-outgoing-queues");
    DIY_UNUSED(scoped);

    if (iexchange)                                      // for iexchange, send queues from a single block
    {
        OutgoingQueues& outgoing = outgoing_[iexchange->from_gid].queues;
        for (OutgoingQueues::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
        {
            BlockID to_block    = it->first;
            int     to_gid      = to_block.gid;
            int     to_proc     = to_block.proc;

            log->debug("Processing queue:      {} <- {} of size {}", to_gid, iexchange->from_gid, outgoing_[iexchange->from_gid].queues[to_block].size());
            send_queue(iexchange->from_gid, to_gid, to_proc, it->second, remote, iexchange);
        }
    }
    else                                                // normal mode: send all outgoing queues
    {
        while (inflight_sends().size() < gid_order.limit && !gid_order.empty())
        {
            int from_gid = gid_order.pop();

            // move external queues going to our rank
            move_external_local(from_gid);

            if (outgoing_[from_gid].external != -1)
                load_outgoing(from_gid);

            OutgoingQueues& outgoing = outgoing_[from_gid].queues;
            for (OutgoingQueues::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
            {
                BlockID to_block    = it->first;
                int     to_gid      = to_block.gid;
                int     to_proc     = to_block.proc;

                log->debug("Processing queue:      {} <- {} of size {}", to_gid, from_gid, outgoing_[from_gid].queues[to_block].size());
                send_queue(from_gid, to_gid, to_proc, it->second, remote, iexchange);
            }
        }
    }
}

void
diy::Master::
move_external_local(int from)
{
    IncomingRound& current_incoming = incoming_[exchange_round_];

    // deal with external_local queues
    for (auto& x : outgoing_[from].external_local)
    {
        int to = x.first.gid;

        log->debug("Processing local queue: {} <- {} of size {}", to, from, x.second.size);

        QueueRecord& in_qr        = current_incoming.map[to].records[from];
        bool         to_external  = block(lid(to)) == 0;

        if (to_external)
            in_qr = x.second;
        else
        {
            // load the queue
            in_qr.size     = x.second.size;
            in_qr.external = -1;

            MemoryBuffer bb;
            storage_->get(x.second.external, bb);

            current_incoming.map[to].queues[from].swap(bb);
        }
        current_incoming.received++;
    }
    outgoing_[from].external_local.clear();
}

void
diy::Master::
send_same_rank(int from, int to, MemoryBuffer& bb, IExchangeInfo* iexchange)
{
    auto scoped = prof.scoped("send-same-rank");

    log->debug("Moving queue in-place: {} <- {}", to, from);

    IncomingRound& current_incoming = incoming_[exchange_round_];

    QueueRecord& in_qr       = current_incoming.map[to].records[from];
    bool         to_external = block(lid(to)) == 0;
    if (to_external)
    {
        log->debug("Unloading outgoing directly as incoming: {} <- {}", to, from);
        in_qr.size = bb.size();
        if (queue_policy_->unload_incoming(*this, from, to, in_qr.size))
            in_qr.external = storage_->put(bb);
        else
        {
            MemoryBuffer& in_bb = current_incoming.map[to].queues[from];
            if (!iexchange)
            {
                in_bb.swap(bb);
                in_bb.reset();
            }
            else
            {
                iexchange->not_done(to);
                in_bb.append_binary(&bb.buffer[0], bb.size());
                bb.clear();
            }
            in_qr.external = -1;
        }
    } else        // !to_external
    {
        log->debug("Swapping in memory:    {} <- {}", to, from);
        MemoryBuffer& in_bb = current_incoming.map[to].queues[from];
        if (!iexchange)
        {
            in_bb.swap(bb);
            in_bb.reset();
        }
        else
        {
            iexchange->not_done(to);
            in_bb.append_binary(&bb.buffer[0], bb.size());
            bb.wipe();
        }
        in_qr.size = in_bb.size();
        in_qr.external = -1;
    }

    ++current_incoming.received;
}

void
diy::Master::
send_different_rank(int from, int to, int proc, MemoryBuffer& bb, bool remote, IExchangeInfo* iexchange)
{
    auto scoped = prof.scoped("send-different-rank");

    static const size_t MAX_MPI_MESSAGE_COUNT = INT_MAX;

    // sending to a different rank
    std::shared_ptr<MemoryBuffer> buffer = std::make_shared<MemoryBuffer>();
    buffer->swap(bb);

    MessageInfo info{from, to, 1, exchange_round_};
    // size fits in one message
    if (Serialization<MemoryBuffer>::size(*buffer) + Serialization<MessageInfo>::size(info) <= MAX_MPI_MESSAGE_COUNT)
    {
        diy::save(*buffer, info);

        inflight_sends().emplace_back();
        auto& inflight_send = inflight_sends().back();

        inflight_send.info = info;
        if (remote || iexchange)
        {
            if (iexchange)
            {
                iexchange->inc_work();
                log->debug("[{}] Incrementing work when sending queue\n", comm_.rank());
            }
            inflight_send.request = comm_.issend(proc, tags::queue, buffer->buffer);
        }
        else
            inflight_send.request = comm_.isend(proc, tags::queue, buffer->buffer);
        inflight_send.message = buffer;
    }
    else // large message gets broken into chunks
    {
        int npieces = static_cast<int>((buffer->size() + MAX_MPI_MESSAGE_COUNT - 1)/MAX_MPI_MESSAGE_COUNT);
        info.nparts += npieces;

        // first send the head
        std::shared_ptr<MemoryBuffer> hb = std::make_shared<MemoryBuffer>();
        diy::save(*hb, buffer->size());
        diy::save(*hb, info);

        inflight_sends().emplace_back();
        auto& inflight_send = inflight_sends().back();

        inflight_send.info = info;
        if (remote || iexchange)
        {
            // add one unit of work for the entire large message (upon sending the head, not the individual pieces below)
            if (iexchange)
            {
                iexchange->inc_work();
                log->debug("[{}] Incrementing work when sending the leading piece\n", comm_.rank());
            }
            inflight_send.request = comm_.issend(proc, tags::queue, hb->buffer);
        }
        else
            inflight_send.request = comm_.isend(proc, tags::queue, hb->buffer);
        inflight_send.message = hb;

        // send the message pieces
        size_t msg_buff_idx = 0;
        for (int i = 0; i < npieces; ++i, msg_buff_idx += MAX_MPI_MESSAGE_COUNT)
        {
            detail::VectorWindow<char> window;
            window.begin = &buffer->buffer[msg_buff_idx];
            window.count = (std::min)(MAX_MPI_MESSAGE_COUNT, buffer->size() - msg_buff_idx);

            inflight_sends().emplace_back();
            auto& inflight_send = inflight_sends().back();

            inflight_send.info = info;
            if (remote || iexchange)
            {
                if (iexchange)
                {
                    iexchange->inc_work();
                    log->debug("[{}] Incrementing work when sending non-leading piece\n", comm_.rank());
                }
                inflight_send.request = comm_.issend(proc, tags::queue, window);
            }
            else
                inflight_send.request = comm_.isend(proc, tags::queue, window);
            inflight_send.message = buffer;
        }
    }   // large message broken into pieces
}

void
diy::Master::
check_incoming_queues(IExchangeInfo* iexchange)
{
    auto scoped = prof.scoped("check-incoming-queues");
    DIY_UNUSED(scoped);

    mpi::optional<mpi::status> ostatus = comm_.iprobe(mpi::any_source, tags::queue);
    while (ostatus)
    {
        InFlightRecv& ir = inflight_recv(ostatus->source());

        if (iexchange)
            iexchange->inc_work();                      // increment work before sender's issend request can complete (so we are now responsible for the queue)
        bool first_message = ir.recv(comm_, *ostatus);  // possibly partial recv, in case of a multi-piece message
        if (!first_message && iexchange)
            iexchange->dec_work();

        if (ir.done)                // all pieces assembled
        {
            assert(ir.info.round >= exchange_round_);
            IncomingRound* in = &incoming_[ir.info.round];

            bool unload = ((ir.info.round == exchange_round_) ? (block(lid(ir.info.to)) == 0) : (limit_ != -1))
                          && queue_policy_->unload_incoming(*this, ir.info.from, ir.info.to, ir.message.size());

            ir.place(in, unload, storage_, iexchange);
            ir.reset();
        }

        ostatus = comm_.iprobe(mpi::any_source, tags::queue);
    }
}

void
diy::Master::
flush(bool remote)
{
#ifdef DEBUG
  time_type start = get_time();
  unsigned wait = 1;
#endif

  // prepare for next round
  incoming_.erase(exchange_round_);
  ++exchange_round_;
  exchange_round_annotation.set(exchange_round_);


  if (remote)
      rcomm_exchange();
  else
  {
      auto gid_order = order_gids();
      do
      {
          comm_exchange(gid_order);

#ifdef DEBUG
          time_type cur = get_time();
          if (cur - start > wait*1000)
          {
              log->warn("Waiting in flush [{}]: {} - {} out of {}",
                      comm_.rank(), inflight_sends().size(), incoming_[exchange_round_].received, expected_);
              wait *= 2;
          }
#endif
      } while (!inflight_sends().empty() || incoming_[exchange_round_].received < expected_ || !gid_order.empty());
  }

  outgoing_.clear();

  log->debug("Done in flush");
  //show_incoming_records();

  process_collectives();
}

bool
diy::Master::
nudge(IExchangeInfo* iexchange)
{
  bool success = false;
  for (InFlightSendsList::iterator it = inflight_sends().begin(); it != inflight_sends().end();)
  {
    mpi::optional<mpi::status> ostatus = it->request.test();
    if (ostatus)
    {
      success = true;
      it = inflight_sends().erase(it);
      if (iexchange)
      {
          log->debug("[{}] message left, decrementing work", iexchange->comm.rank());
          iexchange->dec_work();                // this message is receiver's responsibility now
      }
    }
    else
    {
      ++it;
    }
  }
  return success;
}

void
diy::Master::
show_incoming_records() const
{
  for (IncomingRoundMap::const_iterator rounds_itr = incoming_.begin(); rounds_itr != incoming_.end(); ++rounds_itr)
  {
    for (IncomingQueuesMap::const_iterator it = rounds_itr->second.map.begin(); it != rounds_itr->second.map.end(); ++it)
    {
      const IncomingQueuesRecords& in_qrs = it->second;
      for (InQueueRecords::const_iterator cur = in_qrs.records.begin(); cur != in_qrs.records.end(); ++cur)
      {
        const QueueRecord& qr = cur->second;
        log->info("round: {}, {} <- {}: (size,external) = ({},{})",
                  rounds_itr->first,
                  it->first, cur->first,
                  qr.size,
                  qr.external);
      }
      for (IncomingQueues::const_iterator cur = in_qrs.queues.begin(); cur != in_qrs.queues.end(); ++cur)
      {
        log->info("round: {}, {} <- {}: queue.size() = {}",
                  rounds_itr->first,
                  it->first, cur->first,
                  const_cast<IncomingQueuesRecords&>(in_qrs).queues[cur->first].size());
      }
    }
  }
}

#endif
