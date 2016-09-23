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
#include "vtkMathUtilities.h"

vtkStandardNewMacro(vtkEllipsoidalGaussianKernel);

//----------------------------------------------------------------------------
vtkEllipsoidalGaussianKernel::vtkEllipsoidalGaussianKernel()
{
  this->UseNormals = true;
  this->UseScalars = false;

  this->NormalsArrayName = "Normals";
  this->ScalarsArrayName = "Scalars";

  this->ScaleFactor = 1.0;
  this->Sharpness = 2.0;
  this->Eccentricity = 2.0;

  this->F2 = this->Sharpness / this->Radius;
  this->E2 = this->Eccentricity * this->Eccentricity;
  this->NormalsArray = NULL;
  this->ScalarsArray = NULL;
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

  if ( this->NormalsArray )
  {
    this->NormalsArray->Delete();
    this->NormalsArray = NULL;
  }

  if ( this->ScalarsArray )
  {
    this->ScalarsArray->Delete();
    this->ScalarsArray = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *pd)
{
  this->Superclass::Initialize(loc, ds, pd);

  // Grab the scalars if requested
  if ( this->UseScalars)
  {
    this->ScalarsArray = pd->GetScalars();
    if ( !this->ScalarsArray )
    {
      this->ScalarsArray = pd->GetArray(this->ScalarsArrayName);
    }
    if ( this->ScalarsArray &&
         this->ScalarsArray->GetNumberOfComponents() == 1 )
    {
      this->ScalarsArray->Register(this);
    }
  }
  else
  {
    this->ScalarsArray = NULL;
  }

  // Grab the normals if requested
  if ( this->UseNormals)
  {
    this->NormalsArray = pd->GetNormals();
    if ( !this->NormalsArray )
    {
      this->NormalsArray = pd->GetArray(this->NormalsArrayName);
    }
    if ( this->NormalsArray )
    {
      this->NormalsArray->Register(this);
    }
    else
    {
      this->NormalsArray = NULL;
    }
  }

  // Set up computation
  this->F2 = this->Sharpness / this->Radius;
  this->F2 = this->F2 * this->F2;
  this->E2 = this->Eccentricity * this->Eccentricity;
}

//----------------------------------------------------------------------------
vtkIdType vtkEllipsoidalGaussianKernel::
ComputeWeights(double x[3], vtkIdList *pIds, vtkDoubleArray *prob,
               vtkDoubleArray *weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  int i;
  vtkIdType id;
  double sum = 0.0;
  weights->SetNumberOfTuples(numPts);
  double *p = (prob ? prob->GetPointer(0) : NULL);
  double *w = weights->GetPointer(0);
  double y[3], v[3], r2, z2, rxy2, mag;
  double n[3], s, scale;
  double f2=this->F2, e2=this->E2;

  for (i=0; i<numPts; ++i)
  {
    id = pIds->GetId(i);
    this->DataSet->GetPoint(id,y);

    v[0] = x[0] - y[0];
    v[1] = x[1] - y[1];
    v[2] = x[2] - y[2];
    r2 = vtkMath::Dot(v,v);

    if ( vtkMathUtilities::FuzzyCompare(r2, 0.0, std::numeric_limits<double>::epsilon()*256.0 )) //precise hit on existing point
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
      if ( this->NormalsArray )
      {
        this->NormalsArray->GetTuple(id,n);
        mag = vtkMath::Dot(n,n);
        mag = ( mag == 0.0 ? 1.0 : sqrt(mag) );
        z2 = vtkMath::Dot(v,n) / mag;
        z2 = z2*z2;
      }
      else
      {
        z2 = 0.0;
        mag = 1.0;
      }

      // Scalar scaling
      if ( this->ScalarsArray )
      {
        this->ScalarsArray->GetTuple(id,&s);
      }
      else
      {
        s = 1.0;
      }

      rxy2 = r2 - z2;

      scale = this->ScaleFactor * (p ? p[i] : 1.0);

      w[i] = scale * s * exp(-f2 * (rxy2/e2 + z2));

      sum += w[i];
    }//computing weights
  }//over all points

  // Normalize
  if ( this->NormalizeWeights && sum != 0.0 )
  {
    for (i=0; i<numPts; ++i)
    {
      w[i] /= sum;
    }
  }

  return numPts;
}

//----------------------------------------------------------------------------
void vtkEllipsoidalGaussianKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Use Normals: "
     << (this->GetUseNormals()? "On" : " Off") << "\n";
  os << indent << "Use Scalars: "
     << (this->GetUseScalars()? "On" : " Off") << "\n";

  os << indent << "Scalars Array Name: " << this->GetScalarsArrayName() << "\n";
  os << indent << "Normals Array Name: " << this->GetNormalsArrayName() << "\n";

  os << indent << "Radius: " << this->GetRadius() << endl;
  os << indent << "ScaleFactor: " << this->GetScaleFactor() << endl;
  os << indent << "Sharpness: " << this->GetSharpness() << endl;
  os << indent << "Eccentricity: " << this->GetEccentricity() << endl;

}
