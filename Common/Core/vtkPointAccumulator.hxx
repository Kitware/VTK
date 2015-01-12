/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointAccumulator.hxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointAccumulator - Container class that manages appending data arrays of points.
// .SECTION Description
//
// The template types are T_CPP for the c++ data type and T_VTK for
// the VTK data type. Eg: if T_CCP==double the T_VTK must be
// vtkDoubleArray. The main difference between the way this
// works and if you were to do the same thing with a data array is
// that here the memory grows by exactly what is needed, and
// in VTK data arrays the memory will grow by at least twice
// what is requested.

#ifndef vtkPointAccumulator_hxx
#define vtkPointAccumulator_hxx


#include <exception>
#include "vtkPoints.h"

template<typename T_CPP, class T_VTK>
class vtkPointAccumulator
{
  public:
    vtkPointAccumulator()
    {
      this->PtStore=0;
      this->NPts=0;
    }
    ~vtkPointAccumulator()
    {
      this->Clear();
    }
    // Description:
    // Free resources and mark as empty.
    void Clear()
    {
      if (this->PtStore!=0)
        {
        free(this->PtStore);
        }
      this->PtStore=0;
      this->NPts=0;
    }
    // Description:
    // Test if there is anything in the store.
    bool Empty()
    {
      return this->NPts==0;
    }
    // Description:
    // Extend the internal store and get a pointer to
    // the newly added memory.
    T_CPP *Expand(vtkIdType n)
    {
      const int bytesPerPoint=3*sizeof(T_CPP);
      // extend
      vtkIdType newNPts=this->NPts+n;
      T_CPP *newPointStore
        = static_cast<T_CPP *>(realloc(this->PtStore,newNPts*bytesPerPoint));
      if (newPointStore==0)
        {
        #ifndef NDEBUG
        abort();
        #else
        throw std::bad_alloc();
        #endif
        }
      // mark begin of new
      T_CPP *writePointer=newPointStore+3*this->NPts;
      // update
      this->PtStore=newPointStore;
      this->NPts=newNPts;

      return writePointer;
    }
    // Description:
    // Adds an array of points to the end of
    // the internal store.
    void Accumulate(T_CPP *pts, vtkIdType n)
    {
      // extend
      T_CPP *writePointer=this->Expand(n);
      // copy at end
      const int bytesPerPoint=3*sizeof(T_CPP);
      memcpy(writePointer,pts,n*bytesPerPoint);
    }
    // Description:
    // Adds an array of points at the end of
    // the internal store.
    void Accumulate(T_VTK *pts)
    {
      this->Accumulate(pts->GetPointer(0),pts->GetNumberOfTuples());
    }
    // Description:
    // Creates a vtkPoints data structure from
    // the internal store. Caller to delete the points.
    vtkPoints *BuildVtkPoints()
    {
      T_VTK *da=T_VTK::New();
      da->SetNumberOfComponents(3);
      da->SetArray(this->PtStore,3*this->NPts,1);
      vtkPoints *pts=vtkPoints::New();
      pts->SetData(da);
      da->Delete();

      return pts;
    }
    // Description:
    // Compute axis-aligned bounding box. An exhaustive search is made
    // through points every time. It's calllers responsibility to use
    // sparingly.
    void GetBounds(double bounds[6])
    {
      // Prepare
      for (int q=0; q<3; ++q)
        {
        bounds[q]=static_cast<double>(this->PtStore[q]);
        bounds[q+1]=static_cast<double>(this->PtStore[q+1]);
        }
      // Search
      for (vtkIdType i=1; i<this->NPts; ++i)
        {
        double pt[3];
        vtkIdType ptIdx=3*i;
        pt[0]=static_cast<double>(this->PtStore[ptIdx]);
        pt[1]=static_cast<double>(this->PtStore[ptIdx+1]);
        pt[2]=static_cast<double>(this->PtStore[ptIdx+2]);
        if (pt[0]<bounds[0]) bounds[0]=pt[0];
        if (pt[0]>bounds[1]) bounds[1]=pt[0];
        if (pt[1]<bounds[2]) bounds[2]=pt[1];
        if (pt[1]>bounds[3]) bounds[3]=pt[1];
        if (pt[2]<bounds[4]) bounds[4]=pt[2];
        if (pt[2]>bounds[5]) bounds[5]=pt[2];
        }
    }
    // Description:
    // Return the number of points currently in the point store.
    vtkIdType GetNumberOfPoints()
    {
      return this->NPts;
    }
    // Description:
    // Print the contents of the internal store.
    void Print()
    {
      T_CPP *pBuf=this->PtStore;
      for (int i=0; i<this->NPts; ++i)
        {
        cerr << i << " (" << pBuf[0];
        for (int q=1; q<3; ++q)
          {
          cerr << ", " << pBuf[q];
          }
        cerr << ")" << endl;
        pBuf+=3;
        }
    }

  private:
    vtkPointAccumulator(const vtkPointAccumulator &); // Not implemented
    vtkPointAccumulator &operator=(const vtkPointAccumulator &); // Not implemented

    T_CPP *PtStore;
    vtkIdType NPts;
};
#endif

