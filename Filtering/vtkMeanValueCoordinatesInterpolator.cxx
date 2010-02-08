/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMeanValueCoordinatesInterpolator.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkObjectFactory.h"

#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"


vtkCxxRevisionMacro(vtkMeanValueCoordinatesInterpolator, "$Revision: 1.83 $");
vtkStandardNewMacro(vtkMeanValueCoordinatesInterpolator);

// Special class that can iterate over different type of triangle representations
class vtkMVCTriIterator
{
public:
  vtkIdType *Tris;
  vtkIdType *Current;
  vtkIdType Offset;
  vtkIdType NumberOfTriangles;
  vtkIdType Id;

  vtkMVCTriIterator(vtkIdType numIds,vtkIdType offset,vtkIdType *t)
    {
      this->Tris = t;
      this->Current = t;
      this->Offset = offset;
      this->NumberOfTriangles = numIds / offset;
      this->Id = 0;
    }
  vtkIdType* operator++()
    {
      this->Current += this->Offset;
      this->Id++;
      return this->Current;
    }
};
  

//----------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkMeanValueCoordinatesInterpolator::
vtkMeanValueCoordinatesInterpolator()
{
}

//----------------------------------------------------------------------------
vtkMeanValueCoordinatesInterpolator::
~vtkMeanValueCoordinatesInterpolator()
{
}

//----------------------------------------------------------------------------
// Templated function to generate weights. This class actually implements the
// algorithm. (Note: the input point type should be float or double, but this is
// not enfored.)
template <class T>
void vtkComputeMVCWeights(double x[3], T *pts, vtkIdType npts, 
                          vtkIdType *tris, vtkMVCTriIterator& iter,
                          double *weights)
{
  //Points are organized {(x,y,z), (x,y,z), ....}
  //Tris are organized {(i,j,k), (i,j,k), ....}
  //Weights per point are computed
  
  // Begin by initializing weights. Too bad we can't use memset.
  for (vtkIdType pid=0; pid < npts; ++pid)
    {
    weights[pid] = static_cast<double>(0.0);
    }
         
  // Now loop over all triangle to compute weights
  vtkIdType *tri;
  while ( iter.Id < iter.NumberOfTriangles)
    {
    //algorithm follows
    

    //increment id and next triangle
    tri = ++iter; 
    }
}

//----------------------------------------------------------------------------
// Static function to compute weights (with vtkIdList)
// Satisfy classes' public API.
void vtkMeanValueCoordinatesInterpolator::
ComputeInterpolationWeights(double x[3], vtkPoints *pts, vtkIdList *tris, double *weights)
{
  // Check the input
  if ( !tris )
    {
    vtkGenericWarningMacro("Did not provide triangles");
    return;
    }
  vtkIdType *t = tris->GetPointer(0);
  // Below the vtkCellArray has three entries per triangle {(i,j,k), (i,j,k), ....}
  vtkMVCTriIterator iter(tris->GetNumberOfIds(),3,t);

  vtkMeanValueCoordinatesInterpolator::
    ComputeInterpolationWeights(x,pts,t,iter,weights);
}

//----------------------------------------------------------------------------
// Static function to compute weights (with vtkCellArray)
// Satisfy classes' public API.
void vtkMeanValueCoordinatesInterpolator::
ComputeInterpolationWeights(double x[3], vtkPoints *pts, vtkCellArray *tris, double *weights)
{
  // Check the input
  if ( !tris )
    {
    vtkGenericWarningMacro("Did not provide triangles");
    return;
    }
  vtkIdType *t = tris->GetPointer();
  // Below the vtkCellArray has four entries per triangle {(3,i,j,k), (3,i,j,k), ....}
  vtkMVCTriIterator iter(tris->GetNumberOfConnectivityEntries(),4,t);

  vtkMeanValueCoordinatesInterpolator::
    ComputeInterpolationWeights(x,pts,t,iter,weights);
}

//----------------------------------------------------------------------------
// Static function to compute weights
void vtkMeanValueCoordinatesInterpolator::
ComputeInterpolationWeights(double x[3], vtkPoints *pts, vtkIdType *tris, 
                            vtkMVCTriIterator& iter, double *weights)
{
  // Check the input
  if ( !pts || !weights)
    {
    vtkGenericWarningMacro("Did not provide proper input");
    return;
    }

  // Prepare the arrays
  vtkIdType numPts = pts->GetNumberOfPoints();
  if ( numPts <= 0 )
    {
    return;
    }
  
  void *p = pts->GetVoidPointer(0);
  
  // call templated function to compute the weights. Note that we do not
  // use VTK's template macro because we are limiting usage to floats and doubles.
  switch (pts->GetDataType())
    {
    vtkTemplateMacro(
      vtkComputeMVCWeights(x, (VTK_TT *)(p), numPts, tris, iter, weights));

    default:
      break;
    }
}


//----------------------------------------------------------------------------
void vtkMeanValueCoordinatesInterpolator::
PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
