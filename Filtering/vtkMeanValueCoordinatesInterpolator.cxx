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

#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"

// test git commit.

vtkCxxRevisionMacro(vtkMeanValueCoordinatesInterpolator, "$Revision: 1.83 $");
vtkStandardNewMacro(vtkMeanValueCoordinatesInterpolator);

// Special class that can iterate over different type of triangle representations
class vtkMVCTriIterator
{
public:
  vtkIdType Offset;
  vtkIdType *Tris;
  vtkIdType *Current;
  vtkIdType NumberOfTriangles;
  vtkIdType Id;

  vtkMVCTriIterator(vtkIdType numIds,vtkIdType offset,vtkIdType *t)
    {
      this->Offset = offset;
      this->Tris = t;
      this->Current = t+(this->Offset-3); //leave room for three indices
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
  if (!npts)
    {
    return;
    }

  // Begin by initializing weights. 
  for (vtkIdType pid=0; pid < npts; ++pid)
    {
    weights[pid] = static_cast<double>(0.0);
    }

  // create local array for storing point-to-vertex vectors and distances
  double *dist = new double [npts];
  double *uVec = new double [3*npts];
  static const double eps = 0.000001;
  for (vtkIdType pid = 0; pid < npts; ++pid)
    {
    // point-to-vertex vector
    uVec[3*pid]   = pts[3*pid  ] - x[0];
    uVec[3*pid+1] = pts[3*pid+1] - x[1];
    uVec[3*pid+2] = pts[3*pid+2] - x[2];
    
    // distance
    dist[pid] = vtkMath::Norm(uVec+3*pid);
    
    // handle special case when the point is really close to a vertex
    if (dist[pid] < eps)
      {
      weights[pid] = 1.0;
      delete [] dist;
      delete [] uVec;
      return;
      }
    
    // project onto unit sphere
    uVec[3*pid]   /= dist[pid];
    uVec[3*pid+1] /= dist[pid];
    uVec[3*pid+2] /= dist[pid];
    }
  
  // Now loop over all triangle to compute weights
  vtkIdType *tri = iter.Current;
  while ( iter.Id < iter.NumberOfTriangles)
    {
#if 1
    // vertex id
    vtkIdType pid0 = iter.Current[0];
    vtkIdType pid1 = iter.Current[1];
    vtkIdType pid2 = iter.Current[2];

    // unit vector
    double *u0 = uVec + 3*pid0;
    double *u1 = uVec + 3*pid1;
    double *u2 = uVec + 3*pid2;

    // edge length
    double l0 = sqrt(vtkMath::Distance2BetweenPoints(u1, u2));
    double l1 = sqrt(vtkMath::Distance2BetweenPoints(u2, u0));
    double l2 = sqrt(vtkMath::Distance2BetweenPoints(u0, u1));

    // angle
    double theta0 = 2.0*asin(l0/2.0);
    double theta1 = 2.0*asin(l1/2.0);
    double theta2 = 2.0*asin(l2/2.0);
    double halfSum = (theta0 + theta1 + theta2)/2.0;

    //std::cout << "Start ===== " << std::endl;

    // special case when the point lies on the triangle
    if (vtkMath::Pi() - halfSum < eps)
      {
      weights[pid0] = sin(theta0)* dist[pid1]* dist[pid2];
      weights[pid1] = sin(theta1)* dist[pid2]* dist[pid0];
      weights[pid2] = sin(theta2)* dist[pid0]* dist[pid1];

      double sumWeight = weights[pid0] + weights[pid1] + weights[pid2];

      weights[pid0] /= sumWeight;
      weights[pid1] /= sumWeight;
      weights[pid2] /= sumWeight;

      delete [] dist;
      delete [] uVec;
      return;
      }

    // coefficient
    double sinHalfSum = sin(halfSum);
    double sinHalfSumSubTheta0 = sin(halfSum-theta0) + eps;
    double sinHalfSumSubTheta1 = sin(halfSum-theta1) + eps;
    double sinHalfSumSubTheta2 = sin(halfSum-theta2) + eps;
    double sinTheta0 = sin(theta0) + eps;
    double sinTheta1 = sin(theta1) + eps;
    double sinTheta2 = sin(theta2) + eps;

    double c0 = 2 * sinHalfSum * sinHalfSumSubTheta0 / sinTheta1 / sinTheta2 - 1;
    double c1 = 2 * sinHalfSum * sinHalfSumSubTheta1 / sinTheta2 / sinTheta0 - 1;
    double c2 = 2 * sinHalfSum * sinHalfSumSubTheta2 / sinTheta0 / sinTheta1 - 1;
    
    if (fabs(c0) > 1)
      {
      c0 = c0 > 0 ? 1 : -1;
      }
    if (fabs(c1) > 1)
      {
      c1 = c1 > 0 ? 1 : -1;
      }
    if (fabs(c2) > 1)
      {
      c2 = c2 > 0 ? 1 : -1;
      }

    // sign
    double det = vtkMath::Determinant3x3(u0, u1, u2);

    if (fabs(det) < eps)
      {
      tri = ++iter;
      continue;
      }

    double detSign = det > 0 ? 1 : -1;
    double sign0 = detSign * sqrt(1 - c0*c0);
    double sign1 = detSign * sqrt(1 - c1*c1);
    double sign2 = detSign * sqrt(1 - c2*c2);
    
    // if x lies on the plane of current triangle but outside it, ignore
    // the current triangle.
    if (fabs(sign0) < eps || fabs(sign1) < eps || fabs(sign2) < eps)
      {
      tri = ++iter;
      continue;
      }

    // weight
    weights[pid0] += (theta0-c1*theta2-c2*theta1) / (dist[pid0]*sinTheta1*sign2);
    weights[pid1] += (theta1-c2*theta0-c0*theta2) / (dist[pid1]*sinTheta2*sign0);
    weights[pid2] += (theta2-c0*theta1-c1*theta0) / (dist[pid2]*sinTheta0*sign1);

#endif

    // 
    //increment id and next triangle
    tri = ++iter; 
    }

  // clear memory
  delete [] dist;
  delete [] uVec;

  // normalize weight
  double sumWeight = 0.0;
  for (vtkIdType pid=0; pid < npts; ++pid)
    {
    sumWeight += weights[pid];
    }
  
  if (fabs(sumWeight) < eps)
    {
    return;
    }
  
  for (vtkIdType pid=0; pid < npts; ++pid)
    {
    weights[pid] /= sumWeight;
    }
  
  return;
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
