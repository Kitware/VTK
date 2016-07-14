#ifndef DIY_CRITICAL_RESOURCE_HPP
#define DIY_CRITICAL_RESOURCE_HPP

namespace diy
{
  // TODO: when not running under C++11, i.e., when lock_guard is TinyThread's
  //       lock_guard, and not C++11's unique_lock, this implementation might
  //       be buggy since the copy constructor is invoked when
  //       critical_resource::access() returns an instance of this class. Once
  //       the temporary is destroyed the mutex is unlocked. I'm not 100%
  //       certain of this because I'd expect a deadlock on copy constructor,
  //       but it's clearly not happening -- so I may be missing something.
  //       (This issue will take care of itself in DIY3 once we switch to C++11 completely.)
  template<class T, class Mutex>
  class resource_accessor
  {
    public:
                resource_accessor(T& x, Mutex& m):
                    x_(x), lock_(m)                         {}

      T&        operator*()                                 { return x_; }
      T*        operator->()                                { return &x_; }
      const T&  operator*() const                           { return x_; }
      const T*  operator->() const                          { return &x_; }

    private:
      T&                        x_;
      lock_guard<Mutex>         lock_;
  };

  template<class T, class Mutex = fast_mutex>
  class critical_resource
  {
    public:
      typedef           resource_accessor<T, Mutex>         accessor;
      typedef           resource_accessor<const T, Mutex>   const_accessor;     // eventually, try shared locking

    public:
                        critical_resource()                 {}
                        critical_resource(const T& x):
                            x_(x)                           {}

      accessor          access()                            { return accessor(x_, m_); }
      const_accessor    const_access() const                { return const_accessor(x_, m_); }

    private:
      T                 x_;
      mutable Mutex     m_;
  };
}


#endif
