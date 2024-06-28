/*
 * Copyright(C) 1999-2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

/*
  Scopeguard, by Andrei Alexandrescu and Petru Marginean, December 2000.
  Modified by Joshua Lehrer, FactSet Research Systems, November 2005.
*/

namespace Ioss {
  template <class T> class RefHolder
  {
    T &ref_;

  public:
    explicit RefHolder(T &ref) : ref_(ref) {}
    explicit   operator T &() const { return ref_; }
    RefHolder &operator=(const RefHolder &) = delete;
  };

  template <class T> inline RefHolder<T> ByRef(T &t) { return RefHolder<T>(t); }

  class IOSS_EXPORT ScopeGuardImplBase
  {
  public:
    ScopeGuardImplBase &operator=(const ScopeGuardImplBase &) = delete;

  protected:
    ~ScopeGuardImplBase() = default;
    ScopeGuardImplBase(const ScopeGuardImplBase &other) : dismissed_(other.dismissed_)
    {
      other.Dismiss();
    }

    template <typename J> static void SafeExecute(J &j)
    {
      if (!j.dismissed_) {
        try {
          j.Execute();
        }
        catch (...) {
        }
      }
    }

    mutable bool dismissed_{false};

  public:
    ScopeGuardImplBase() = default;
    void Dismiss() const { dismissed_ = true; }
  };

// typedef const ScopeGuardImplBase& ScopeGuard;
#ifndef _MSC_VER
  __attribute__((unused))
#endif
  typedef const ScopeGuardImplBase &ScopeGuard; // NOLINT

  template <typename F> class ScopeGuardImpl0 : public ScopeGuardImplBase
  {
  public:
    static ScopeGuardImpl0<F> MakeGuard(F fun) { return ScopeGuardImpl0<F>(fun); }
    ~ScopeGuardImpl0() { SafeExecute(*this); }
    void Execute() { fun_(); }

  protected:
    explicit ScopeGuardImpl0(F fun) : fun_(fun) {}
    F fun_;
  };

  template <typename F> inline ScopeGuardImpl0<F> MakeGuard(F fun)
  {
    return ScopeGuardImpl0<F>::MakeGuard(fun);
  }

  template <typename F, typename P1> class ScopeGuardImpl1 : public ScopeGuardImplBase
  {
  public:
    static ScopeGuardImpl1<F, P1> MakeGuard(F fun, P1 p1)
    {
      return ScopeGuardImpl1<F, P1>(fun, p1);
    }
    ~ScopeGuardImpl1() { SafeExecute(*this); }
    void Execute() { fun_(p1_); }

  protected:
    ScopeGuardImpl1(F fun, P1 p1) : fun_(fun), p1_(p1) {}
    F        fun_;
    const P1 p1_;
  };

  template <typename F, typename P1> inline ScopeGuardImpl1<F, P1> MakeGuard(F fun, P1 p1)
  {
    return ScopeGuardImpl1<F, P1>::MakeGuard(fun, p1);
  }

  template <typename F, typename P1, typename P2> class ScopeGuardImpl2 : public ScopeGuardImplBase
  {
  public:
    static ScopeGuardImpl2<F, P1, P2> MakeGuard(F fun, P1 p1, P2 p2)
    {
      return ScopeGuardImpl2<F, P1, P2>(fun, p1, p2);
    }
    ~ScopeGuardImpl2() { SafeExecute(*this); }
    void Execute() { fun_(p1_, p2_); }

  protected:
    ScopeGuardImpl2(F fun, P1 p1, P2 p2) : fun_(fun), p1_(p1), p2_(p2) {}
    F        fun_;
    const P1 p1_;
    const P2 p2_;
  };

  template <typename F, typename P1, typename P2>
  inline ScopeGuardImpl2<F, P1, P2> MakeGuard(F fun, P1 p1, P2 p2)
  {
    return ScopeGuardImpl2<F, P1, P2>::MakeGuard(fun, p1, p2);
  }

  template <typename F, typename P1, typename P2, typename P3>
  class ScopeGuardImpl3 : public ScopeGuardImplBase
  {
  public:
    static ScopeGuardImpl3<F, P1, P2, P3> MakeGuard(F fun, P1 p1, P2 p2, P3 p3)
    {
      return ScopeGuardImpl3<F, P1, P2, P3>(fun, p1, p2, p3);
    }
    ~ScopeGuardImpl3() { SafeExecute(*this); }
    void Execute() { fun_(p1_, p2_, p3_); }

  protected:
    ScopeGuardImpl3(F fun, P1 p1, P2 p2, P3 p3) : fun_(fun), p1_(p1), p2_(p2), p3_(p3) {}
    F        fun_;
    const P1 p1_;
    const P2 p2_;
    const P3 p3_;
  };

  template <typename F, typename P1, typename P2, typename P3>
  inline ScopeGuardImpl3<F, P1, P2, P3> MakeGuard(F fun, P1 p1, P2 p2, P3 p3)
  {
    return ScopeGuardImpl3<F, P1, P2, P3>::MakeGuard(fun, p1, p2, p3);
  }

  //************************************************************

  template <class Obj, typename MemFun> class ObjScopeGuardImpl0 : public ScopeGuardImplBase
  {
  public:
    static ObjScopeGuardImpl0<Obj, MemFun> MakeObjGuard(Obj &obj, MemFun memFun)
    {
      return ObjScopeGuardImpl0<Obj, MemFun>(obj, memFun);
    }
    ~ObjScopeGuardImpl0() { SafeExecute(*this); }
    void Execute() { (obj_.*memFun_)(); }

  protected:
    ObjScopeGuardImpl0(Obj &obj, MemFun memFun) : obj_(obj), memFun_(memFun) {}
    Obj   &obj_;
    MemFun memFun_;
  };

  template <class Obj, typename MemFun>
  inline ObjScopeGuardImpl0<Obj, MemFun> MakeObjGuard(Obj &obj, MemFun memFun)
  {
    return ObjScopeGuardImpl0<Obj, MemFun>::MakeObjGuard(obj, memFun);
  }

  template <typename Ret, class Obj1, class Obj2>
  inline ObjScopeGuardImpl0<Obj1, Ret (Obj2::*)()> MakeGuard(Ret (Obj2::*memFun)(), Obj1 &obj)
  {
    return ObjScopeGuardImpl0<Obj1, Ret (Obj2::*)()>::MakeObjGuard(obj, memFun);
  }

  template <typename Ret, class Obj1, class Obj2>
  inline ObjScopeGuardImpl0<Obj1, Ret (Obj2::*)()> MakeGuard(Ret (Obj2::*memFun)(), Obj1 *obj)
  {
    return ObjScopeGuardImpl0<Obj1, Ret (Obj2::*)()>::MakeObjGuard(*obj, memFun);
  }

  template <class Obj, typename MemFun, typename P1>
  class ObjScopeGuardImpl1 : public ScopeGuardImplBase
  {
  public:
    static ObjScopeGuardImpl1<Obj, MemFun, P1> MakeObjGuard(Obj &obj, MemFun memFun, P1 p1)
    {
      return ObjScopeGuardImpl1<Obj, MemFun, P1>(obj, memFun, p1);
    }
    ~ObjScopeGuardImpl1() { SafeExecute(*this); }
    void Execute() { (obj_.*memFun_)(p1_); }

  protected:
    ObjScopeGuardImpl1(Obj &obj, MemFun memFun, P1 p1) : obj_(obj), memFun_(memFun), p1_(p1) {}
    Obj     &obj_;
    MemFun   memFun_;
    const P1 p1_;
  };

  template <class Obj, typename MemFun, typename P1>
  inline ObjScopeGuardImpl1<Obj, MemFun, P1> MakeObjGuard(Obj &obj, MemFun memFun, P1 p1)
  {
    return ObjScopeGuardImpl1<Obj, MemFun, P1>::MakeObjGuard(obj, memFun, p1);
  }

  template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b>
  inline ObjScopeGuardImpl1<Obj1, Ret (Obj2::*)(P1a), P1b> MakeGuard(Ret (Obj2::*memFun)(P1a),
                                                                     Obj1 &obj, P1b p1)
  {
    return ObjScopeGuardImpl1<Obj1, Ret (Obj2::*)(P1a), P1b>::MakeObjGuard(obj, memFun, p1);
  }

  template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b>
  inline ObjScopeGuardImpl1<Obj1, Ret (Obj2::*)(P1a), P1b> MakeGuard(Ret (Obj2::*memFun)(P1a),
                                                                     Obj1 *obj, P1b p1)
  {
    return ObjScopeGuardImpl1<Obj1, Ret (Obj2::*)(P1a), P1b>::MakeObjGuard(*obj, memFun, p1);
  }

  template <class Obj, typename MemFun, typename P1, typename P2>
  class ObjScopeGuardImpl2 : public ScopeGuardImplBase
  {
  public:
    static ObjScopeGuardImpl2<Obj, MemFun, P1, P2> MakeObjGuard(Obj &obj, MemFun memFun, P1 p1,
                                                                P2 p2)
    {
      return ObjScopeGuardImpl2<Obj, MemFun, P1, P2>(obj, memFun, p1, p2);
    }
    ~ObjScopeGuardImpl2() { SafeExecute(*this); }
    void Execute() { (obj_.*memFun_)(p1_, p2_); }

  protected:
    ObjScopeGuardImpl2(Obj &obj, MemFun memFun, P1 p1, P2 p2)
        : obj_(obj), memFun_(memFun), p1_(p1), p2_(p2)
    {
    }
    Obj     &obj_;
    MemFun   memFun_;
    const P1 p1_;
    const P2 p2_;
  };

  template <class Obj, typename MemFun, typename P1, typename P2>
  inline ObjScopeGuardImpl2<Obj, MemFun, P1, P2> MakeObjGuard(Obj &obj, MemFun memFun, P1 p1, P2 p2)
  {
    return ObjScopeGuardImpl2<Obj, MemFun, P1, P2>::MakeObjGuard(obj, memFun, p1, p2);
  }

  template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a,
            typename P2b>
  inline ObjScopeGuardImpl2<Obj1, Ret (Obj2::*)(P1a, P2a), P1b, P2b>
  MakeGuard(Ret (Obj2::*memFun)(P1a, P2a), Obj1 &obj, P1b p1, P2b p2)
  {
    return ObjScopeGuardImpl2<Obj1, Ret (Obj2::*)(P1a, P2a), P1b, P2b>::MakeObjGuard(obj, memFun,
                                                                                     p1, p2);
  }

  template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a,
            typename P2b>
  inline ObjScopeGuardImpl2<Obj1, Ret (Obj2::*)(P1a, P2a), P1b, P2b>
  MakeGuard(Ret (Obj2::*memFun)(P1a, P2a), Obj1 *obj, P1b p1, P2b p2)
  {
    return ObjScopeGuardImpl2<Obj1, Ret (Obj2::*)(P1a, P2a), P1b, P2b>::MakeObjGuard(*obj, memFun,
                                                                                     p1, p2);
  }

#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2)        CONCATENATE_DIRECT(s1, s2)
#define ANONYMOUS_VARIABLE(str)    CONCATENATE(str, __LINE__)

#define ON_BLOCK_EXIT     ::Ioss::ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = ::Ioss::MakeGuard
#define ON_BLOCK_EXIT_OBJ ::Ioss::ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = ::Ioss::MakeObjGuard
} // namespace Ioss
