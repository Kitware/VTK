#ifndef DIY_MPI_WINODW_HPP
#define DIY_MPI_WINODW_HPP

#include "config.hpp"
#include "communicator.hpp"
#include "operations.hpp"

#include <type_traits>
#include <vector>

namespace diy
{
namespace mpi
{

#ifndef DIY_MPI_AS_LIB
constexpr int nocheck  = MPI_MODE_NOCHECK;
#else
DIY_MPI_EXPORT extern const int nocheck;
#endif

namespace detail
{

DIY_MPI_EXPORT_FUNCTION
DIY_MPI_Win win_allocate(const communicator& comm, void** base, unsigned size, int disp);

DIY_MPI_EXPORT_FUNCTION
DIY_MPI_Win win_create(const communicator& comm, void* base, unsigned size, int disp);

DIY_MPI_EXPORT_FUNCTION
void win_free(DIY_MPI_Win& win);

DIY_MPI_EXPORT_FUNCTION
void put(const DIY_MPI_Win& win,
         const void* data, int count, const datatype& type,
         int rank, unsigned offset);

DIY_MPI_EXPORT_FUNCTION
void get(const DIY_MPI_Win& win,
         void* data, int count, const datatype& type,
         int rank, unsigned offset);

DIY_MPI_EXPORT_FUNCTION
void fence(const DIY_MPI_Win& win, int assert);

DIY_MPI_EXPORT_FUNCTION
void lock(const DIY_MPI_Win& win, int lock_type, int rank, int assert);

DIY_MPI_EXPORT_FUNCTION
void unlock(const DIY_MPI_Win& win, int rank);

DIY_MPI_EXPORT_FUNCTION
void lock_all(const DIY_MPI_Win& win, int assert);

DIY_MPI_EXPORT_FUNCTION
void unlock_all(const DIY_MPI_Win& win);

DIY_MPI_EXPORT_FUNCTION
void fetch_and_op(const DIY_MPI_Win& win,
                  const void* origin, void* result, const datatype& type,
                  int rank, unsigned offset,
                  const operation& op);

DIY_MPI_EXPORT_FUNCTION
void fetch(const DIY_MPI_Win& win, void* result, const datatype& type, int rank, unsigned offset);

DIY_MPI_EXPORT_FUNCTION
void replace(const DIY_MPI_Win& win,
             const void* value, const datatype& type,
             int rank, unsigned offset);

DIY_MPI_EXPORT_FUNCTION
void sync(const DIY_MPI_Win& win);

DIY_MPI_EXPORT_FUNCTION
void flush(const DIY_MPI_Win& win, int rank);

DIY_MPI_EXPORT_FUNCTION
void flush_all(const DIY_MPI_Win& win);

DIY_MPI_EXPORT_FUNCTION
void flush_local(const DIY_MPI_Win& win, int rank);

DIY_MPI_EXPORT_FUNCTION
void flush_local_all(const DIY_MPI_Win& win);

} // detail

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
            inline window(window&&);
            inline window& operator=(window&&);

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

            inline void fetch_and_op(const T* origin, T* result, int rank, unsigned offset, const operation& op);
            inline void fetch(T& result, int rank, unsigned offset);
            inline void replace(const T& value, int rank, unsigned offset);

            inline void sync();

            inline void flush(int rank);
            inline void flush_all();
            inline void flush_local(int rank);
            inline void flush_local_all();

        private:
            void*               buffer_;
            int                 rank_;
            DIY_MPI_Win         window_;
    };

} // mpi
} // diy

template<class T>
diy::mpi::window<T>::
window(const diy::mpi::communicator& comm, unsigned size):
  buffer_(nullptr), rank_(comm.rank())
{
  window_ = detail::win_allocate(comm, &buffer_, static_cast<unsigned>(size*sizeof(T)), static_cast<int>(sizeof(T)));
}

template<class T>
diy::mpi::window<T>::
~window()
{
  if (buffer_)
    detail::win_free(window_);
}

template<class T>
diy::mpi::window<T>::
window(window&& rhs):
  buffer_(rhs.buffer_), rank_(rhs.rank_), window_(std::move(rhs.window_))
{
  rhs.buffer_ = nullptr;
  rhs.window_.reset();
}

template<class T>
diy::mpi::window<T>&
diy::mpi::window<T>::
operator=(window&& rhs)
{
  if (this == &rhs)
    return *this;

  if (buffer_)
    detail::win_free(window_);

  buffer_ = rhs.buffer_;
  rhs.buffer_ = nullptr;
  rank_ = rhs.rank_;
  window_ = std::move(rhs.window_);
  rhs.window_.reset();

  return *this;
}

template<class T>
void
diy::mpi::window<T>::
put(const T& x, int rank, unsigned offset)
{
  detail::put(window_, address(x), count(x), datatype_of(x), rank, offset);
}

template<class T>
void
diy::mpi::window<T>::
put(const std::vector<T>& x, int rank, unsigned offset)
{
  detail::put(window_, address(x), count(x), datatype_of(x), rank, offset);
}

template<class T>
void
diy::mpi::window<T>::
get(T& x, int rank, unsigned offset)
{
  detail::get(window_, address(x), count(x), datatype_of(x), rank, offset);
}

template<class T>
void
diy::mpi::window<T>::
get(std::vector<T>& x, int rank, unsigned offset)
{
  detail::get(window_, address(x), count(x), datatype_of(x), rank, offset);
}

template<class T>
void
diy::mpi::window<T>::
fence(int assert)
{
  detail::fence(window_, assert);
}

template<class T>
void
diy::mpi::window<T>::
lock(int lock_type, int rank, int assert)
{
  detail::lock(window_, lock_type, rank, assert);
}

template<class T>
void
diy::mpi::window<T>::
unlock(int rank)
{
  detail::unlock(window_, rank);
}

template<class T>
void
diy::mpi::window<T>::
lock_all(int assert)
{
  detail::lock_all(window_, assert);
}

template<class T>
void
diy::mpi::window<T>::
unlock_all()
{
  detail::unlock_all(window_);
}

template<class T>
void
diy::mpi::window<T>::
fetch_and_op(const T* origin, T* result, int rank, unsigned offset, const diy::mpi::operation& op)
{
  detail::fetch_and_op(window_, origin, result, datatype_of(*origin), rank, offset, op);
}

template<class T>
void
diy::mpi::window<T>::
fetch(T& result, int rank, unsigned offset)
{
  detail::fetch(window_, &result, datatype_of(result), rank, offset);
}

template<class T>
void
diy::mpi::window<T>::
replace(const T& value, int rank, unsigned offset)
{
  detail::replace(window_, &value, datatype_of(value), rank, offset);
}

template<class T>
void
diy::mpi::window<T>::
sync()
{
  detail::sync(window_);
}

template<class T>
void
diy::mpi::window<T>::
flush(int rank)
{
  detail::flush(window_, rank);
}

template<class T>
void
diy::mpi::window<T>::
flush_all()
{
  detail::flush_all(window_);
}

template<class T>
void
diy::mpi::window<T>::
flush_local(int rank)
{
  detail::flush_local(window_, rank);
}

template<class T>
void
diy::mpi::window<T>::
flush_local_all()
{
  detail::flush_local_all(window_);
}

#ifndef DIY_MPI_AS_LIB
#include "window.cpp"
#endif

#endif // DIY_MPI_WINODW_HPP
