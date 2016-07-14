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

    T&          operator*()                     { return *static_cast<T*>(address()); }
    const T&    operator*() const               { return *static_cast<const T*>(address()); }

    T*          operator->()                    { return &(operator*()); }
    const T*    operator->() const              { return &(operator*()); }

    private:
      void      clear()                         { static_cast<T*>(address())->~T(); }

      void*         address()                   { return buf_; }
      const void*   address() const             { return buf_; }

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
