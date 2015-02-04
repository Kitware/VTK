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

#ifndef vtkSMPTools2_h__
#define vtkSMPTools2_h__

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include "vtkSMPThreadLocal.h" // For Initialized

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>

class vtkSMPTools;
class vtkParallelTree;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef __WRAP__
namespace vtk
{
namespace detail
{
namespace smp
{
template <typename Tree, typename Functor>
class TaskTraverse : public tbb::task {
    const Tree* tree;
    Functor& functor;
    const int level;
    const vtkIdType index, BranchingFactor;

  public:
    TaskTraverse(
        const Tree* t, Functor& f,
        int l, vtkIdType i, vtkIdType b)
          : tree(t), functor(f),
            level(l), index(i),
            BranchingFactor(b)
    {
    }

    ~TaskTraverse()
    {
    }

    tbb::task* execute()
    {
    functor.TestInit();
    if ( tree->TraverseNode(index, level, functor.F) )
      {
      int l = level + 1;
      this->set_ref_count(BranchingFactor + 1);
      TaskTraverse* t;
      for ( vtkIdType i = index * BranchingFactor + 1, j = 0;
            j < BranchingFactor; ++i, ++j )
        {
        t = new(this->allocate_child())
          TaskTraverse(tree, functor, l, i, BranchingFactor);
        tbb::task::spawn(*t);
        }
      this->wait_for_all();
      }
    return 0;
    }

};

template <typename T>
class vtkSMPTools2_Has_Initialize
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
class vtkSMPTools2_Has_Initialize_const
{
  typedef char (&no_type)[1];
  typedef char (&yes_type)[2];
  template <typename U, void (U::*)() const> struct V {};
  template <typename U> static yes_type check(V<U, &U::Initialize>*);
  template <typename U> static no_type check(...);
public:
  static bool const value = sizeof(check<T>(0)) == sizeof(yes_type);
};

template <typename Tree, typename FunctorInternal>
static void vtkSMPTools2_Impl_Traverse(int level, vtkIdType bf, const Tree* t, FunctorInternal& fi)
{
  TaskTraverse<Tree,FunctorInternal>* task = new(tbb::task::allocate_root())
    TaskTraverse<Tree,FunctorInternal>(t, fi, level, 0, bf);
  tbb::task::spawn_root_and_wait(*task);
}

template <typename T, typename Functor>
class vtkSMPTools2_Has_Traverse_Node_const
{
  typedef char (&no_type)[1];
  typedef char (&yes_type)[2];
  template <typename U, int (U::*)(vtkIdType, int, Functor&) const> struct V {};
  template <typename U> static yes_type check(V<U, &U::TraverseNode>*);
  template <typename U> static no_type check(...);
public:
  static bool const value = sizeof(check<T>(0)) == sizeof(yes_type);
};

template <typename Tree, bool Init>
struct vtkSMPTools2_FunctorTraverse;

template <typename Tree>
struct vtkSMPTools2_FunctorTraverse<Tree, true>
{
  public:
    typedef const Tree* actual_type;
};

template <typename T, typename Functor>
class vtkSMPTools2_Lookup_Traverse
{
  static bool const init = vtkSMPTools2_Has_Traverse_Node_const<T, Functor>::value;
public:
  typedef vtkSMPTools2_FunctorTraverse<T, init> type;
};

template <typename Functor, bool Init>
struct vtkSMPTools2_FunctorInternal;

template <typename Functor>
struct vtkSMPTools2_FunctorInternal<Functor, false>
{
  Functor& F;
  vtkSMPTools2_FunctorInternal(Functor& f): F(f) {}
  void Execute(vtkIdType first, vtkIdType last)
  {
    this->F(first, last);
  }
  void TestInit()
  {
  }
  template <typename Tree>
  void Traverse(int level, vtkIdType bf, typename vtkSMPTools2_Lookup_Traverse<Tree const, Functor>::type::actual_type t)
  {
    vtk::detail::smp::vtkSMPTools2_Impl_Traverse(level, bf, t, *this);
  }
  vtkSMPTools2_FunctorInternal<Functor, false>& operator=(
    const vtkSMPTools2_FunctorInternal<Functor, false>&);
  vtkSMPTools2_FunctorInternal<Functor, false>(
    const vtkSMPTools2_FunctorInternal<Functor, false>&);
};

template <typename Functor>
struct vtkSMPTools2_FunctorInternal<Functor, true>
{
  Functor& F;
  vtkSMPThreadLocal<unsigned char> Initialized;
  vtkSMPTools2_FunctorInternal(Functor& f): F(f), Initialized(0) {}
  void Execute(vtkIdType first, vtkIdType last)
  {
    this->TestInit();
    this->F(first, last);
  }
  void TestInit()
  {
    unsigned char& inited = this->Initialized.Local();
    if (!inited)
      {
      this->F.Initialize();
      inited = 1;
      }
  }
  template <typename Tree>
  void Traverse(int level, vtkIdType bf, typename vtkSMPTools2_Lookup_Traverse<Tree const, Functor>::type::actual_type t)
  {
    vtk::detail::smp::vtkSMPTools2_Impl_Traverse(level, bf, t, *this);
    this->F.Reduce();
  }
  vtkSMPTools2_FunctorInternal<Functor, true>& operator=(
    const vtkSMPTools2_FunctorInternal<Functor, true>&);
  vtkSMPTools2_FunctorInternal<Functor, true>(
    const vtkSMPTools2_FunctorInternal<Functor, true>&);
};

template <typename Functor>
class vtkSMPTools2_Lookup_For
{
  static bool const init = vtkSMPTools2_Has_Initialize<Functor>::value;
public:
  typedef vtkSMPTools2_FunctorInternal<Functor, init> type;
};

template <typename Functor>
class vtkSMPTools2_Lookup_For<Functor const>
{
  static bool const init = vtkSMPTools2_Has_Initialize_const<Functor>::value;
public:
  typedef vtkSMPTools2_FunctorInternal<Functor const, init> type;
};
} // namespace smp
} // namespace detail
} // namespace vtk
#endif // __WRAP__
#endif // DOXYGEN_SHOULD_SKIP_THIS

class VTKCOMMONCORE_EXPORT vtkSMPTools2
{
public:

  // Description:
  // Traverse a tree in parallel. The tree has to be parallel aware.
  template <typename Tree, typename Functor>
  static void Traverse(int level, vtkIdType bf, const Tree* t, Functor& f)
  {
    typename vtk::detail::smp::vtkSMPTools2_Lookup_For<Functor>::type fi(f);
    fi.template Traverse<Tree const>(level, bf, t);
  }

  template <typename Tree, typename Functor>
  static void Traverse(int level, vtkIdType bf, const Tree* t, Functor const& f)
  {
    typename vtk::detail::smp::vtkSMPTools2_Lookup_For<Functor const>::type fi(f);
    fi.template Traverse<Tree>(level, bf, t);
  }
};

#endif
// VTK-HeaderTest-Exclude: vtkSMPTools2.h
