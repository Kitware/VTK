namespace diy
{
namespace mpi
{
  template<class T>
  struct optional
  {
                optional():
                  init_(false)                  {}

                optional(const T& v):
                  init_(true)                   { new(buf_) T(v); }

                optional(const optional& o):
                  init_(o.init_)                { if (init_) new(buf_) T(*o);  }

                ~optional()                     { if (init_) clear(); }

    inline
    optional&   operator=(const optional& o);

                operator bool() const           { return init_; }

    T&          operator*()                     { return *reinterpret_cast<T*>(buf_); }
    const T&    operator*() const               { return *reinterpret_cast<const T*>(buf_); }

    T*          operator->()                    { return &(operator*()); }
    const T*    operator->() const              { return &(operator*()); }

    private:
      void      clear()                         { reinterpret_cast<T*>(buf_)->~T(); }

    private:
      bool init_;
      char buf_[sizeof(T)];
  };
}
}

template<class T>
diy::mpi::optional<T>&
diy::mpi::optional<T>::
operator=(const optional& o)
{
  if (init_)
    clear();
  init_ = o.init_;
  if (init_)
    new (buf_) T(*o);

  return *this;
}
