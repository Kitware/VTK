/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatedVelocityField.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInterpolatedVelocityField.h"

#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif
#include <vector>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

vtkCxxRevisionMacro(vtkInterpolatedVelocityField, "1.24.2.1");
vtkStandardNewMacro(vtkInterpolatedVelocityField);

typedef vtkstd::vector< vtkDataSet* > DataSetsTypeBase;
class vtkInterpolatedVelocityFieldDataSetsType: public DataSetsTypeBase {};

vtkInterpolatedVelocityField::vtkInterpolatedVelocityField()
{
  this->NumFuncs = 3; // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->Weights = 0;
  this->WeightsSize = 0;
  this->GenCell = vtkGenericCell::New();
  this->LastCellId = -1;
  this->CacheHit = 0;
  this->CacheMiss = 0;
  this->Caching = 1; // Caching on by default

  this->Cell = vtkGenericCell::New();
  this->VectorsSelection = 0;

  this->DataSets = new vtkInterpolatedVelocityFieldDataSetsType;
  this->LastDataSet = 0;
}

vtkInterpolatedVelocityField::~vtkInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->GenCell->Delete();
  delete[] this->Weights;
  this->Weights = 0;

  this->Cell->Delete();
  this->SetVectorsSelection(0);

  delete this->DataSets;
}

static int tmp_count=0;
// Evaluate u,v,w at x,y,z,t
int vtkInterpolatedVelocityField::FunctionValues(float* x, float* f)
{
  vtkDataSet* ds=0;
  if(!this->LastDataSet && !this->DataSets->empty())
    {
    ds = (*this->DataSets)[0];
    this->LastDataSet = ds;
    }
  else
    {
    ds = this->LastDataSet;
    }
  int retVal = this->FunctionValues(ds, x, f);
  if (!retVal)
    {
    tmp_count = 0;
    for(DataSetsTypeBase::iterator i = this->DataSets->begin();
        i != this->DataSets->end(); ++i)
      {
      ds = *i;
      if(ds && ds != this->LastDataSet)
        {
        this->ClearLastCellId();
        retVal = this->FunctionValues(ds, x, f);
        if (retVal) 
          {
          this->LastDataSet = ds;
          return retVal;
          }
        }
      }
    this->ClearLastCellId();
    return 0;
    }
  tmp_count++;
  return retVal;
}

// Evaluate u,v,w at x,y,z,t
int vtkInterpolatedVelocityField::FunctionValues(vtkDataSet* dataset,
                                                 float* x, 
                                                 float* f)
{
  int i, j, subId , numPts, id;
  vtkDataArray* vectors;
  float vec[3];
  float dist2;
  int ret;

  for(i=0; i<3; i++)
    {
    f[i] = 0;
    }

  // See if a dataset has been specified and if there are input vectors
  if (!dataset || 
      !(vectors = dataset->GetPointData()->GetVectors(this->VectorsSelection)))
    {
    vtkErrorMacro(<<"Can't evaluate dataset!");
    return 0;
    }

  if (this->Caching)
    {
    // See if the point is in the cached cell
    if (this->LastCellId == -1 || 
        !(ret=this->GenCell->EvaluatePosition(x, 0, subId,
                                              this->LastPCoords, dist2, 
                                              this->Weights))
        || ret == -1)
      {

      // if not, find and get it
      if (this->LastCellId != - 1 )
        {
        this->CacheMiss++;

        dataset->GetCell(this->LastCellId, this->Cell);
        
        this->LastCellId = 
          dataset->FindCell(x, this->Cell, this->GenCell, -1, 0, 
                            subId, this->LastPCoords, this->Weights);
        if (this->LastCellId != - 1)
          {
          dataset->GetCell(this->LastCellId, this->GenCell);
          }
        else
          {
          return 0;
          }
        }
      else
        {
        this->LastCellId = 
          dataset->FindCell(x, 0, this->GenCell, -1, 0, 
                            subId, this->LastPCoords, this->Weights);
        if (this->LastCellId != - 1)
          {
          dataset->GetCell(this->LastCellId, this->GenCell);
          }
        else
          {
          return 0;
          }
        }
      }
    else
      {
      this->CacheHit++;
      }
    }
  else
    {
    // if caching is off, find the cell and get it
    this->LastCellId = 
      dataset->FindCell(x, 0, this->GenCell, -1, 0, 
                              subId, this->LastPCoords, this->Weights);
    if (this->LastCellId != - 1)
      {
      dataset->GetCell(this->LastCellId, this->GenCell);
      }
    else
      {
      return 0;
      }
    }

  // if the cell is valid
  if (this->LastCellId >= 0)
    {
    numPts = this->GenCell->GetNumberOfPoints();
    // interpolate the vectors
    for (j=0; j < numPts; j++)
      {
      id = this->GenCell->PointIds->GetId(j);
      vectors->GetTuple(id, vec);
      for (i=0; i < 3; i++)
        {
        f[i] +=  vec[i] * this->Weights[j];
        }
      }
    }
  // if not, return false
  else
    {
    return 0;
    }

  return 1;
}

void vtkInterpolatedVelocityField::AddDataSet(vtkDataSet* dataset)
{
  if (!dataset)
    {
    return;
    }

  this->DataSets->push_back(dataset);

  int size = dataset->GetMaxCellSize();
  if (size > this->WeightsSize)
    {
    this->WeightsSize = size;
    delete[] this->Weights;
    this->Weights = new float[size]; 
    }
}

int vtkInterpolatedVelocityField::GetLastWeights(float* w)
{
  int j, numPts;

  // If last cell is valid, fill w with the interpolation weights
  // and return true
  if (this->LastCellId >= 0)
    {
    numPts = this->GenCell->GetNumberOfPoints();
    for (j=0; j < numPts; j++)
      {
      w[j] = this->Weights[j];
      }
    return 1;
    }
  // otherwise, return false
  else
    {
    return 0;
    }
}

int vtkInterpolatedVelocityField::GetLastLocalCoordinates(float pcoords[3])
{
  int j;

  // If last cell is valid, fill p with the local coordinates
  // and return true
  if (this->LastCellId >= 0)
    {
    for (j=0; j < 3; j++)
      {
      pcoords[j] = this->LastPCoords[j];
      }
    return 1;
    }
  // otherwise, return false
  else
    {
    return 0;
    }
}

void vtkInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if ( this->VectorsSelection )
    {
    os << indent << "VectorsSelection: " << this->VectorsSelection << endl;
    }
  else
    {
    os << indent << "VectorsSelection: (none)" << endl;
    }
  if ( this->GenCell )
    {
    os << indent << "Last cell: " << this->GenCell << endl;
    }
  else
    {
    os << indent << "Last cell: (none)" << endl;
    }
  os << indent << "Weights: " << this->Weights << endl;
  os << indent << "Last cell Id: " << this->LastCellId << endl;
  os << indent << "Cache hit: " << this->CacheHit << endl;
  os << indent << "Cache miss: " << this->CacheMiss << endl;
  os << indent << "Caching: ";
  if ( this->Caching )
    {
    os << "on." << endl;
    }
  else
    {
    os << "off." << endl;
    }

  os << indent << "VectorsSelection: " 
     << (this->VectorsSelection?this->VectorsSelection:"(none)") << endl;
  os << indent << "LastDataSet : "
     << this->LastDataSet << endl;

}

