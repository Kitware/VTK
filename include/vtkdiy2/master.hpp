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
#include <climits>
#include <random>

#include "link.hpp"
#include "collection.hpp"

// Communicator functionality
#include "mpi.hpp"
#include "serialization.hpp"
#include "time.hpp"

#include "thread.hpp"

#include "coroutine.hpp"
#include "utils.hpp"

#include "detail/block_traits.hpp"

#include "log.hpp"
#include "stats.hpp"

namespace diy
{

  struct MemoryManagement
  {
      using Allocate = std::function<char* (int, size_t)>;
      using Deallocate = BinaryBlob::Deleter;
      using MemCopy = std::function<void(char*, const char*, size_t)>;

      MemoryManagement()    = default;
      MemoryManagement(Allocate allocate_, Deallocate deallocate_, MemCopy copy_):
            allocate(allocate_), deallocate(deallocate_), copy(copy_)       {}

      Allocate      allocate    = [](int /* gid */, size_t n) { return new char[n]; };
      Deallocate    deallocate  = [](const char* p) { delete[] p; };
      MemCopy       copy        = [](char* dest, const char* src, size_t count) { std::memcpy(dest, src, count); };
  };


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

      // foreach_exchange callback
      template<class Block>
      using CoroutineCallback = std::function<void(Block* const&, const ProxyWithLink&)>;

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
        bool    unload_outgoing(const Master&, int, size_t sz) const              { return sz > size; }

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
                        QueueRecord(MemoryBuffer&& b):
                            buffer_(std::move(b))                                       { size_ = buffer_.size(); external_ = -1; }
                        QueueRecord(size_t s = 0, int e = -1): size_(s), external_(e)   {}
                        QueueRecord(const QueueRecord&) =delete;
                        QueueRecord(QueueRecord&&)      =default;
        QueueRecord&    operator=(const QueueRecord&)   =delete;
        QueueRecord&    operator=(QueueRecord&&)        =default;

        bool            external() const                                                { return external_ != -1; }
        MemoryBuffer&&  move()                                                          { return std::move(buffer_); }
        size_t          size() const                                                    { if (external()) return size_; return buffer_.size(); }

        void            reset()                                                         { buffer_.reset(); }

        void            unload(ExternalStorage* storage)                                { size_ = buffer_.size(); external_ = storage->put(buffer_); }
        void            load(ExternalStorage* storage)                                  { storage->get(external_, buffer_); external_ = -1; }

        MemoryBuffer&   buffer()                                                        { return buffer_; }

        private:
            size_t          size_;
            int             external_;
            MemoryBuffer    buffer_;
      };

      using RecordQueue = critical_resource<std::deque<QueueRecord>>;

      using IncomingQueues = concurrent_map<int,      RecordQueue>;       // gid  -> [(size, external, buffer), ...]
      using OutgoingQueues = concurrent_map<BlockID,  RecordQueue>;       // bid  -> [(size, external, buffer), ...]

      using IncomingQueuesMap = std::map<int, IncomingQueues>;      // gid  -> {  gid -> [(size, external, buffer), ...]}
      using OutgoingQueuesMap = std::map<int, OutgoingQueues>;      // gid  -> {  bid -> [(size, external, buffer), ...]}

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
      inline int    add(int gid, void* b, const Link& l){ return add(gid, b, l.clone()); }
      inline void*  release(int i);                     //!< release ownership of the block

      //!< return the `i`-th block
      inline void*  block(int i) const                  { return blocks_.find(i); }
      template<class Block>
      Block*        block(int i) const                  { return static_cast<Block*>(block(i)); }

      const Collection&
                    blocks() const                      { return blocks_; }

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
      inline void   exchange(bool remote = false, MemoryManagement mem = MemoryManagement());

      //! nonblocking exchange of the queues between all the blocks
      template<class Block>
      void          iexchange_(const ICallback<Block>&  f, MemoryManagement mem);

      template<class F>
      void          iexchange(const F& f, MemoryManagement mem = MemoryManagement())
      {
          using Block = typename detail::block_traits<F>::type;
          iexchange_<Block>(f, mem);
      }

      inline void   process_collectives();

      inline
      ProxyWithLink proxy(int i, IExchangeInfo* iex = 0) const;

      //! return the number of local blocks
      unsigned int  size() const                        { return static_cast<unsigned int>(blocks_.size()); }
      void*         create() const                      { return blocks_.create(); }

      // accessors
      int           limit() const                       { return limit_; }
      int           threads() const                     { return threads_; }
      int           in_memory() const                   { return *blocks_.in_memory().const_access(); }

      void          set_threads(int threads__)
      {
          threads_ = threads__;
#if defined(DIY_NO_THREADS)
          threads_ = 1;
#endif
      }

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

      /** foreach_exchange **/
      struct CoroutineArg;

      inline static
      void          launch_process_block_coroutine();

      template<class Block>
      void          foreach_exchange_(const CoroutineCallback<Block>& f, bool remote, unsigned int stack_size);

      template<class F>
      void          foreach_exchange(const F& f, bool remote = false, unsigned int stack_size = 16*1024*1024);

    public:
      // Communicator functionality
      IncomingQueues&   incoming(int gid__)             { return incoming_[exchange_round_].map[gid__]; }
      OutgoingQueues&   outgoing(int gid__)             { return outgoing_[gid__]; }
      inline CollectivesList&  collectives(int gid__);
      inline CollectivesMap&   collectives();

      void              set_expected(int expected)      { expected_ = expected; }
      void              add_expected(int i)             { expected_ += i; }
      int               expected() const                { return expected_; }
      void              replace_link(int i, Link* link__) { expected_ -= links_[i]->size_unique(); delete links_[i]; links_[i] = link__; expected_ += links_[i]->size_unique(); }

    public:
      // Communicator functionality
      inline void       flush(bool remote, MemoryManagement mem = MemoryManagement());            // makes sure all the serialized queues migrate to their target processors

    private:
      // Communicator functionality
      inline void       comm_exchange(GidSendOrder& gid_order, MemoryManagement mem, IExchangeInfo*    iex = 0);
      inline void       rcomm_exchange(MemoryManagement mem);    // possibly called in between block computations
      inline bool       nudge(IExchangeInfo* iex = 0);
      inline void       send_queue(int from_gid, int to_gid, int to_proc, QueueRecord& qr, bool remote, MemoryManagement mem, IExchangeInfo* iex);
      inline void       send_outgoing_queues(GidSendOrder&   gid_order,
                                             bool            remote,
                                             MemoryManagement mem,
                                             IExchangeInfo*  iex = 0);
      inline void       check_incoming_queues(MemoryManagement mem, IExchangeInfo* iex = 0);
      inline GidSendOrder
                        order_gids();
      inline void       touch_queues();
      inline void       send_same_rank(int from, int to, QueueRecord& qr, MemoryManagement mem, IExchangeInfo* iex);
      inline void       send_different_rank(int from, int to, int proc, QueueRecord& qr, bool remote, IExchangeInfo* iex);

      inline InFlightRecv&         inflight_recv(int proc);
      inline InFlightSendsList&    inflight_sends();

      // iexchange commmunication
      inline void       icommunicate(IExchangeInfo* iex, MemoryManagement mem);     // async communication

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
      std::mt19937 mt_gen;          // mersenne_twister random number generator
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
#include "detail/master/foreach_exchange.hpp"

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
#if !defined(DIY_NO_THREADS)
  threads_(threads__ == -1 ? static_cast<int>(thread::hardware_concurrency()) : threads__),
#else
  threads_(1),
#endif
  storage_(storage),
  // Communicator functionality
  inflight_sends_(new InFlightSendsList),
  inflight_recvs_(new InFlightRecvsMap),
  collectives_(new CollectivesMap)
{
#ifdef DIY_NO_THREADS
  (void) threads__;
#endif
    comm_.duplicate(comm);

    // seed random number generator, broadcast seed, offset by rank
    std::random_device rd;                      // seed source for the random number engine
    unsigned int s = rd();
    diy::mpi::broadcast(communicator(), s, 0);
    std::mt19937 gen(s + communicator().rank());          // mersenne_twister random number generator
    mt_gen = gen;
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
      continue;

    IncomingQueues& in_qs = qmap_itr->second;
    for (auto& x : in_qs)
    {
        int from = x.first;
        for (QueueRecord& qr : *x.second.access())
        {
          if (queue_policy_->unload_incoming(*this, from, gid__, qr.size()))
          {
            log->debug("Unloading queue: {} <- {}", gid__, from);
            qr.unload(storage_);
          }
        }
    }
  }
}

void
diy::Master::
unload_outgoing(int gid__)
{
  OutgoingQueues& out_qs = outgoing_[gid__];
  for (auto& x : out_qs)
  {
      int to = x.first.gid;
      for (QueueRecord& qr : *x.second.access())
      {
        if (queue_policy_->unload_outgoing(*this, gid__, qr.size()))
        {
          log->debug("Unloading outgoing queue: {} -> {}", gid__, to);
          qr.unload(storage_);
        }
      }
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
  IncomingQueues& in_qs = incoming_[exchange_round_].map[gid__];
  for (auto& x : in_qs)
  {
      int from = x.first;
      auto access = x.second.access();
      if (!access->empty())
      {
          // NB: we only load the front queue, if we want to use out-of-core
          //     machinery with iexchange, this will require changes
          auto& qr = access->front();
          if (qr.external())
          {
            log->debug("Loading queue: {} <- {}", gid__, from);
            qr.load(storage_);
          }
      }
  }
}

void
diy::Master::
load_outgoing(int gid__)
{
  // TODO: we could adjust this mechanism to read directly from storage,
  //       bypassing an intermediate MemoryBuffer
  OutgoingQueues& out_qs = outgoing_[gid__];
  for (auto& x : out_qs)
  {
      int to      = x.first.gid;
      int to_rank = x.first.proc;
      auto access = x.second.access();
      if (!access->empty())
      {
          // NB: we only load the front queue, if we want to use out-of-core
          //     machinery with iexchange, this will require changes
          auto& qr = access->front();
          if (qr.external() && comm_.rank() != to_rank)     // skip queues to the same rank
          {
            log->debug("Loading queue: {} -> {}", gid__, to);
            qr.load(storage_);
          }
      }
  }
}

diy::Master::ProxyWithLink
diy::Master::
proxy(int i, IExchangeInfo* iex) const
{ return ProxyWithLink(Proxy(const_cast<Master*>(this), gid(i), iex), block(i), link(i)); }

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

  expected_ -= links_[i]->size_unique();
  delete link(i);   links_[i] = 0;
  std::swap(links_[i], links_.back());
  links_.pop_back();

  lids_.erase(gid(i));

  std::swap(gids_[i], gids_.back());
  gids_.pop_back();
  lids_[gid(i)] = i;

  return b;
}

bool
diy::Master::
has_incoming(int i) const
{
  const IncomingQueues& in_qs = const_cast<Master&>(*this).incoming_[exchange_round_].map[gid(i)];
  for (auto& x : in_qs)
  {
      auto access = x.second.const_access();
      if (!access->empty() && access->front().size() != 0)
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
exchange(bool remote, MemoryManagement mem)
{
  auto scoped = prof.scoped("exchange");
  DIY_UNUSED(scoped);

  execute();

  log->debug("Starting exchange");

  if (comm_.size() == 1)
  {
    remote = false;
  }

  // make sure there is a queue for each neighbor
  if (!remote)
      touch_queues();

  flush(remote, mem);
  log->debug("Finished exchange");
}

void
diy::Master::
touch_queues()
{
  for (int i = 0; i < (int)size(); ++i)
  {
      OutgoingQueues&  outgoing_queues  = outgoing_[gid(i)];
      for (BlockID target : link(i)->neighbors())
      {
          auto access = outgoing_queues[target].access();
          if (access->empty())
              access->emplace_back();
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
iexchange_(const ICallback<Block>& f, MemoryManagement mem)
{
    auto scoped = prof.scoped("iexchange");
    DIY_UNUSED(scoped);

#if !defined(DIY_NO_THREADS) && (!defined(DIY_USE_CALIPER) && defined(DIY_PROFILE))
    static_assert(false, "Cannot use DIY's internal profiler; it's not thread safe. Use caliper.");
#endif

    // prepare for next round
    incoming_.erase(exchange_round_);
    ++exchange_round_;
    exchange_round_annotation.set(exchange_round_);

    // touch the outgoing and incoming queues to make sure they exist
    for (unsigned i = 0; i < size(); ++i)
    {
      outgoing(gid(i));
      incoming(gid(i));
    }

    //IExchangeInfoDUD iexchange(comm_, min_queue_size, max_hold_time, fine, prof);
    IExchangeInfoCollective iex(comm_, prof);
    iex.add_work(size());                 // start with one work unit for each block

    thread comm_thread;
    if (threads() > 1)
        comm_thread = thread([this,&iex,mem]()
        {
            while(!iex.all_done())
            {
                icommunicate(&iex, mem);
                iex.control();
                //std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });

    auto empty_incoming = [this](int gid)
    {
        for (auto& x : incoming(gid))
            if (!x.second.access()->empty())
                return false;
        return true;
    };

    std::map<int, bool> done_result;
    do
    {
        size_t work_done = 0;
        DIY_UNUSED(work_done);
        for (int i = 0; i < static_cast<int>(size()); i++)     // for all blocks
        {
            int gid = this->gid(i);
            stats::Annotation::Guard g( stats::Annotation("diy.block").set(gid) );

            if (threads() == 1)
                icommunicate(&iex, mem);
            bool done = done_result[gid];
            if (!done || !empty_incoming(gid))
            {
                prof << "callback";
                iex.inc_work();       // even if we remove the queues, when constructing the proxy, we still have work to do
                {
                    ProxyWithLink cp = proxy(i, &iex);
                    done = f(block<Block>(i), cp);
                    if (done_result[gid] ^ done)        // status changed
                    {
                        if (done)
                            iex.dec_work();
                        else
                            iex.inc_work();
                    }
                }   // NB: we need cp to go out of scope and copy out its queues before we can decrement the work
                iex.dec_work();
                prof >> "callback";
                ++work_done;
            }
            done_result[gid] = done;
            log->debug("Done: {}", done);
        }

        if (threads() == 1)
        {
            prof << "iexchange-control";
            iex.control();
            prof >> "iexchange-control";
        }
        //else
        //if (work_done == 0)
        //    std::this_thread::sleep_for(std::chrono::microseconds(1));
    } while (!iex.all_done());
    log->info("[{}] ==== Leaving iexchange ====\n", iex.comm.rank());

    if (threads() > 1)
        comm_thread.join();

    //comm_.barrier();        // TODO: this is only necessary for DUD
    prof >> "consensus-time";

    outgoing_.clear();
}

/* Communicator */
void
diy::Master::
comm_exchange(GidSendOrder& gid_order, MemoryManagement mem, IExchangeInfo* iex)
{
    auto scoped = prof.scoped("comm-exchange");
    DIY_UNUSED(scoped);

    send_outgoing_queues(gid_order, false, mem, iex);

    while(nudge(iex))                         // kick requests
        ;

    check_incoming_queues(mem, iex);
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
rcomm_exchange(MemoryManagement mem)
{
    bool            done                = false;
    bool            ibarr_act           = false;
    mpi::request    ibarr_req;                      // mpi request associated with ibarrier

    // make a list of outgoing queues to send (the ones in memory come first)
    auto gid_order = order_gids();

    while (!done)
    {
        send_outgoing_queues(gid_order, true, mem, 0);

        // kick requests
        nudge();

        check_incoming_queues(mem);
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

    for (auto& x : outgoing_)
    {
        OutgoingQueues& out = x.second;
        if (!out.empty())
        {
            auto access = out.begin()->second.access();
            if (!access->empty() && !access->front().external())
            {
                order.list.push_front(x.first);
                continue;
            }
        }
        order.list.push_back(x.first);
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
icommunicate(IExchangeInfo* iex, MemoryManagement mem)
{
    auto scoped = prof.scoped("icommunicate");
    DIY_UNUSED(scoped);

    log->debug("Entering icommunicate()");

    auto gid_order = order_gids();

    // exchange
    comm_exchange(gid_order, mem, iex);

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
           QueueRecord&     qr,
           bool             remote,
           MemoryManagement mem,
           IExchangeInfo*   iex)
{
    stats::Annotation::Guard gb( stats::Annotation("diy.block").set(from_gid) );
    stats::Annotation::Guard gt( stats::Annotation("diy.to").set(to_gid) );
    stats::Annotation::Guard gq( stats::Annotation("diy.q-size").set(stats::Variant(static_cast<uint64_t>(qr.size()))) );

    // skip empty queues and hold queues shorter than some limit for some time
    assert(!iex || qr.size() != 0);
    log->debug("[{}] Sending queue: {} <- {} of size {}, iexchange = {}", comm_.rank(), to_gid, from_gid, qr.size(), iex ? 1 : 0);

    if (to_proc == comm_.rank())            // sending to same rank, simply swap buffers
        send_same_rank(from_gid, to_gid, qr, mem, iex);
    else                                    // sending to an actual message to a different rank
        send_different_rank(from_gid, to_gid, to_proc, qr, remote, iex);
}

void
diy::Master::
send_outgoing_queues(GidSendOrder&   gid_order,
                     bool            remote,            // TODO: are remote and iexchange mutually exclusive? If so, use single enum?
                     MemoryManagement mem,
                     IExchangeInfo*  iex)
{
    auto scoped = prof.scoped("send-outgoing-queues");
    DIY_UNUSED(scoped);

    if (iex)                                      // for iex, send queues from a single block
    {
        for (int from : gid_order.list)
        {
            OutgoingQueues& outgoing = this->outgoing(from);
            for (auto& x : outgoing)
            {
                BlockID to_block    = x.first;
                int     to_gid      = to_block.gid;
                int     to_proc     = to_block.proc;

                auto access = x.second.access();
                while (!access->empty())
                {
                    auto qr = std::move(access->front());
                    access->pop_front();
                    access.unlock();            // others can push on this queue, while we are working
                    assert(!qr.external());
                    log->debug("Processing queue:      {} <- {} of size {}", to_gid, from, qr.size());
                    send_queue(from, to_gid, to_proc, qr, remote, mem, iex);
                    access.lock();
                }
            }
        }
    }
    else                                                // normal mode: send all outgoing queues
    {
        while (inflight_sends().size() < gid_order.limit && !gid_order.empty())
        {
            int from_gid = gid_order.pop();

            load_outgoing(from_gid);

            OutgoingQueues& outgoing = outgoing_[from_gid];
            for (auto& x : outgoing)
            {
                BlockID to_block    = x.first;
                int     to_gid      = to_block.gid;
                int     to_proc     = to_block.proc;

                auto access = x.second.access();
                if (access->empty())
                    continue;

                // NB: send only front
                auto& qr = access->front();
                log->debug("Processing queue:      {} <- {} of size {}", to_gid, from_gid, qr.size());
                send_queue(from_gid, to_gid, to_proc, qr, remote, mem, iex);
                access->pop_front();
            }
        }
    }
}

void
diy::Master::
send_same_rank(int from, int to, QueueRecord& qr, MemoryManagement mem, IExchangeInfo*)
{
    auto scoped = prof.scoped("send-same-rank");

    log->debug("Moving queue in-place: {} <- {}", to, from);

    IncomingRound& current_incoming = incoming_[exchange_round_];

    auto access_incoming = current_incoming.map[to][from].access();

    // save blobs to copy them explicitly
    std::vector<BinaryBlob> blobs;
    qr.buffer().blobs.swap(blobs);
    qr.buffer().blob_position = 0;

    access_incoming->emplace_back(std::move(qr));
    QueueRecord& in_qr = access_incoming->back();

    // copy blobs explicitly; we cannot just move them in place, since we don't
    // own their memory and must guarantee that it's safe to free, once
    // exchange() is done
    for (BinaryBlob& blob : blobs)
    {
        char* p = mem.allocate(to, blob.size);
        mem.copy(p, blob.pointer.get(), blob.size);
        in_qr.buffer().save_binary_blob(p, blob.size, mem.deallocate);
    }

    if (!in_qr.external())
    {
        in_qr.reset();

        bool to_external = block(lid(to)) == 0;
        if (to_external)
        {
            log->debug("Unloading outgoing directly as incoming: {} <- {}", to, from);
            if (queue_policy_->unload_incoming(*this, from, to, in_qr.size()))
                in_qr.unload(storage_);
        }
    }

    ++current_incoming.received;
}

void
diy::Master::
send_different_rank(int from, int to, int proc, QueueRecord& qr, bool remote, IExchangeInfo* iex)
{
    auto scoped = prof.scoped("send-different-rank");

    assert(!qr.external());

    static const size_t MAX_MPI_MESSAGE_COUNT = INT_MAX;

    // sending to a different rank
    std::shared_ptr<MemoryBuffer> buffer = std::make_shared<MemoryBuffer>(qr.move());

    MessageInfo info{from, to, 1, exchange_round_, static_cast<int>(buffer->nblobs())};
    // size fits in one message
    if (Serialization<MemoryBuffer>::size(*buffer) + Serialization<MessageInfo>::size(info) <= MAX_MPI_MESSAGE_COUNT)
    {
        diy::save(*buffer, info);

        inflight_sends().emplace_back();
        auto& inflight_send = inflight_sends().back();

        inflight_send.info = info;
        if (remote || iex)
            inflight_send.request = comm_.issend(proc, tags::queue, buffer->buffer);
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

        {
          inflight_sends().emplace_back();
          auto& inflight_send = inflight_sends().back();

          inflight_send.info = info;
          if (remote || iex)
          {
              // add one unit of work for the entire large message (upon sending the head, not the individual pieces below)
              if (iex)
              {
                  iex->inc_work();
                  log->debug("[{}] Incrementing work when sending the leading piece\n", comm_.rank());
              }
              inflight_send.request = comm_.issend(proc, tags::queue, hb->buffer);
          }
          else
          {
              inflight_send.request = comm_.isend(proc, tags::queue, hb->buffer);
          }
          inflight_send.message = hb;
        }

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
            if (remote || iex)
            {
                if (iex)
                {
                    iex->inc_work();
                    log->debug("[{}] Incrementing work when sending non-leading piece\n", comm_.rank());
                }
                inflight_send.request = comm_.issend(proc, tags::queue, window);
            }
            else
                inflight_send.request = comm_.isend(proc, tags::queue, window);
            inflight_send.message = buffer;
        }
    }   // large message broken into pieces

    // send binary blobs
    for (size_t i = 0; i < buffer->nblobs(); ++i)
    {
        auto blob = buffer->load_binary_blob();
        assert(blob.size < MAX_MPI_MESSAGE_COUNT);      // for now assume blobs are small enough that we don't need to break them into multiple parts

        inflight_sends().emplace_back();
        auto& inflight_send = inflight_sends().back();

        inflight_send.info = info;

        detail::VectorWindow<char> window;
        window.begin = const_cast<char*>(blob.pointer.get());
        window.count = blob.size;

        if (remote || iex)
            inflight_send.request = comm_.issend(proc, tags::queue, window);
        else
            inflight_send.request = comm_.isend(proc, tags::queue, window);
        inflight_send.blob = std::move(blob);
    }
}

void
diy::Master::
check_incoming_queues(MemoryManagement mem, IExchangeInfo* iex)
{
    auto scoped = prof.scoped("check-incoming-queues");
    DIY_UNUSED(scoped);

    mpi::optional<mpi::status> ostatus = comm_.iprobe(mpi::any_source, tags::queue);
    while (ostatus)
    {
        InFlightRecv& ir = inflight_recv(ostatus->source());
        ir.mem = mem;

        if (iex)
            iex->inc_work();                      // increment work before sender's issend request can complete (so we are now responsible for the queue)
        bool first_message = ir.recv(comm_, *ostatus);  // possibly partial recv, in case of a multi-piece message
        if (!first_message && iex)
            iex->dec_work();

        if (ir.done)                // all pieces assembled
        {
            assert(ir.info.round >= exchange_round_);
            IncomingRound* in = &incoming_[ir.info.round];

            bool unload = ((ir.info.round == exchange_round_) ? (block(lid(ir.info.to)) == 0) : (limit_ != -1))
                          && queue_policy_->unload_incoming(*this, ir.info.from, ir.info.to, ir.message.size());

            ir.place(in, unload, storage_, iex);
            ir.reset();
        }

        ostatus = comm_.iprobe(mpi::any_source, tags::queue);
    }
}

void
diy::Master::
flush(bool remote, MemoryManagement mem)
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
      rcomm_exchange(mem);
  else
  {
      auto gid_order = order_gids();
      do
      {
          comm_exchange(gid_order, mem);

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

  process_collectives();
}

bool
diy::Master::
nudge(IExchangeInfo* iex)
{
  bool success = false;
  for (InFlightSendsList::iterator it = inflight_sends().begin(); it != inflight_sends().end();)
  {
    mpi::optional<mpi::status> ostatus = it->request.test();
    if (ostatus)
    {
      success = true;
      it = inflight_sends().erase(it);
      if (iex)
      {
          log->debug("[{}] message left, decrementing work", iex->comm.rank());
          iex->dec_work();                // this message is receiver's responsibility now
      }
    }
    else
    {
      ++it;
    }
  }
  return success;
}

#endif
