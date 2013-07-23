//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef vtkToDax_Allocators_h
#define vtkToDax_Allocators_h

#include <dax/VectorTraits.h>
#include <cstddef>

class vtkPoints;
class vtkCellArray;

/*
 * The one rule of allocators is that they can allocate
 * memory, but they can't set any values in the allocated memory.
 * We can't write to the memory because that causes affinity
 * between the memory location and the current thread, which is a very
 * bad thing as we want that memory location affinity to be assigned to the dax
 * thread that will be using section, not the master thread
 */

namespace vtkToDax
{
template< typename _T,
          int NUM_COMPONENTS>
struct vtkAlloc
{
  typedef _T T;
  typedef vtkAlloc<T,NUM_COMPONENTS> self;

  typedef std::size_t   size_type;
  typedef ptrdiff_t     difference_type;
  typedef T*            pointer;
  typedef const T*      const_pointer;
  typedef T&            reference;
  typedef const T&      const_reference;
  typedef T             value_type;


  pointer allocate(size_type n, self::const_pointer hint = 0)
    {
    pointer p =  value_type::New();
    p->SetNumberOfComponents(NUM_COMPONENTS);
    p->SetNumberOfTuples(n);
    return p;
    }

  void deallocate(self::pointer p, self::size_type)
    {
    p->Delete();
    }
};

template<int NUM_COMPONENTS>
struct vtkAlloc<vtkPoints, NUM_COMPONENTS>
{
  typedef vtkPoints T;
  typedef vtkAlloc<T,NUM_COMPONENTS> self;

  typedef std::size_t   size_type;
  typedef ptrdiff_t     difference_type;
  typedef T*            pointer;
  typedef const T*      const_pointer;
  typedef T&            reference;
  typedef const T&      const_reference;
  typedef T             value_type;


  pointer allocate(size_type n, const_pointer hint = 0)
    {
#ifdef DAX_USE_DOUBLE_PRECISION
    pointer p = value_type::New(VTK_DOUBLE);
#else
    pointer p = value_type::New(VTK_FLOAT);
#endif
    p->SetNumberOfPoints(n);
    return p;
    }

  void deallocate(pointer p, size_type)
    {
    p->Delete();
    }
};


template<int NUM_COMPONENTS>
struct vtkAlloc<vtkCellArray, NUM_COMPONENTS>
{
  typedef vtkCellArray T;
  typedef vtkAlloc<T,NUM_COMPONENTS> self;

  typedef std::size_t   size_type;
  typedef ptrdiff_t     difference_type;
  typedef T*            pointer;
  typedef const T*      const_pointer;
  typedef T&            reference;
  typedef const T&      const_reference;
  typedef T             value_type;


  //for cell arrays dax requests an allocation that is num_cells * num_components
  pointer allocate(size_type n, const_pointer hint = 0)
    {
    pointer p =  value_type::New();
    const size_type numCells = n/NUM_COMPONENTS;
    p->SetNumberOfCells(numCells);
    p->GetData()->SetNumberOfTuples(n+numCells);
    return p;
    }

  void deallocate(pointer p, size_type)
    {
    p->Delete();
    }
};
}

#endif //vtkToDax_Allocators_h
