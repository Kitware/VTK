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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkInterpolatedVelocityField, "1.13");
vtkStandardNewMacro(vtkInterpolatedVelocityField);

vtkInterpolatedVelocityField::vtkInterpolatedVelocityField()
{
  this->NumFuncs = 3; // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->DataSet = 0;
  this->Weights = 0;
  this->GenCell = vtkGenericCell::New();
  this->LastCellId = -1;
  this->CacheHit = 0;
  this->CacheMiss = 0;
  this->Caching = 1; // Caching on by default

  this->Cell = vtkGenericCell::New();
}

vtkInterpolatedVelocityField::~vtkInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->SetDataSet(0);
  this->GenCell->Delete();
  delete[] this->Weights;
  this->Weights = 0;

  this->Cell->Delete();
}

void vtkInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if ( this->DataSet )
    {
    os << indent << "Data Set: " << this->DataSet << endl;
    }
  else
    {
    os << indent << "Data Set: (none)" << endl;
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

}

// Evaluate u,v,w at x,y,z,t
int vtkInterpolatedVelocityField::FunctionValues(float* x, float* f)
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
  if ( !this->DataSet || 
       !(vectors = this->DataSet->GetPointData()->GetVectors()) )
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

	this->DataSet->GetCell(this->LastCellId, this->Cell);
	
	this->LastCellId = 
	  this->DataSet->FindCell(x, this->Cell, this->GenCell, -1, 0, 
				  subId, this->LastPCoords, this->Weights);
	if (this->LastCellId != - 1)
	  {
	  this->DataSet->GetCell(this->LastCellId, this->GenCell);
	  }
	else
	  {
	  return 0;
	  }
	}
      else
	{
	this->LastCellId = 
	  this->DataSet->FindCell(x, 0, this->GenCell, -1, 0, 
				  subId, this->LastPCoords, this->Weights);
	if (this->LastCellId != - 1)
	  {
	  this->DataSet->GetCell(this->LastCellId, this->GenCell);
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
      this->DataSet->FindCell(x, 0, this->GenCell, -1, 0, 
                              subId, this->LastPCoords, this->Weights);
    this->DataSet->GetCell(this->LastCellId, this->GenCell);
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

void vtkInterpolatedVelocityField::SetDataSet(vtkDataSet* dataset)
{
  if (this->DataSet != dataset)
    {
    if (this->DataSet != 0) { this->DataSet->UnRegister(this); }
    this->DataSet = dataset;
    if (this->DataSet != 0) { this->DataSet->Register(this); }
    this->Modified();
    delete[] this->Weights;
    this->Weights = 0; 
    // Allocate space for the largest cell
    if (this->DataSet != 0) 
      {
      this->Weights = new float[this->DataSet->GetMaxCellSize()];
      }
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


  














