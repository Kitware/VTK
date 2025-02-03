#ifndef DIY_CRITICAL_RESOURCE_HPP
#define DIY_CRITICAL_RESOURCE_HPP

namespace diy
{
  template<class T, class Mutex>
  class resource_accessor
  {
    public:
                resource_accessor(T& x, Mutex& m):
                    x_(x), lock_(m)                         {}
                resource_accessor(resource_accessor&&)      = default;
                resource_accessor(const resource_accessor&) = delete;

      T&        operator*()                                 { return x_; }
      T*        operator->()                                { return &x_; }
      const T&  operator*() const                           { return x_; }
      const T*  operator->() const                          { return &x_; }

      void      lock()                                      { lock_.lock(); }
      void      unlock()                                    { lock_.unlock(); }

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
                        critical_resource(T&& x):
                            x_(std::move(x))                {}

      accessor          access()                            { return accessor(x_, m_); }
      const_accessor    const_access() const                { return const_accessor(x_, m_); }

    private:
      T                 x_;
      mutable Mutex     m_;
  };
}


#endif
