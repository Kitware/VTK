#ifndef DIY_MASTER_HPP
#define DIY_MASTER_HPP

#include <vector>
#include <map>
#include <list>
#include <deque>
#include <algorithm>

#include "link.hpp"
#include "collection.hpp"

// Communicator functionality
#include "mpi.hpp"
#include "serialization.hpp"
#include "detail/collectives.hpp"
#include "time.hpp"

#include "thread.hpp"

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

      template<class Block>
      struct Binder;

      // Commands
      struct BaseCommand;

      template<class Block, class Functor, class Skip>
      struct Command;

      typedef std::vector<BaseCommand*>     Commands;

      // Skip
      struct SkipNoIncoming;
      struct NeverSkip { bool    operator()(int i, const Master& master) const   { return false; } };

      // Collection
      typedef Collection::Create            CreateBlock;
      typedef Collection::Destroy           DestroyBlock;
      typedef Collection::Save              SaveBlock;
      typedef Collection::Load              LoadBlock;

    public:
      // Communicator types
      struct Proxy;
      struct ProxyWithLink;

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
        bool    unload_incoming(const Master& master, int from, int to, size_t sz) const    { return sz > size; }
        bool    unload_outgoing(const Master& master, int from, size_t sz) const            { return sz > size*master.outgoing_count(from); }

        size_t  size;
      };

      struct InFlight
      {
        MemoryBuffer        message;
        mpi::request        request;

        // for debug purposes:
        int from, to;
      };
      struct Collective;
      struct tags       { enum { queue }; };

      typedef           std::list<InFlight>                 InFlightList;
      typedef           std::list<int>                      ToSendList;         // [gid]
      typedef           std::list<Collective>               CollectivesList;
      typedef           std::map<int, CollectivesList>      CollectivesMap;     // gid          -> [collectives]


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
                    Master(mpi::communicator    comm,          //!< communicator
                           int                  threads  = 1,  //!< number of threads DIY can use
                           int                  limit    = -1, //!< number of blocks to store in memory
                           CreateBlock          create   = 0,  //!< block create function; master manages creation if create != 0
                           DestroyBlock         destroy  = 0,  //!< block destroy function; master manages destruction if destroy != 0
                           ExternalStorage*     storage  = 0,  //!< storage object (path, method, etc.) for storing temporary blocks being shuffled in/out of core
                           SaveBlock            save     = 0,  //!< block save function; master manages saving if save != 0
                           LoadBlock            load     = 0,  //!< block load function; master manages loading if load != 0
                           QueuePolicy*         q_policy = new QueueSizePolicy(4096)): //!< policy for managing message queues specifies maximum size of message queues to keep in memory
                      blocks_(create, destroy, storage, save, load),
                      queue_policy_(q_policy),
                      limit_(limit),
                      threads_(threads == -1 ? thread::hardware_concurrency() : threads),
                      storage_(storage),
                      // Communicator functionality
                      comm_(comm),
                      inflight_size_(0),
                      expected_(0),
                      received_(0),
                      immediate_(true)
                                                        {}
                    ~Master()                           { set_immediate(true); clear(); delete queue_policy_; }
      inline void   clear();
      inline void   destroy(int i)                      { if (blocks_.own()) blocks_.destroy(i); }

      inline int    add(int gid, void* b, Link* l);     //!< add a block
      inline void*  release(int i);                     //!< release ownership of the block

      //!< return the `i`-th block
      inline void*  block(int i) const                  { return blocks_.find(i); }
      template<class Block>
      Block*        block(int i) const                  { return static_cast<Block*>(block(i)); }
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

      //! return the `i`-th block, loading it if necessary
      void*         get(int i)                          { return blocks_.get(i); }
      //! return gid of the `i`-th block
      int           gid(int i) const                    { return gids_[i]; }
      //! return the local id of the local block with global id gid, or -1 if not local
      int           lid(int gid) const                  { return local(gid) ?  lids_.find(gid)->second : -1; }
      //! whether the block with global id gid is local
      bool          local(int gid) const                { return lids_.find(gid) != lids_.end(); }

      //! exchange the queues between all the blocks (collective operation)
      inline void   exchange();
      inline void   process_collectives();

      inline
      ProxyWithLink proxy(int i) const;

      //! return the number of local blocks
      unsigned      size() const                        { return blocks_.size(); }
      void*         create() const                      { return blocks_.create(); }

      // accessors
      int           limit() const                       { return limit_; }
      int           threads() const                     { return threads_; }
      int           in_memory() const                   { return *blocks_.in_memory().const_access(); }

      void          set_threads(int threads)            { threads_ = threads; }

      CreateBlock   creator() const                     { return blocks_.creator(); }
      DestroyBlock  destroyer() const                   { return blocks_.destroyer(); }
      LoadBlock     loader() const                      { return blocks_.loader(); }
      SaveBlock     saver() const                       { return blocks_.saver(); }

      //! call `f` with every block
      template<class Functor>
      void          foreach(const Functor& f)           { foreach<void>(f); }

      template<class Block, class Functor>
      void          foreach(const Functor& f)           { foreach<Block>(f, (void*) 0); }

      template<class Functor, class T>
      void          foreach(const Functor& f, T* aux)   { foreach<void>(f, aux); }

      template<class Block, class Functor, class T>
      void          foreach(const Functor& f, T* aux)   { foreach<Block>(f, NeverSkip(), aux); }

      template<class Functor, class Skip>
      void          foreach(const Functor& f, const Skip& skip, void* aux = 0)  { foreach<void>(f,skip,aux); }

      template<class Block, class Functor, class Skip>
      void          foreach(const Functor& f, const Skip& skip, void* aux = 0);

      // calls to member functions
      template<class Block>
      void          foreach(void (Block::*f)(const ProxyWithLink&, void*))                   { foreach<Block>(f, (void*) 0); }

      template<class Block, class T>
      void          foreach(void (Block::*f)(const ProxyWithLink&, void*), T* aux)           { foreach<Block>(f, NeverSkip(), aux); }

      template<class Block, class Skip>
      void          foreach(void (Block::*f)(const ProxyWithLink&, void*), const Skip& skip, void* aux = 0) { foreach<Block>(Binder<Block>(f), skip, aux); }

      inline void   execute();

      bool          immediate() const                   { return immediate_; }
      void          set_immediate(bool i)               { if (i && !immediate_) execute(); immediate_ = i; }

    public:
      // Communicator functionality
      IncomingQueues&   incoming(int gid)               { return incoming_[gid].queues; }
      OutgoingQueues&   outgoing(int gid)               { return outgoing_[gid].queues; }
      CollectivesList&  collectives(int gid)            { return collectives_[gid]; }
      size_t            incoming_count(int gid) const   { IncomingQueuesMap::const_iterator it = incoming_.find(gid); if (it == incoming_.end()) return 0; return it->second.queues.size(); }
      size_t            outgoing_count(int gid) const   { OutgoingQueuesMap::const_iterator it = outgoing_.find(gid); if (it == outgoing_.end()) return 0; return it->second.queues.size(); }

      void              set_expected(int expected)      { expected_ = expected; }
      void              add_expected(int i)             { expected_ += i; }
      int               expected() const                { return expected_; }
      void              replace_link(int i, Link* link) { expected_ -= links_[i]->size_unique(); delete links_[i]; links_[i] = link; expected_ += links_[i]->size_unique(); }

    public:
      // Communicator functionality
      inline void       flush();            // makes sure all the serialized queues migrate to their target processors

    private:
      // Communicator functionality
      inline void       comm_exchange(ToSendList& to_send, int out_queues_limit);     // possibly called in between block computations
      inline bool       nudge();

      void              cancel_requests();              // TODO

      // debug
      inline void       show_incoming_records() const;

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
      IncomingQueuesMap     incoming_;
      OutgoingQueuesMap     outgoing_;
      InFlightList          inflight_;
      size_t                inflight_size_;
      CollectivesMap        collectives_;
      int                   expected_;
      int                   received_;
      bool                  immediate_;
      Commands              commands_;

    private:
      fast_mutex            add_mutex_;
  };

  struct Master::BaseCommand
  {
      virtual       ~BaseCommand()                                                  {}      // to delete derived classes
      virtual void  execute(void* b, const ProxyWithLink& cp) const                 =0;
      virtual bool  skip(int i, const Master& master) const                         =0;
  };

  template<class Block, class Functor, class Skip>
  struct Master::Command: public BaseCommand
  {
            Command(const Functor& f_, const Skip& s_, void* aux_):
                f(f_), s(s_), aux(aux_)                                             {}

      void  execute(void* b, const ProxyWithLink& cp) const                         { f(static_cast<Block*>(b), cp, aux); }
      bool  skip(int i, const Master& m) const                                      { return s(i,m); }

      Functor f;
      Skip    s;
      void*   aux;
  };

  template<class Block>
  struct Master::Binder
  {
    typedef     void (Block::*MemberFn)(const ProxyWithLink&, void*);
                Binder(MemberFn f): f_(f)                                           {}
    void        operator()(Block* b, const ProxyWithLink& cp, void* aux) const      { if (b) (b->*f_)(cp, aux); }
    MemberFn    f_;
  };

  struct Master::SkipNoIncoming
  { bool operator()(int i, const Master& master) const   { return !master.has_incoming(i); } };

  struct Master::Collective
  {
            Collective():
              cop_(0)                           {}
            Collective(detail::CollectiveOp* cop):
              cop_(cop)                         {}
            // this copy constructor is very ugly, but need it to insert Collectives into a list
            Collective(const Collective& other):
              cop_(0)                           { swap(const_cast<Collective&>(other)); }
            ~Collective()                       { delete cop_; }

    void    init()                              { cop_->init(); }
    void    swap(Collective& other)             { std::swap(cop_, other.cop_); }
    void    update(const Collective& other)     { cop_->update(*other.cop_); }
    void    global(const mpi::communicator& c)  { cop_->global(c); }
    void    copy_from(Collective& other) const  { cop_->copy_from(*other.cop_); }
    void    result_out(void* x) const           { cop_->result_out(x); }

    detail::CollectiveOp*                       cop_;

    private:
    Collective& operator=(const Collective& other);
  };
}

#include "proxy.hpp"

// --- ProcessBlock ---
struct diy::Master::ProcessBlock
{
          ProcessBlock(Master&                    master_,
                       const std::deque<int>&     blocks_,
                       int                        local_limit_,
                       critical_resource<int>&    idx_):
              master(master_),
              blocks(blocks_),
              local_limit(local_limit_),
              idx(idx_)
          {}

  void    process()
  {
    //fprintf(stderr, "Processing with thread: %d\n",  (int) this_thread::get_id());

    std::vector<int>      local;
    do
    {
      int cur = (*idx.access())++;

      if ((size_t)cur >= blocks.size())
          return;

      int i = blocks[cur];
      if (master.block(i))
      {
          if (local.size() == (size_t)local_limit)
              master.unload(local);
          local.push_back(i);
      }

      //fprintf(stderr, "Processing block: %d\n", master.gid(i));

      bool skip_block = true;
      for (size_t cmd = 0; cmd < master.commands_.size(); ++cmd)
      {
          if (!master.commands_[cmd]->skip(i, master))
          {
              skip_block = false;
              break;
          }
      }
      if (skip_block)
      {
          if (master.block(i) == 0)
              master.load_queues(i);      // even though we are skipping the block, the queues might be necessary

          for (size_t cmd = 0; cmd < master.commands_.size(); ++cmd)
          {
              master.commands_[cmd]->execute(0, master.proxy(i));  // 0 signals that we are skipping the block (even if it's loaded)

              // no longer need them, so get rid of them, rather than risk reloading
              master.incoming_[master.gid(i)].queues.clear();
              master.incoming_[master.gid(i)].records.clear();
          }

          if (master.block(i) == 0)
              master.unload_queues(i);    // even though we are skipping the block, the queues might be necessary
      }
      else
      {
          if (master.block(i) == 0)                             // block unloaded
          {
              if (local.size() == (size_t)local_limit)                    // reached the local limit
                  master.unload(local);

              master.load(i);
              local.push_back(i);
          }

          for (size_t cmd = 0; cmd < master.commands_.size(); ++cmd)
          {
              master.commands_[cmd]->execute(master.block(i), master.proxy(i));

              // no longer need them, so get rid of them
              master.incoming_[master.gid(i)].queues.clear();
              master.incoming_[master.gid(i)].records.clear();
          }
      }
    } while(true);

    // TODO: invoke opportunistic communication
    //       don't forget to adjust Master::exchange()
  }

  static void run(void* bf)                   { static_cast<ProcessBlock*>(bf)->process(); }

  Master&                 master;
  const std::deque<int>&  blocks;
  int                     local_limit;
  critical_resource<int>& idx;
};
// --------------------

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
  //fprintf(stdout, "Unloading block: %d\n", gid(i));

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
unload_incoming(int gid)
{
  IncomingQueuesRecords& in_qrs = incoming_[gid];
  for (InQueueRecords::iterator it = in_qrs.records.begin(); it != in_qrs.records.end(); ++it)
  {
    QueueRecord& qr = it->second;
    if (queue_policy_->unload_incoming(*this, it->first, gid, qr.size))
    {
        //fprintf(stderr, "Unloading queue: %d <- %d\n", gid, it->first);
        qr.external = storage_->put(in_qrs.queues[it->first]);
    }
  }
}

void
diy::Master::
unload_outgoing(int gid)
{
  OutgoingQueuesRecord& out_qr = outgoing_[gid];

  size_t out_queues_size = sizeof(size_t);   // map size
  size_t count = 0;
  for (OutgoingQueues::iterator it = out_qr.queues.begin(); it != out_qr.queues.end(); ++it)
  {
    if (it->first.proc == comm_.rank()) continue;

    out_queues_size += sizeof(BlockID);     // target
    out_queues_size += sizeof(size_t);      // buffer.position
    out_queues_size += sizeof(size_t);      // buffer.size
    out_queues_size += it->second.size();   // buffer contents
    ++count;
  }
  if (queue_policy_->unload_outgoing(*this, gid, out_queues_size - sizeof(size_t)))
  {
      //fprintf(stderr, "Unloading outgoing queues: %d -> ...; size = %lu\n", gid, out_queues_size);
      MemoryBuffer  bb;     bb.reserve(out_queues_size);
      diy::save(bb, count);

      for (OutgoingQueues::iterator it = out_qr.queues.begin(); it != out_qr.queues.end();)
      {
        if (it->first.proc == comm_.rank())
        {
          // treat as incoming
          if (queue_policy_->unload_incoming(*this, gid, it->first.gid, it->second.size()))
          {
            QueueRecord& qr = out_qr.external_local[it->first];
            qr.size = it->second.size();
            qr.external = storage_->put(it->second);

            out_qr.queues.erase(it++);
            continue;
          } // else keep in memory
        } else
        {
          diy::save(bb, it->first);
          diy::save(bb, it->second);

          out_qr.queues.erase(it++);
          continue;
        }
        ++it;
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
  //fprintf(stdout, "Loading block: %d\n", gid(i));

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
load_incoming(int gid)
{
  IncomingQueuesRecords& in_qrs = incoming_[gid];
  for (InQueueRecords::iterator it = in_qrs.records.begin(); it != in_qrs.records.end(); ++it)
  {
    QueueRecord& qr = it->second;
    if (qr.external != -1)
    {
        //fprintf(stderr, "Loading queue: %d <- %d\n", gid, it->first);
        storage_->get(qr.external, in_qrs.queues[it->first]);
        qr.external = -1;
    }
  }
}

void
diy::Master::
load_outgoing(int gid)
{
  // TODO: we could adjust this mechanism to read directly from storage,
  //       bypassing an intermediate MemoryBuffer
  OutgoingQueuesRecord& out_qr = outgoing_[gid];
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


int
diy::Master::
add(int gid, void* b, Link* l)
{
  if (*blocks_.in_memory().const_access() == limit_)
    unload_all();

  lock_guard<fast_mutex>    lock(add_mutex_);       // allow to add blocks from multiple threads

  blocks_.add(b);
  links_.push_back(l);
  gids_.push_back(gid);

  int lid = gids_.size() - 1;
  lids_[gid] = lid;
  add_expected(l->size_unique()); // NB: at every iteration we expect a message from each unique neighbor

  return lid;
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
  const IncomingQueuesRecords& in_qrs = const_cast<Master&>(*this).incoming_[gid(i)];
  for (InQueueRecords::const_iterator it = in_qrs.records.begin(); it != in_qrs.records.end(); ++it)
  {
    const QueueRecord& qr = it->second;
    if (qr.size != 0)
        return true;
  }
  return false;
}

template<class Block, class Functor, class Skip>
void
diy::Master::
foreach(const Functor& f, const Skip& skip, void* aux)
{
    commands_.push_back(new Command<Block, Functor, Skip>(f, skip, aux));

    if (immediate())
        execute();
}

void
diy::Master::
execute()
{
  //fprintf(stderr, "Entered execute()\n");
  //show_incoming_records();

  // touch the outgoing and incoming queues as well as collectives to make sure they exist
  for (unsigned i = 0; i < size(); ++i)
  {
    outgoing(gid(i));
    incoming(gid(i));           // implicitly touches queue records
    collectives(gid(i));
  }

  if (commands_.empty())
      return;

  // Order the blocks, so the loaded ones come first
  std::deque<int>   blocks;
  for (unsigned i = 0; i < size(); ++i)
    if (block(i) == 0)
        blocks.push_back(i);
    else
        blocks.push_front(i);

  // don't use more threads than we can have blocks in memory
  int num_threads;
  int blocks_per_thread;
  if (limit_ == -1)
  {
    num_threads = threads_;
    blocks_per_thread = size();
  }
  else
  {
    num_threads = std::min(threads_, limit_);
    blocks_per_thread = limit_/num_threads;
  }

  // idx is shared
  critical_resource<int> idx(0);

  typedef                 ProcessBlock                                   BlockFunctor;
  if (num_threads > 1)
  {
    // launch the threads
    typedef               std::pair<thread*, BlockFunctor*>               ThreadFunctorPair;
    typedef               std::list<ThreadFunctorPair>                    ThreadFunctorList;
    ThreadFunctorList     threads;
    for (unsigned i = 0; i < (unsigned)num_threads; ++i)
    {
        BlockFunctor* bf = new BlockFunctor(*this, blocks, blocks_per_thread, idx);
        threads.push_back(ThreadFunctorPair(new thread(&BlockFunctor::run, bf), bf));
    }

    // join the threads
    for(ThreadFunctorList::iterator it = threads.begin(); it != threads.end(); ++it)
    {
        thread*           t  = it->first;
        BlockFunctor*     bf = it->second;
        t->join();
        delete t;
        delete bf;
    }
  } else
  {
      BlockFunctor bf(*this, blocks, blocks_per_thread, idx);
      BlockFunctor::run(&bf);
  }

  // clear incoming queues
  incoming_.clear();

  if (limit() != -1 && in_memory() > limit())
  {
    fprintf(stderr, "Fatal: %d blocks in memory, with limit %d\n", in_memory(), limit());
    std::abort();
  }

  // clear commands
  for (size_t i = 0; i < commands_.size(); ++i)
      delete commands_[i];
  commands_.clear();
}

void
diy::Master::
exchange()
{
  execute();

  //fprintf(stdout, "Starting exchange\n");

  // make sure there is a queue for each neighbor
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

  flush();
  //fprintf(stdout, "Finished exchange\n");
}

/* Communicator */
void
diy::Master::
comm_exchange(ToSendList& to_send, int out_queues_limit)
{
  // isend outgoing queues, up to the out_queues_limit
  while(inflight_size_ < (size_t)out_queues_limit && !to_send.empty())
  {
    int from = to_send.front();

    // deal with external_local queues
    for (OutQueueRecords::iterator it = outgoing_[from].external_local.begin(); it != outgoing_[from].external_local.end(); ++it)
    {
      int to = it->first.gid;

      //fprintf(stderr, "Processing local queue: %d <- %d\n", to, from);
      //fprintf(stderr, "   size:    %lu\n",     it->second.size);

      QueueRecord& in_qr  = incoming_[to].records[from];
      bool in_external  = block(lid(to)) == 0;

      if (in_external)
          in_qr = it->second;
      else
      {
          // load the queue
          in_qr.size     = it->second.size;
          in_qr.external = -1;

          MemoryBuffer bb;
          storage_->get(it->second.external, bb);

          incoming_[to].queues[from].swap(bb);
      }
      ++received_;
    }
    outgoing_[from].external_local.clear();

    if (outgoing_[from].external != -1)
      load_outgoing(from);
    to_send.pop_front();

    OutgoingQueues& outgoing = outgoing_[from].queues;
    for (OutgoingQueues::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
    {
      BlockID to_proc = it->first;
      int     to      = to_proc.gid;
      int     proc    = to_proc.proc;

      //fprintf(stderr, "Processing queue: %d <- %d\n", to, from);
      //fprintf(stderr, "   size:    %lu\n",     outgoing_[from].queues[to_proc].size());

      // There may be local outgoing queues that remained in memory
      if (proc == comm_.rank())     // sending to ourselves: simply swap buffers
      {
        //fprintf(stderr, "Moving queue in-place: %d <- %d\n", to, from);

        QueueRecord& in_qr  = incoming_[to].records[from];
        bool in_external  = block(lid(to)) == 0;
        if (in_external)
        {
          //fprintf(stderr, "Unloading outgoing directly as incoming: %d <- %d\n", to, from);
          MemoryBuffer& bb = it->second;
          in_qr.size = bb.size();
          if (queue_policy_->unload_incoming(*this, from, to, in_qr.size))
            in_qr.external = storage_->put(bb);
          else
          {
            MemoryBuffer& in_bb = incoming_[to].queues[from];
            in_bb.swap(bb);
            in_bb.reset();
            in_qr.external = -1;
          }
        } else        // !in_external
        {
          //fprintf(stderr, "Swapping in memory: %d <- %d\n", to, from);
          MemoryBuffer& bb = incoming_[to].queues[from];
          bb.swap(it->second);
          bb.reset();
          in_qr.size = bb.size();
          in_qr.external = -1;
        }

        ++received_;
        continue;
      }

      inflight_.push_back(InFlight()); ++inflight_size_;
      inflight_.back().from = from;
      inflight_.back().to   = to;
      MemoryBuffer& bb = inflight_.back().message;
      bb.swap(it->second);
      diy::save(bb, std::make_pair(from, to));
      inflight_.back().request = comm_.isend(proc, tags::queue, bb.buffer);
    }
  }

  // kick requests
  while(nudge());

  // check incoming queues
  mpi::optional<mpi::status>        ostatus = comm_.iprobe(mpi::any_source, tags::queue);
  while(ostatus)
  {
    MemoryBuffer bb;
    comm_.recv(ostatus->source(), tags::queue, bb.buffer);

    std::pair<int,int> from_to;
    diy::load_back(bb, from_to);
    int from = from_to.first;
    int to   = from_to.second;

    int size     = bb.size();
    int external = -1;

    incoming_[to].queues[from] = MemoryBuffer();
    if (block(lid(to)) != 0 || !queue_policy_->unload_incoming(*this, from, to, size))
    {
        incoming_[to].queues[from].swap(bb);
        incoming_[to].queues[from].reset();     // buffer position = 0
    } else if (queue_policy_->unload_incoming(*this, from, to, size))
    {
        //fprintf(stderr, "Directly unloading queue %d <- %d\n", to, from);
        external = storage_->put(bb);           // unload directly
    }
    incoming_[to].records[from] = QueueRecord(size, external);

    ++received_;

    ostatus = comm_.iprobe(mpi::any_source, tags::queue);
  }
}

void
diy::Master::
flush()
{
#ifdef DEBUG
  time_type start = get_time();
  unsigned wait = 1;
#endif

  // make a list of outgoing queues to send (the ones in memory come first)
  ToSendList    to_send;
  for (OutgoingQueuesMap::iterator it = outgoing_.begin(); it != outgoing_.end(); ++it)
  {
    OutgoingQueuesRecord& out = it->second;
    if (out.external == -1)
        to_send.push_front(it->first);
    else
        to_send.push_back(it->first);
  }
  //fprintf(stderr, "to_send.size(): %lu\n", to_send.size());

  // XXX: we probably want a cleverer limit than block limit times average number of queues per block
  // XXX: with queues we could easily maintain a specific space limit
  int out_queues_limit;
  if (limit_ == -1 || size() == 0)
    out_queues_limit = to_send.size();
  else
    out_queues_limit = std::max((size_t) 1, to_send.size()/size()*limit_);      // average number of queues per block * in-memory block limit

  do
  {
    comm_exchange(to_send, out_queues_limit);

#ifdef DEBUG
    time_type cur = get_time();
    if (cur - start > wait*1000)
    {
        fprintf(stderr, "Waiting in flush [%d]: %lu - %d out of %d\n",
                        comm_.rank(), inflight_size_, received_, expected_);
        wait *= 2;
    }
#endif
  } while (!inflight_.empty() || received_ < expected_ || !to_send.empty());

  outgoing_.clear();

  //fprintf(stderr, "Done in flush\n");
  //show_incoming_records();

  process_collectives();
  comm_.barrier();

  received_ = 0;
}

void
diy::Master::
process_collectives()
{
  if (collectives_.empty())
      return;

  typedef       CollectivesList::iterator       CollectivesIterator;
  std::vector<CollectivesIterator>  iters;
  std::vector<int>                  gids;
  for (CollectivesMap::iterator cur = collectives_.begin(); cur != collectives_.end(); ++cur)
  {
    gids.push_back(cur->first);
    iters.push_back(cur->second.begin());
  }

  while (iters[0] != collectives_.begin()->second.end())
  {
    iters[0]->init();
    for (unsigned j = 1; j < iters.size(); ++j)
    {
      // NB: this assumes that the operations are commutative
      iters[0]->update(*iters[j]);
    }
    iters[0]->global(comm_);        // do the mpi collective

    for (unsigned j = 1; j < iters.size(); ++j)
    {
      iters[j]->copy_from(*iters[0]);
      ++iters[j];
    }

    ++iters[0];
  }
}

bool
diy::Master::
nudge()
{
  bool success = false;
  for (InFlightList::iterator it = inflight_.begin(); it != inflight_.end(); ++it)
  {
    mpi::optional<mpi::status> ostatus = it->request.test();
    if (ostatus)
    {
      success = true;
      InFlightList::iterator rm = it;
      --it;
      inflight_.erase(rm); --inflight_size_;
    }
  }
  return success;
}

void
diy::Master::
show_incoming_records() const
{
  for (IncomingQueuesMap::const_iterator it = incoming_.begin(); it != incoming_.end(); ++it)
  {
    const IncomingQueuesRecords& in_qrs = it->second;
    for (InQueueRecords::const_iterator cur = in_qrs.records.begin(); cur != in_qrs.records.end(); ++cur)
    {
      const QueueRecord& qr = cur->second;
      fprintf(stderr, "%d <- %d: (size,external) = (%lu,%d)\n",
                      it->first, cur->first,
                      qr.size,
                      qr.external);
    }
    for (IncomingQueues::const_iterator cur = in_qrs.queues.begin(); cur != in_qrs.queues.end(); ++cur)
    {
      fprintf(stderr, "%d <- %d: queue.size() = %lu\n",
                      it->first, cur->first,
                      const_cast<IncomingQueuesRecords&>(in_qrs).queues[cur->first].size());
      }
  }
}

#endif
