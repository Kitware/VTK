#ifndef DIY_PROXY_HPP
#define DIY_PROXY_HPP

#include "coroutine.hpp"

namespace diy
{
  //! Communication proxy, used for enqueueing and dequeueing items for future exchange.
  struct Master::Proxy
  {
    template <class T>
    struct EnqueueIterator;

    using IncomingQueues = std::map<int,     MemoryBuffer>;
    using OutgoingQueues = std::map<BlockID, MemoryBuffer>;

                        Proxy(Master* master__, int gid__,
                              IExchangeInfo*  iexchange__ = 0):
                          gid_(gid__),
                          master_(master__),
                          iexchange_(iexchange__),
                          collectives_(&master__->collectives(gid__))
    {
        fill_incoming();

        // move outgoing_ back into proxy, in case it's a multi-foreach round
        if (!iexchange_)
            for (auto& x : master_->outgoing(gid_))
            {
                auto access = x.second.access();
                if (!access->empty())
                {
                    outgoing_.emplace(x.first, access->back().move());
                    access->pop_back();
                }
            }
    }

    // delete copy constructor to avoid coping incoming_ and outgoing_ (plus it
    // won't work otherwise because MemoryBuffer has a deleted copy
    // constructor)
                        Proxy(const Proxy&)     =delete;
                        Proxy(Proxy&&)          =default;
    Proxy&              operator=(const Proxy&) =delete;
    Proxy&              operator=(Proxy&&)      =default;

                        ~Proxy()
    {
        auto& outgoing = master_->outgoing(gid_);
        auto& incoming = master_->incoming(gid_);

        // copy out outgoing_
        for (auto& x : outgoing_)
        {
            outgoing[x.first].access()->emplace_back(std::move(x.second));
            if (iexchange_)
                iexchange_->inc_work();
        }

        // move incoming_ back into master, in case it's a multi-foreach round
        if (!iexchange_)
            for (auto& x : incoming_)
                incoming[x.first].access()->emplace_front(std::move(x.second));
    }

    void                init()
    {
        collectives_ = &master()->collectives(gid());
    }

    int                 gid() const                                     { return gid_; }

    bool                fill_incoming() const
    {
        bool exists = false;

        incoming_.clear();

        // fill incoming_
        for (auto& x : master_->incoming(gid_))
        {
            auto access = x.second.access();
            if (!access->empty())
            {
                exists = true;
                incoming_.emplace(x.first, access->front().move());
                access->pop_front();
                if (iexchange_)
                    iexchange_->dec_work();
            }
        }

        return exists;
    }

    //! Enqueue data whose size can be determined automatically, e.g., an STL vector.
    template<class T>
    void                enqueue(const BlockID&  to,                                     //!< target block (gid,proc)
                                const T&        x,                                      //!< data (eg. STL vector)
                                void (*save)(BinaryBuffer&, const T&) = &::diy::save    //!< optional serialization function
                               ) const
    {
        save(outgoing_[to], x);
    }

    //! Enqueue data whose size is given explicitly by the user, e.g., an array.
    template<class T>
    void                enqueue(const BlockID&  to,                                     //!< target block (gid,proc)
                                const T*        x,                                      //!< pointer to the data (eg. address of start of vector)
                                size_t          n,                                      //!< size in data elements (eg. ints)
                                void (*save)(BinaryBuffer&, const T&) = &::diy::save    //!< optional serialization function
                               ) const;

    void inline         enqueue_blob
                               (const BlockID&  to,                                     //!< target block (gid,proc)
                                const char*     x,                                      //!< pointer to the data
                                size_t          n                                       //!< size in data elements (eg. ints)
                               ) const;

    //! Dequeue data whose size can be determined automatically (e.g., STL vector) and that was
    //! previously enqueued so that diy knows its size when it is received.
    //! In this case, diy will allocate the receive buffer; the user does not need to do so.
    template<class T>
    void                dequeue(int             from,                                   //!< target block gid
                                T&              x,                                      //!< data (eg. STL vector)
                                void (*load)(BinaryBuffer&, T&) = &::diy::load          //!< optional serialization function
                               ) const
    { load(incoming_[from], x); }

    //! Dequeue an array of data whose size is given explicitly by the user.
    //! In this case, the user needs to allocate the receive buffer prior to calling dequeue.
    template<class T>
    void                dequeue(int             from,                                   //!< target block gid
                                T*              x,                                      //!< pointer to the data (eg. address of start of vector)
                                size_t          n,                                      //!< size in data elements (eg. ints)
                                void (*load)(BinaryBuffer&, T&) = &::diy::load          //!< optional serialization function
                               ) const;

    //! Dequeue data whose size can be determined automatically (e.g., STL vector) and that was
    //! previously enqueued so that diy knows its size when it is received.
    //! In this case, diy will allocate the receive buffer; the user does not need to do so.
    template<class T>
    void                dequeue(const BlockID&  from,                                   //!< target block (gid,proc)
                                T&              x,                                      //!< data (eg. STL vector)
                                void (*load)(BinaryBuffer&, T&) = &::diy::load          //!< optional serialization function
                               ) const                                  { dequeue(from.gid, x, load); }

    //! Dequeue an array of data whose size is given explicitly by the user.
    //! In this case, the user needs to allocate the receive buffer prior to calling dequeue.
    template<class T>
    void                dequeue(const BlockID&  from,                                   //!< target block (gid,proc)
                                T*              x,                                      //!< pointer to the data (eg. address of start of vector)
                                size_t          n,                                      //!< size in data elements (eg. ints)
                                void (*load)(BinaryBuffer&, T&) = &::diy::load          //!< optional serialization function
                               ) const                                  { dequeue(from.gid, x, n, load); }

    BinaryBlob inline   dequeue_blob
                               (int  from) const;

    template<class T>
    EnqueueIterator<T>  enqueuer(const T& x,
                                 void (*save)(BinaryBuffer&, const T&) = &::diy::save   ) const
    { return EnqueueIterator<T>(this, x, save); }

    IncomingQueues*     incoming() const                                { return &incoming_; }
    MemoryBuffer&       incoming(int from) const                        { return incoming_[from]; }
    inline void         incoming(std::vector<int>& v) const;            // fill v with every gid from which we have a message

    OutgoingQueues*     outgoing() const                                { return &outgoing_; }
    MemoryBuffer&       outgoing(const BlockID& to) const               { return outgoing_[to]; }

    inline bool         empty_incoming_queues() const;
    inline bool         empty_outgoing_queues() const;
    inline bool         empty_queues() const;

/**
 * \ingroup Communication
 * \brief Post an all-reduce collective using an existing communication proxy.
 * Available operators are:
 * maximum<T>, minimum<T>, std::plus<T>, std::multiplies<T>, std::logical_and<T>, and
 * std::logical_or<T>.
 */
    template<class T, class Op>
    inline void         all_reduce(const T& in,                  //!< local value being reduced
                                   Op op                         //!< operator
                                   ) const;
/**
 * \ingroup Communication
 * \brief Return the result of a proxy collective without popping it off the collectives list (same result would be returned multiple times). The list can be cleared with collectives()->clear().
 */
    template<class T>
    inline T            read() const;
/**
 * \ingroup Communication
 * \brief Return the result of a proxy collective; result is popped off the collectives list.
 */
    template<class T>
    inline T            get() const;

    template<class T>
    inline void         scratch(const T& in) const;

/**
 * \ingroup Communication
 * \brief Return the list of proxy collectives (values and operations)
 */
    CollectivesList*    collectives() const                             { return collectives_; }

    Master*             master() const                                  { return master_; }
    IExchangeInfo*      iexchange() const                               { return iexchange_; }

    // Coroutine machinery
    void                set_main(coroutine::cothread_t main)            { main_ = main; }
    void                yield() const                                   { coroutine::co_switch(main_); }
    void                set_done(bool x)                                { done_ = x; }
    bool                done() const                                    { return done_; }

    private:
      int               gid_;
      Master*           master_;
      IExchangeInfo*    iexchange_;

      // TODO: these are marked mutable to not have to undo consts on enqueue/dequeue, in case it breaks things;
      //       eventually, implement this change
      mutable IncomingQueues    incoming_;
      mutable OutgoingQueues    outgoing_;

      CollectivesList*  collectives_;

      coroutine::cothread_t main_;
      bool                  done_ = false;
  };

  template<class T>
  struct Master::Proxy::EnqueueIterator
  {
    typedef     void (*SaveT)(BinaryBuffer&, const T&);

                        EnqueueIterator(const Proxy* proxy, const T& x,
                                        SaveT save = &::diy::save<T>):
                            proxy_(proxy), x_(x), save_(save)               {}

    EnqueueIterator&    operator=(const BlockID& to)                        { proxy_->enqueue(to, x_, save_); return *this; }
    EnqueueIterator&    operator*()                                         { return *this; }
    EnqueueIterator&    operator++()                                        { return *this; }
    EnqueueIterator&    operator++(int)                                     { return *this; }

    private:
      const Proxy*  proxy_;
      const T&      x_;
      SaveT         save_;

  };

  struct Master::ProxyWithLink: public Master::Proxy
  {
            ProxyWithLink(Proxy&&         proxy,
                          void*           block__,
                          Link*           link__):
              Proxy(std::move(proxy)),
              block_(block__),
              link_(link__)                                         {}

      Link*   link() const                                          { return link_; }
      void*   block() const                                         { return block_; }

    private:
      void*             block_;
      Link*             link_;
  };
}                                           // diy namespace

void
diy::Master::Proxy::
incoming(std::vector<int>& v) const
{
  for (auto& x : incoming_)
    v.push_back(x.first);
}

bool
diy::Master::Proxy::
empty_incoming_queues() const
{
    for (auto& x : *incoming())
        if (x.second)
            return false;
    return true;
}

bool
diy::Master::Proxy::
empty_outgoing_queues() const
{
    for (auto& x : *outgoing())
        if (x.second.size())
            return false;
    return true;
}

bool
diy::Master::Proxy::
empty_queues() const
{
    return empty_incoming_queues() && empty_outgoing_queues();
}


template<class T, class Op>
void
diy::Master::Proxy::
all_reduce(const T& in, Op op) const
{
  collectives_->push_back(Collective(new detail::AllReduceOp<T,Op>(in, op)));
}

template<class T>
T
diy::Master::Proxy::
read() const
{
  T res;
  collectives_->front().result_out(&res);
  return res;
}

template<class T>
T
diy::Master::Proxy::
get() const
{
  T res = read<T>();
  collectives_->pop_front();
  return res;
}

template<class T>
void
diy::Master::Proxy::
scratch(const T& in) const
{
  collectives_->push_back(Collective(new detail::Scratch<T>(in)));
}

template<class T>
void
diy::Master::Proxy::
enqueue(const BlockID& to, const T* x, size_t n,
        void (*save)(BinaryBuffer&, const T&)) const
{
    BinaryBuffer&   bb  = outgoing_[to];
    if (save == (void (*)(BinaryBuffer&, const T&)) &::diy::save<T>)
        diy::save(bb, x, n);       // optimized for unspecialized types
    else
        for (size_t i = 0; i < n; ++i)
            save(bb, x[i]);
}

template<class T>
void
diy::Master::Proxy::
dequeue(int from, T* x, size_t n,
        void (*load)(BinaryBuffer&, T&)) const
{
    BinaryBuffer&   bb = incoming_[from];
    if (load == (void (*)(BinaryBuffer&, T&)) &::diy::load<T>)
        diy::load(bb, x, n);       // optimized for unspecialized types
    else
        for (size_t i = 0; i < n; ++i)
            load(bb, x[i]);
}

void
diy::Master::Proxy::
enqueue_blob(const BlockID& to, const char* x, size_t n) const
{
    BinaryBuffer&   bb  = outgoing_[to];
    bb.save_binary_blob(x,n);
}

diy::BinaryBlob
diy::Master::Proxy::
dequeue_blob(int from) const
{
    BinaryBuffer&   bb = incoming_[from];
    return bb.load_binary_blob();
}

#endif
