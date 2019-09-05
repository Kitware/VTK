#include <type_traits>

namespace diy
{
namespace mpi
{

    //! \ingroup MPI
    //! Simple wrapper around MPI window functions.
    template<class T>
    class window
    {
        static_assert(std::is_same<typename detail::is_mpi_datatype<T>::type, detail::true_type>::value, "Only MPI datatypes are allowed in windows");

        public:
            inline window(const communicator& comm, unsigned size);
            inline ~window();

            // moving is Ok
            window(window&&)      = default;
            window& operator=(window&&) = default;

            // cannot copy because of the buffer_
            window(const window&) = delete;
            window& operator=(const window&) = delete;

            inline void put(const T&              x, int rank, unsigned offset);
            inline void put(const std::vector<T>& x, int rank, unsigned offset);

            inline void get(T&              x, int rank, unsigned offset);
            inline void get(std::vector<T>& x, int rank, unsigned offset);

            inline void fence(int assert);

            inline void lock(int lock_type, int rank, int assert = 0);
            inline void unlock(int rank);

            inline void lock_all(int assert = 0);
            inline void unlock_all();

            inline void fetch_and_op(const T* origin, T* result, int rank, unsigned offset, MPI_Op op);
            inline void fetch(T& result, int rank, unsigned offset);
            inline void replace(const T& value, int rank, unsigned offset);

            inline void sync();

            inline void flush(int rank);
            inline void flush_all();
            inline void flush_local(int rank);
            inline void flush_local_all();

        private:
            std::vector<T>      buffer_;
            int                 rank_;
#ifndef DIY_NO_MPI
            MPI_Win             window_;
#endif
    };
} // mpi
} // diy

template<class T>
diy::mpi::window<T>::
window(const communicator& comm, unsigned size):
  buffer_(size), rank_(comm.rank())
{
#ifndef DIY_NO_MPI
    MPI_Win_create(buffer_.data(), buffer_.size()*sizeof(T), sizeof(T), MPI_INFO_NULL, comm, &window_);
#endif
}

template<class T>
diy::mpi::window<T>::
~window()
{
#ifndef DIY_NO_MPI
    MPI_Win_free(&window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
put(const T& x, int rank, unsigned offset)
{
#ifndef DIY_NO_MPI
    MPI_Put(address(x), count(x), datatype(x),
            rank,
            offset,
            count(x), datatype(x),
            window_);
#else
    buffer_[offset] = x;
#endif
}

template<class T>
void
diy::mpi::window<T>::
put(const std::vector<T>& x, int rank, unsigned offset)
{
#ifndef DIY_NO_MPI
    MPI_Put(address(x), count(x), datatype(x),
            rank,
            offset,
            count(x), datatype(x),
            window_);
#else
    for (size_t i = 0; i < x.size(); ++i)
        buffer_[offset + i] = x[i];
#endif
}

template<class T>
void
diy::mpi::window<T>::
get(T& x, int rank, unsigned offset)
{
#ifndef DIY_NO_MPI
    MPI_Get(address(x), count(x), datatype(x),
            rank,
            offset,
            count(x), datatype(x),
            window_);
#else
    x = buffer_[offset];
#endif
}

template<class T>
void
diy::mpi::window<T>::
get(std::vector<T>& x, int rank, unsigned offset)
{
#ifndef DIY_NO_MPI
    MPI_Get(address(x), count(x), datatype(x),
            rank,
            offset,
            count(x), datatype(x),
            window_);
#else
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = buffer_[offset + i];
#endif
}

template<class T>
void
diy::mpi::window<T>::
fence(int assert)
{
#ifndef DIY_NO_MPI
    MPI_Win_fence(assert, window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
lock(int lock_type, int rank, int assert)
{
#ifndef DIY_NO_MPI
    MPI_Win_lock(lock_type, rank, assert, window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
unlock(int rank)
{
#ifndef DIY_NO_MPI
    MPI_Win_unlock(rank, window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
lock_all(int assert)
{
#ifndef DIY_NO_MPI
    MPI_Win_lock_all(assert, window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
unlock_all()
{
#ifndef DIY_NO_MPI
    MPI_Win_unlock_all(window_);
#endif
}
template<class T>
void
diy::mpi::window<T>::
fetch_and_op(const T* origin, T* result, int rank, unsigned offset, MPI_Op op)
{
#ifndef DIY_NO_MPI
    MPI_Fetch_and_op(origin, result, datatype(*origin), rank, offset, op, window_);
#else
    DIY_UNSUPPORTED_MPI_CALL(MPI_Fetch_and_op);
#endif
}

template<class T>
void
diy::mpi::window<T>::
fetch(T& result, int rank, unsigned offset)
{
#ifndef DIY_NO_MPI
    T unused;
    fetch_and_op(&unused, &result, rank, offset, MPI_NO_OP);
#else
    result = buffer_[offset];
#endif
}

template<class T>
void
diy::mpi::window<T>::
replace(const T& value, int rank, unsigned offset)
{
#ifndef DIY_NO_MPI
    T unused;
    fetch_and_op(&value, &unused, rank, offset, MPI_REPLACE);
#else
    buffer_[offset] = value;
#endif
}

template<class T>
void
diy::mpi::window<T>::
sync()
{
#ifndef DIY_NO_MPI
    MPI_Win_sync(window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
flush(int rank)
{
#ifndef DIY_NO_MPI
    MPI_Win_flush(rank, window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
flush_all()
{
#ifndef DIY_NO_MPI
    MPI_Win_flush_all(window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
flush_local(int rank)
{
#ifndef DIY_NO_MPI
    MPI_Win_flush_local(rank, window_);
#endif
}

template<class T>
void
diy::mpi::window<T>::
flush_local_all()
{
#ifndef DIY_NO_MPI
    MPI_Win_flush_local_all(window_);
#endif
}
