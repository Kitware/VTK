/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEllipsoidalGaussianKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEllipsoidalGaussianKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkEllipsoidalGaussianKernel);

//----------------------------------------------------------------------------
vtkEllipsoidalGaussianKernel::vtkEllipsoidalGaussianKernel()
{
  this->Radius = 1.0;
  this->Sharpness = 2.0;
  this->Eccentricity = 2.0;

  this->Normals = NULL;
  this->Scalars = NULL;

  this->F2 = this->Sharpness / this->Radius;
  this->E2 = this->Eccentricity * this->Eccentricity;
}


//----------------------------------------------------------------------------
vtkEllipsoidalGaussianKernel::~vtkEllipsoidalGaussianKernel()
{
  this->FreeStructures();
}


//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::
FreeStructures()
{
  this->Superclass::FreeStructures();

  if ( this->Normals )
    {
    this->Normals->Delete();
    this->Normals = NULL;
    }

  if ( this->Scalars )
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *pd)
{
  this->Superclass::Initialize(loc, ds, pd);

  this->Scalars = pd->GetScalars();
  if ( this->Scalars && this->Scalars->GetNumberOfComponents() == 1 )
    {
    this->Scalars->Register(this);
    }
  else
    {
    this->Scalars = NULL;
    }

  this->Normals = pd->GetNormals();
  if ( this->Normals )
    {
    this->Normals->Register(this);
    }
  else
    {
    this->Normals = NULL;
    }

  this->F2 = this->Sharpness / this->Radius;
  this->F2 = this->F2 * this->F2;
  this->E2 = this->Eccentricity * this->Eccentricity;
}

//----------------------------------------------------------------------------
vtkIdType vtkEllipsoidalGaussianKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *weights)
{
  this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
  vtkIdType numPts = pIds->GetNumberOfIds();

  if ( numPts >= 1 ) //Use Gaussian kernel
    {
    int i;
    vtkIdType id;
    double sum = 0.0;
    weights->SetNumberOfTuples(numPts);
    double *w = weights->GetPointer(0);
    double y[3], v[3], r2, z2, rxy2, mag;
    double n[3], s;
    double f2=this->F2, e2=this->E2;

    for (i=0; i<numPts; ++i)
      {
      id = pIds->GetId(i);
      this->DataSet->GetPoint(id,y);

      v[0] = x[0] - y[0];
      v[1] = x[1] - y[1];
      v[2] = x[2] - y[2];
      r2 = vtkMath::Dot(v,v);

      if ( r2 == 0.0 ) //precise hit on existing point
        {
        pIds->SetNumberOfIds(1);
        pIds->SetId(0,id);
        weights->SetNumberOfTuples(1);
        weights->SetValue(0,1.0);
        return 1;
        }
      else // continue computing weights
        {
        // Normal affect
        if ( this->Normals )
          {
          this->Normals->GetTuple(id,n);
          mag = vtkMath::Dot(n,n);
          mag = ( mag == 0.0 ? 1.0 : sqrt(mag) );
          }
        else
          {
          mag = 1.0;
          }

        // Scalar scaling
        if ( this->Scalars )
          {
          this->Scalars->GetTuple(id,&s);
          }
        else
          {
          s = 1.0;
          }

        z2 = vtkMath::Dot(v,n) / mag;
        z2 = z2*z2;
        rxy2 = r2 - z2;

        w[i] = s * exp(-f2 * (rxy2/e2 + z2));
        sum += w[i];
        }//computing weights
      }//over all points

    // Normalize
    for (i=0; i<numPts; ++i)
      {
      w[i] /= sum;
      }

    return numPts;
    }//using ellipsoidal Gaussian Kernel

  else //null point
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << endl;
  os << indent << "Sharpness: " << this->Sharpness << endl;
  os << indent << "Eccentricity: " << this->Eccentricity << endl;
}
