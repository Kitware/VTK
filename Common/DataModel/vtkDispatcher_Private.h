/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDispatcher.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////
#ifndef vtkDispatcher_Private_h
#define vtkDispatcher_Private_h

#include <typeinfo>
#include <cassert>
#include <memory>

namespace vtkDispatcherPrivate
{
////////////////////////////////////////////////////////////////////////////////
// Dispatch helper for reference functors
////////////////////////////////////////////////////////////////////////////////
template <class BaseLhs,
          class SomeLhs,
          typename RT,
          class CastLhs,
          class Fun>
class FunctorRefDispatcherHelper
{
  Fun& fun_;
public:
  typedef RT ResultType;

  FunctorRefDispatcherHelper(const FunctorRefDispatcherHelper& rhs) : fun_(rhs.fun_) {}
  FunctorRefDispatcherHelper(Fun& f) : fun_(f) {}

  ResultType operator()(BaseLhs& lhs)
  {
    return fun_(CastLhs::Cast(lhs));
  }
private:
  FunctorRefDispatcherHelper& operator =(const FunctorRefDispatcherHelper& b);
};

////////////////////////////////////////////////////////////////////////////////
// Dispatch helper
////////////////////////////////////////////////////////////////////////////////
template <class BaseLhs,
          class SomeLhs,
          typename RT,
          class CastLhs,
          class Fun>
class FunctorDispatcherHelper
{
  Fun fun_;
public:
  typedef RT ResultType;

  FunctorDispatcherHelper(const FunctorDispatcherHelper& rhs) : fun_(rhs.fun_) {}
  FunctorDispatcherHelper(Fun fun) : fun_(fun) {}

  ResultType operator()(BaseLhs& lhs)
  {
    return fun_(CastLhs::Cast(lhs));
  }
};


////////////////////////////////////////////////////////////////////////////////
// Parent class for all FunctorImpl, helps hide functor template args
////////////////////////////////////////////////////////////////////////////////
template <typename R, typename P1>
class FunctorImpl{
  public:
    typedef R ResultType;
    typedef P1 Parm1;

    virtual ~FunctorImpl() {}
    virtual R operator()(P1&) = 0;
    virtual FunctorImpl* DoClone() const = 0;

    template <class U>
    static U* Clone(U* pObj)
    {
        if (!pObj) return 0;
        U* pClone = static_cast<U*>(pObj->DoClone());
        assert(typeid(*pClone) == typeid(*pObj));
        return pClone;
    }
  protected:
    FunctorImpl() {}
    FunctorImpl(const FunctorImpl&) {}
  private:
    FunctorImpl& operator =(const FunctorImpl&) VTK_DELETE_FUNCTION;
};

////////////////////////////////////////////////////////////////////////////////
// Impl functor that calls a user functor
////////////////////////////////////////////////////////////////////////////////
template <class ParentFunctor,typename Fun>
class FunctorHandler: public ParentFunctor::Impl
{
  typedef typename ParentFunctor::Impl Base;
public:
  typedef typename Base::ResultType ResultType;
  typedef typename Base::Parm1 Parm1;

  FunctorHandler(Fun& fun) : f_(fun) {}
  virtual ~FunctorHandler() {}

  ResultType operator()(Parm1& p1)
  { return f_(p1); }
  virtual FunctorHandler* DoClone() const { return new FunctorHandler(*this); }

private:
  Fun f_;
  FunctorHandler(const FunctorHandler &b) : ParentFunctor::Impl(b), f_(b.f_) {}
  FunctorHandler& operator =(const FunctorHandler& b) VTK_DELETE_FUNCTION;
};


////////////////////////////////////////////////////////////////////////////////
// Functor wrapper class
////////////////////////////////////////////////////////////////////////////////
template <typename R,typename Parm1>
class Functor
{
public:
  typedef FunctorImpl<R, Parm1> Impl;
  typedef R ResultType;

#if defined(VTK_HAS_STD_UNIQUE_PTR)
  Functor() : spImpl_()
#else
  Functor() : spImpl_(0)
#endif
    {}

  Functor(const Functor& rhs) : spImpl_(Impl::Clone(rhs.spImpl_.get()))
    {}

  template <typename Fun>
  Functor(Fun fun)
  : spImpl_(new FunctorHandler<Functor,Fun>(fun))
    {}

  Functor& operator=(const Functor& rhs)
  {
      Functor copy(rhs);
#if defined(VTK_HAS_STD_UNIQUE_PTR)
      spImpl_.swap(copy.spImpl_);
#else
      // swap auto_ptrs by hand
      Impl* p = spImpl_.release();
      spImpl_.reset(copy.spImpl_.release());
      copy.spImpl_.reset(p);
#endif
      return *this;
  }


  ResultType operator()(Parm1& p1)
    { return  (*spImpl_)(p1); }
private:
#if defined(VTK_HAS_STD_UNIQUE_PTR)
  std::unique_ptr<Impl> spImpl_;
#else
  std::auto_ptr<Impl> spImpl_;
#endif

};

}


namespace vtkDoubleDispatcherPrivate
{

////////////////////////////////////////////////////////////////////////////////
// Dispatch helper
////////////////////////////////////////////////////////////////////////////////
template <class BaseLhs, class BaseRhs,
          class SomeLhs, class SomeRhs,
          typename RT,
          class CastLhs, class CastRhs,
          class Fun>
class FunctorRefDispatcherHelper
{
  Fun& fun_;
public:
  typedef RT ResultType;
  FunctorRefDispatcherHelper(const FunctorRefDispatcherHelper& rhs) : fun_(rhs.fun_) {}
  FunctorRefDispatcherHelper(Fun& fun) : fun_(fun) {}

  ResultType operator()(BaseLhs& lhs, BaseRhs& rhs)
  {
    return fun_(CastLhs::Cast(lhs), CastRhs::Cast(rhs));
  }
private:
  FunctorRefDispatcherHelper& operator =(const FunctorRefDispatcherHelper& b);
};

template <class BaseLhs, class BaseRhs,
          class SomeLhs, class SomeRhs,
          typename RT,
          class CastLhs, class CastRhs,
          class Fun>
class FunctorDoubleDispatcherHelper
{
  Fun fun_;
public:
  typedef RT ResultType;
  FunctorDoubleDispatcherHelper(const FunctorDoubleDispatcherHelper& rhs) : fun_(rhs.fun_) {}
  FunctorDoubleDispatcherHelper(Fun fun) : fun_(fun) {}

  ResultType operator()(BaseLhs& lhs, BaseRhs& rhs)
  {
    return fun_(CastLhs::Cast(lhs), CastRhs::Cast(rhs));
  }

};

////////////////////////////////////////////////////////////////////////////////
// Parent class for all FunctorImpl, helps hide functor template args
////////////////////////////////////////////////////////////////////////////////
template <typename R, typename P1, typename P2>
class FunctorImpl{
  public:
    typedef R ResultType;
    typedef P1 Parm1;
    typedef P2 Parm2;

    virtual ~FunctorImpl() {}
    virtual R operator()(P1&,P2&) = 0;
    virtual FunctorImpl* DoClone() const = 0;

    template <class U>
    static U* Clone(U* pObj)
    {
        if (!pObj) return 0;
        U* pClone = static_cast<U*>(pObj->DoClone());
        assert(typeid(*pClone) == typeid(*pObj));
        return pClone;
    }
  protected:
    FunctorImpl() {}
    FunctorImpl(const FunctorImpl&) {}
  private:
    FunctorImpl& operator =(const FunctorImpl&) VTK_DELETE_FUNCTION;
};

////////////////////////////////////////////////////////////////////////////////
// Impl functor that calls a user functor
////////////////////////////////////////////////////////////////////////////////
template <class ParentFunctor,typename Fun>
class FunctorHandler: public ParentFunctor::Impl
{
  typedef typename ParentFunctor::Impl Base;
public:
  typedef typename Base::ResultType ResultType;
  typedef typename Base::Parm1 Parm1;
  typedef typename Base::Parm2 Parm2;

  FunctorHandler(const Fun& fun) : f_(fun) {}
  virtual ~FunctorHandler() {}

  ResultType operator()(Parm1& p1,Parm2& p2)
  { return f_(p1,p2); }

  virtual FunctorHandler* DoClone() const { return new FunctorHandler(*this); }

private:
  Fun f_;
  FunctorHandler(const FunctorHandler &b) : ParentFunctor::Impl(b), f_(b.f_) {}
  FunctorHandler& operator =(const FunctorHandler& b) VTK_DELETE_FUNCTION;
};

////////////////////////////////////////////////////////////////////////////////
// Functor wrapper class
////////////////////////////////////////////////////////////////////////////////
template <typename R,typename Parm1, typename Parm2>
class Functor
{
public:
  typedef FunctorImpl<R, Parm1,Parm2> Impl;
  typedef R ResultType;

#if defined(VTK_HAS_STD_UNIQUE_PTR)
  Functor() : spImpl_()
#else
  Functor() : spImpl_(0)
#endif
    {}

  Functor(const Functor& rhs) : spImpl_(Impl::Clone(rhs.spImpl_.get()))
    {}

  template <typename Fun>
  Functor(const Fun& fun)
  : spImpl_(new FunctorHandler<Functor,Fun>(fun))
    {}

  Functor& operator=(const Functor& rhs)
  {
      Functor copy(rhs);
#if defined(VTK_HAS_STD_UNIQUE_PTR)
      spImpl_.swap(copy.spImpl_);
#else      // swap auto_ptrs by hand
      Impl* p = spImpl_.release();
      spImpl_.reset(copy.spImpl_.release());
      copy.spImpl_.reset(p);
#endif
      return *this;
  }

  ResultType operator()(Parm1& p1,Parm2& p2)
    { return  (*spImpl_)(p1,p2); }
private:
#if defined(VTK_HAS_STD_UNIQUE_PTR)
  std::unique_ptr<Impl> spImpl_;
#else
  std::auto_ptr<Impl> spImpl_;
#endif
};
}

namespace vtkDispatcherCommon
{

template <class To, class From>
struct DynamicCaster
{
  static To& Cast(From& obj)
  {
    return dynamic_cast<To&>(obj);
  }

  static To* Cast(From* obj)
  {
    return dynamic_cast<To*>(obj);
  }
};

template <class To, class From>
struct vtkCaster
{
  static To& Cast(From& obj)
  {
    return *(To::SafeDownCast(&obj));
  }

  static To* Cast(From* obj)
  {
    return To::SafeDownCast(obj);
  }
};

class TypeInfo
{
public:
  // Constructors
  TypeInfo(); // needed for containers
  TypeInfo(const std::type_info&); // non-explicit

  // Access for the wrapped std::type_info
  const std::type_info& Get() const;
  // Compatibility functions
  bool before(const TypeInfo& rhs) const;
  const char* name() const;

private:
  const std::type_info* pInfo_;
};

// Implementation

inline TypeInfo::TypeInfo()
{
  class Nil {};
  pInfo_ = &typeid(Nil);
  assert(pInfo_);
}

inline TypeInfo::TypeInfo(const std::type_info& ti)
  : pInfo_(&ti)
  { assert(pInfo_); }

inline bool TypeInfo::before(const TypeInfo& rhs) const
{
  assert(pInfo_);
  // type_info::before return type is int in some VC libraries
  return pInfo_->before(*rhs.pInfo_) != 0;
}

inline const std::type_info& TypeInfo::Get() const
{
  assert(pInfo_);
  return *pInfo_;
}

inline const char* TypeInfo::name() const
{
  assert(pInfo_);
  return pInfo_->name();
}

// Comparison operators

inline bool operator==(const TypeInfo& lhs, const TypeInfo& rhs)
// type_info::operator== return type is int in some VC libraries
  { return (lhs.Get() == rhs.Get()) != 0; }

inline bool operator<(const TypeInfo& lhs, const TypeInfo& rhs)
  { return lhs.before(rhs); }

inline bool operator!=(const TypeInfo& lhs, const TypeInfo& rhs)
  { return !(lhs == rhs); }

inline bool operator>(const TypeInfo& lhs, const TypeInfo& rhs)
  { return rhs < lhs; }

inline bool operator<=(const TypeInfo& lhs, const TypeInfo& rhs)
  { return !(lhs > rhs); }

inline bool operator>=(const TypeInfo& lhs, const TypeInfo& rhs)
  { return !(lhs < rhs); }

}

#endif // vtkDispatcherPrivate_h
// VTK-HeaderTest-Exclude: vtkDispatcher_Private.h
