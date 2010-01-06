/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlane.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPlane, "1.44");
vtkStandardNewMacro(vtkPlane);

// Construct plane passing through origin and normal to z-axis.
vtkPlane::vtkPlane()
{
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

void vtkPlane::ProjectPoint(double x[3], double origin[3], 
                            double normal[3], double xproj[3])
{
  double t, xo[3];

  xo[0] = x[0] - origin[0];
  xo[1] = x[1] - origin[1];
  xo[2] = x[2] - origin[2];

  t = vtkMath::Dot(normal,xo);

  xproj[0] = x[0] - t * normal[0];
  xproj[1] = x[1] - t * normal[1];
  xproj[2] = x[2] - t * normal[2];
}

void vtkPlane::Push(double distance)
{
  int i;

  if ( distance == 0.0 )
    {
    return;
    }
  for (i=0; i < 3; i++ )
    {
    this->Origin[i] += distance * this->Normal[i];
    }
  this->Modified();
}

// Project a point x onto plane defined by origin and normal. The 
// projected point is returned in xproj. NOTE : normal NOT required to
// have magnitude 1.
void vtkPlane::GeneralizedProjectPoint(double x[3], double origin[3],
                                       double normal[3], double xproj[3])
{
  double t, xo[3], n2;

  xo[0] = x[0] - origin[0];
  xo[1] = x[1] - origin[1];
  xo[2] = x[2] - origin[2];

  t = vtkMath::Dot(normal,xo);
  n2 = vtkMath::Dot(normal, normal);

  if (n2 != 0)
    {
    xproj[0] = x[0] - t * normal[0]/n2;
    xproj[1] = x[1] - t * normal[1]/n2;
    xproj[2] = x[2] - t * normal[2]/n2;
    }
  else
    {
    xproj[0] = x[0];
    xproj[1] = x[1];
    xproj[2] = x[2];
    }
}


// Evaluate plane equation for point x[3].
double vtkPlane::EvaluateFunction(double x[3])
{
  return ( this->Normal[0]*(x[0]-this->Origin[0]) + 
           this->Normal[1]*(x[1]-this->Origin[1]) + 
           this->Normal[2]*(x[2]-this->Origin[2]) );
}

// Evaluate function gradient at point x[3].
void vtkPlane::EvaluateGradient(double vtkNotUsed(x)[3], double n[3])
{
  for (int i=0; i<3; i++)
    {
    n[i] = this->Normal[i];
    }
}
 
#define VTK_PLANE_TOL 1.0e-06

// Given a line defined by the two points p1,p2; and a plane defined by the
// normal n and point p0, compute an intersection. The parametric
// coordinate along the line is returned in t, and the coordinates of 
// intersection are returned in x. A zero is returned if the plane and line
// do not intersect between (0<=t<=1). If the plane and line are parallel,
// zero is returned and t is set to VTK_LARGE_DOUBLE.
int vtkPlane::IntersectWithLine(double p1[3], double p2[3], double n[3], 
                               double p0[3], double& t, double x[3])
{
  double num, den, p21[3];
  double fabsden, fabstolerance;

  // Compute line vector
  // 
  p21[0] = p2[0] - p1[0];
  p21[1] = p2[1] - p1[1];
  p21[2] = p2[2] - p1[2];

  // Compute denominator.  If ~0, line and plane are parallel.
  // 
  num = vtkMath::Dot(n,p0) - ( n[0]*p1[0] + n[1]*p1[1] + n[2]*p1[2] ) ;
  den = n[0]*p21[0] + n[1]*p21[1] + n[2]*p21[2];
  //
  // If denominator with respect to numerator is "zero", then the line and
  // plane are considered parallel. 
  //

  // trying to avoid an expensive call to fabs()
  if (den < 0.0)
    {
    fabsden = -den;
    }
  else
    {
    fabsden = den;
    }
  if (num < 0.0)
    {
    fabstolerance = -num*VTK_PLANE_TOL;
    }
  else
    {
    fabstolerance = num*VTK_PLANE_TOL;
    }
  if ( fabsden <= fabstolerance )
    {
    t = VTK_DOUBLE_MAX;
    return 0;
    }

  // valid intersection
  t = num / den;

  x[0] = p1[0] + t*p21[0];
  x[1] = p1[1] + t*p21[1];
  x[2] = p1[2] + t*p21[2];

  if ( t >= 0.0 && t <= 1.0 )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Normal: (" << this->Normal[0] << ", " 
    << this->Normal[1] << ", " << this->Normal[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", " 
    << this->Origin[1] << ", " << this->Origin[2] << ")\n";
}


void CenterOfMass(vtkDataSet* points, double* center)
{
  center[0] = 0.0;
  center[1] = 0.0;
  center[2] = 0.0;
    
  for(vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double point[3];
    points->GetPoint(i, point);
    
    center[0] += point[0];
    center[1] += point[1];
    center[2] += point[2];
  }
  
  double numberOfPoints = static_cast<double>(points->GetNumberOfPoints());
  center[0] = center[0]/numberOfPoints;
  center[1] = center[1]/numberOfPoints;
  center[2] = center[2]/numberOfPoints;
}

/* allocate memory for an nrow x ncol matrix */
template<class TReal>
    TReal **create_matrix ( long nrow, long ncol )
{
  typedef TReal* TRealPointer;
  TReal **m = new TRealPointer[nrow];

  TReal* block = ( TReal* ) calloc ( nrow*ncol, sizeof ( TReal ) );
  m[0] = block;
  for ( int row = 1; row < nrow; ++row )
  {
    m[ row ] = &block[ row * ncol ];
  }
  return m;
}

/* free a TReal matrix allocated with matrix() */
template<class TReal>
    void free_matrix ( TReal **m )
{
  free ( m[0] );
  delete[] m;
}

void vtkPlane::BestFitFromPoints(vtkDataSet *points)
{
  vtkIdType numPoints = points->GetNumberOfPoints();
  double dNumPoints = static_cast<double>(numPoints);
  
  //find the center of mass of the points
  double center[3];
  CenterOfMass(points, center);
  //cout << "Center of mass: " << Center[0] << " " << Center[1] << " " << Center[2] << endl;
  
  //Compute sample covariance matrix
  double **a = create_matrix<double> ( 3,3 );
  a[0][0] = 0; a[0][1] = 0;  a[0][2] = 0;
  a[1][0] = 0; a[1][1] = 0;  a[1][2] = 0;
  a[2][0] = 0; a[2][1] = 0;  a[2][2] = 0;
  
  for(unsigned int pointId = 0; pointId < numPoints; pointId++ )
  {
    double x[3];
    double xp[3];
    points->GetPoint(pointId, x);
    xp[0] = x[0] - center[0]; 
    xp[1] = x[1] - center[1]; 
    xp[2] = x[2] - center[2];
    for (unsigned int i = 0; i < 3; i++)
    {
      a[0][i] += xp[0] * xp[i];
      a[1][i] += xp[1] * xp[i];
      a[2][i] += xp[2] * xp[i];
    }
  }
  
  //divide by N-1 for an unbiased estimate
  for(unsigned int i = 0; i < 3; i++)
  {
    a[0][i] /= dNumPoints-1;
    a[1][i] /= dNumPoints-1;
    a[2][i] /= dNumPoints-1;
  }

  // Extract eigenvectors from covariance matrix
  double **eigvec = create_matrix<double> ( 3,3 );
  
  double eigval[3];
  vtkMath::Jacobi(a,eigval,eigvec);
  
  //Set the plane normal to the smallest eigen vector
  this->SetNormal(eigvec[0][2], eigvec[1][2], eigvec[2][2]);
  
  //cleanup
  free_matrix(eigvec);
  free_matrix(a);
  
  //Set the plane origin to the center of mass
  this->SetOrigin(center[0], center[1], center[2]);

}
