/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTools.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPTools - A set of parallel (multi-threaded) utility functions.
// .SECTION Description
// vtkSMPTools provides a set of utility functions that can
// be used to parallelize parts of VTK code using multiple threads.
// There are several back-end implementations of parallel functionality
// (currently Sequential, TBB and X-Kaapi) that actual execution is
// delegated to.

#ifndef __vtkSMPTools_h__
#define __vtkSMPTools_h__

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include "vtkSMPThreadLocal.h" // For Initialized

class vtkSMPTools;

#include "vtkSMPToolsInternal.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef __WRAP__
namespace vtk
{
namespace detail
{
namespace smp
{
template <typename T>
class vtkSMPTools_Has_Initialize
{
  typedef char (&no_type)[1];
  typedef char (&yes_type)[2];
  template <typename U, void (U::*)()> struct V {};
  template <typename U> static yes_type check(V<U, &U::Initialize>*);
  template <typename U> static no_type check(...);
public:
  static bool const value = sizeof(check<T>(0)) == sizeof(yes_type);
};

template <typename T>
class vtkSMPTools_Has_Initialize_const
{
  typedef char (&no_type)[1];
  typedef char (&yes_type)[2];
  template <typename U, void (U::*)() const> struct V {};
  template <typename U> static yes_type check(V<U, &U::Initialize>*);
  template <typename U> static no_type check(...);
public:
  static bool const value = sizeof(check<T>(0)) == sizeof(yes_type);
};

template <typename Functor, bool Init>
struct vtkSMPTools_FunctorInternal;

template <typename Functor>
struct vtkSMPTools_FunctorInternal<Functor, false>
{
  Functor& F;
  vtkSMPTools_FunctorInternal(Functor& f): F(f) {}
  void Execute(vtkIdType first, vtkIdType last)
  {
    this->F(first, last);
  }
  void For(vtkIdType first, vtkIdType last, vtkIdType grain)
  {
    vtk::detail::smp::vtkSMPTools_Impl_For(first, last, grain, *this);
  }
  vtkSMPTools_FunctorInternal<Functor, false>& operator=(
    const vtkSMPTools_FunctorInternal<Functor, false>&);
  vtkSMPTools_FunctorInternal<Functor, false>(
    const vtkSMPTools_FunctorInternal<Functor, false>&);
};

template <typename Functor>
struct vtkSMPTools_FunctorInternal<Functor, true>
{
  Functor& F;
  vtkSMPThreadLocal<unsigned char> Initialized;
  vtkSMPTools_FunctorInternal(Functor& f): F(f), Initialized(0) {}
  void Execute(vtkIdType first, vtkIdType last)
  {
    unsigned char& inited = this->Initialized.Local();
    if (!inited)
      {
      this->F.Initialize();
      inited = 1;
      }
    this->F(first, last);
  }
  void For(vtkIdType first, vtkIdType last, vtkIdType grain)
  {
    vtk::detail::smp::vtkSMPTools_Impl_For(first, last, grain, *this);
    this->F.Reduce();
  }
  vtkSMPTools_FunctorInternal<Functor, true>& operator=(
    const vtkSMPTools_FunctorInternal<Functor, true>&);
  vtkSMPTools_FunctorInternal<Functor, true>(
    const vtkSMPTools_FunctorInternal<Functor, true>&);
};

template <typename Functor>
class vtkSMPTools_Lookup_For
{
  static bool const init = vtkSMPTools_Has_Initialize<Functor>::value;
public:
  typedef vtkSMPTools_FunctorInternal<Functor, init> type;
};

template <typename Functor>
class vtkSMPTools_Lookup_For<Functor const>
{
  static bool const init = vtkSMPTools_Has_Initialize_const<Functor>::value;
public:
  typedef vtkSMPTools_FunctorInternal<Functor const, init> type;
};
} // namespace smp
} // namespace detail
} // namespace vtk
#endif // __WRAP__
#endif // DOXYGEN_SHOULD_SKIP_THIS

class VTKCOMMONCORE_EXPORT vtkSMPTools
{
public:

  // Description:
  // Execute a for operation in parallel. First and last
  // define the range over which to operate (which is defined
  // by the operator). The operation executed is defined by
  // operator() of the functor object. The grain gives the parallel
  // engine a hint about the coarseness over which to parallelize
  // the function (as defined by last-first of each execution of
  // operator() ).
  template <typename Functor>
  static void For(vtkIdType first, vtkIdType last, vtkIdType grain, Functor& f)
  {
    typename vtk::detail::smp::vtkSMPTools_Lookup_For<Functor>::type fi(f);
    fi.For(first, last, grain);
  }

  // Description:
  // Execute a for operation in parallel. First and last
  // define the range over which to operate (which is defined
  // by the operator). The operation executed is defined by
  // operator() of the functor object. The grain gives the parallel
  // engine a hint about the coarseness over which to parallelize
  // the function (as defined by last-first of each execution of
  // operator() ).
  template <typename Functor>
  static void For(vtkIdType first, vtkIdType last, vtkIdType grain, Functor const& f)
  {
    typename vtk::detail::smp::vtkSMPTools_Lookup_For<Functor const>::type fi(f);
    fi.For(first, last, grain);
  }

  // Description:
  // Execute a for operation in parallel. First and last
  // define the range over which to operate (which is defined
  // by the operator). The operation executed is defined by
  // operator() of the functor object. The grain gives the parallel
  // engine a hint about the coarseness over which to parallelize
  // the function (as defined by last-first of each execution of
  // operator() ). Uses a default value for the grain.
  template <typename Functor>
  static void For(vtkIdType first, vtkIdType last, Functor& f)
  {
    vtkSMPTools::For(first, last, 0, f);
  }

  // Description:
  // Execute a for operation in parallel. First and last
  // define the range over which to operate (which is defined
  // by the operator). The operation executed is defined by
  // operator() of the functor object. The grain gives the parallel
  // engine a hint about the coarseness over which to parallelize
  // the function (as defined by last-first of each execution of
  // operator() ). Uses a default value for the grain.
  template <typename Functor>
  static void For(vtkIdType first, vtkIdType last, Functor const& f)
  {
    vtkSMPTools::For(first, last, 0, f);
  }

  // Description:
  // Initialize the underlying libraries for execution. This is
  // not required as it is automatically called before the first
  // execution of any parallel code. However, it can be used to
  // control the maximum number of threads used when the back-end
  // supports it (currently Simple and TBB only). Make sure to call
  // it before any other parallel operation.
  // When using Kaapi, use the KAAPI_CPUCOUNT env. variable to control
  // the number of threads used in the thread pool.
  static void Initialize(int numThreads=0);
};

#endif
// VTK-HeaderTest-Exclude: vtkSMPTools.h
