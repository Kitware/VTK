namespace diy
{
    struct Master::MessageInfo
    {
        int from, to;
        int nparts;
        int round;
        int nblobs;
    };

    struct Master::InFlightSend
    {
        std::shared_ptr<MemoryBuffer> message;
        BinaryBlob                    blob;
        mpi::request                  request;

        MessageInfo info;           // for debug purposes
    };

    struct Master::InFlightRecv
    {
        MemoryBuffer    message;
        MessageInfo     info { -1, -1, -1, -1, -1 };
        bool            done = false;
        MemoryManagement mem;

        inline bool     recv(mpi::communicator& comm, const mpi::status& status);
        inline void     place(IncomingRound* in, bool unload, ExternalStorage* storage, IExchangeInfo* iexchange);
        void            reset()
        {
            MemoryManagement mem_ = mem;
            *this = InFlightRecv();
            mem = mem_;
        }
    };

    struct Master::InFlightRecvsMap: public std::map<int, InFlightRecv>
    {};

    struct Master::InFlightSendsList: public std::list<InFlightSend>
    {};

    struct Master::GidSendOrder
    {
        size_t              size() const                        { return list.size(); }
        bool                empty() const                       { return list.empty(); }
        int                 pop()                               { int x = list.front(); list.pop_front(); return x; }

        std::list<int>      list;
        size_t              limit = 0;
    };

    // VectorWindow is used to send and receive subsets of a contiguous array in-place
    namespace detail
    {
        template <typename T>
        struct VectorWindow
        {
            T *begin;
            size_t count;
        };
    } // namespace detail

    namespace mpi
    {
    namespace detail
    {
        template<typename T>  struct is_mpi_datatype< diy::detail::VectorWindow<T> > { typedef true_type type; };

        template <typename T>
        struct mpi_datatype< diy::detail::VectorWindow<T> >
        {
            using VecWin = diy::detail::VectorWindow<T>;
            static diy::mpi::datatype   datatype()                { return get_mpi_datatype<T>(); }
            static const void*          address(const VecWin& x)  { return x.begin; }
            static void*                address(VecWin& x)        { return x.begin; }
            static int                  count(const VecWin& x)    { return static_cast<int>(x.count); }
        };
    }
    } // namespace mpi::detail
} // namespace diy


/** InFlightRecv **/

diy::Master::InFlightRecv&
diy::Master::
inflight_recv(int proc)
{
    return (*inflight_recvs_)[proc];
}

diy::Master::InFlightSendsList&
diy::Master::inflight_sends()
{
    return *inflight_sends_;
}

// receive message described by status
bool
diy::Master::InFlightRecv::
recv(mpi::communicator& comm, const mpi::status& status)
{
    bool result = false;            // indicates whether this is the first (and possibly only) message of a given queue
    if (info.from == -1)            // uninitialized
    {
        MemoryBuffer bb;
        comm.recv(status.source(), status.tag(), bb.buffer);

        diy::load_back(bb, info);
        info.nparts--;
        if (info.nparts > 0)        // multi-part message
        {
            size_t msg_size;
            diy::load(bb, msg_size);
            message.buffer.reserve(msg_size);
        } else
            message.swap(bb);

        result = true;
    }
    else if (info.nparts > 0)
    {
        size_t start_idx = message.buffer.size();
        size_t count = status.count<char>();
        message.buffer.resize(start_idx + count);

        detail::VectorWindow<char> window;
        window.begin = &message.buffer[start_idx];
        window.count = count;

        comm.recv(status.source(), status.tag(), window);

        info.nparts--;
    } else if (info.nblobs > 0)
    {
        size_t count = status.count<char>();
        detail::VectorWindow<char> window;

        char* buffer = mem.allocate(info.to, count);

        window.begin = buffer;
        window.count = count;

        comm.recv(status.source(), status.tag(), window);

        message.save_binary_blob(buffer, count, mem.deallocate);

        info.nblobs--;
    }

    if (info.nparts == 0 && info.nblobs == 0)
        done = true;

    return result;
}

// once the InFlightRecv is done, place it either out of core or in the appropriate incoming queue
void
diy::Master::InFlightRecv::
place(IncomingRound* in, bool unload, ExternalStorage* storage, IExchangeInfo*)
{
    int from        = info.from;
    int to          = info.to;

    message.reset();

    auto access = in->map[to][from].access();
    access->emplace_back(std::move(message));

    if (unload)
    {
        get_logger()->debug("Directly unloading queue {} <- {}", to, from);
        access->back().unload(storage);
    }

    ++(in->received);
}
