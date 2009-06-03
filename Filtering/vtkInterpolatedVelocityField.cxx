/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInterpolatedVelocityField.h"

#include "vtkMath.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <vtkstd/vector>
//---------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkInterpolatedVelocityField, "1.6");
vtkStandardNewMacro(vtkInterpolatedVelocityField); 
//---------------------------------------------------------------------------
typedef vtkstd::vector< vtkDataSet* > DataSetsTypeBase;
class vtkInterpolatedVelocityFieldDataSetsType: public DataSetsTypeBase {};
//---------------------------------------------------------------------------
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
  this->Caching = true; // Caching on by default
  
  this->NormalizeVector = false;

  this->Cell = vtkGenericCell::New();
  this->VectorsSelection = 0;

  this->DataSets = new vtkInterpolatedVelocityFieldDataSetsType;
  this->LastDataSet = 0;
  this->LastDataSetIndex = 0;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void vtkInterpolatedVelocityField::SetLastCellId(vtkIdType c, int dataindex) {
  this->LastCellId = c; 
  this->LastDataSet = (*this->DataSets)[dataindex];
  // if the dataset changes, then the cached cell is invalidated
  // we might as well prefetch the cached cell either way
  if (this->LastCellId!=-1)
  {
    this->LastDataSet->GetCell(this->LastCellId, this->GenCell);
  } 
  this->LastDataSetIndex = dataindex;
}
//---------------------------------------------------------------------------
vtkGenericCell *vtkInterpolatedVelocityField::GetLastCell() 
{
  if (this->LastCellId!=-1) 
    {
    return this->GenCell;
    }
  return NULL;
}
//---------------------------------------------------------------------------
static int tmp_count=0;
// Evaluate u,v,w at x,y,z,t
int vtkInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  vtkDataSet* ds;
  if(!this->LastDataSet && !this->DataSets->empty())
    {
    ds = (*this->DataSets)[0];
    this->LastDataSet = ds;
    this->LastDataSetIndex = 0;
    }
  else
    {
    ds = this->LastDataSet;
    }
  int retVal = this->FunctionValues(ds, x, f);
  
  if (!retVal)
    {
    tmp_count = 0;
    for(this->LastDataSetIndex = 0; 
        this->LastDataSetIndex < static_cast<int>(this->DataSets->size());
        this->LastDataSetIndex++)
      {
      ds = this->DataSets->operator[](this->LastDataSetIndex);
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
    this->LastCellId = -1;
    this->LastDataSetIndex = 0;
    this->LastDataSet = (*this->DataSets)[0];
    return 0;
    }
  tmp_count++;
  return retVal;
}
//---------------------------------------------------------------------------
const double vtkInterpolatedVelocityField::TOLERANCE_SCALE = 1.0E-8;

// Evaluate u,v,w at x,y,z,t
int vtkInterpolatedVelocityField::FunctionValues(vtkDataSet* dataset,
                                                 double* x, 
                                                 double* f)
{
  int i, j, subId , numPts, id;
  vtkDataArray* vectors;
  double vec[3];
  double dist2;
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

  double tol2 = 
    dataset->GetLength() * vtkInterpolatedVelocityField::TOLERANCE_SCALE;

  int found = 0;

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
          dataset->FindCell(x, this->Cell, this->GenCell, -1, tol2, 
                            subId, this->LastPCoords, this->Weights);
        if (this->LastCellId != - 1)
          {
          dataset->GetCell(this->LastCellId, this->GenCell);
          found = 1;
          }
        }
      }
    else
      {
      this->CacheHit++;
      found = 1;
      }
    }

  if (!found)
    {
    // if the cell is not found, do a global search (ignore initial
    // cell if there is one)
    this->LastCellId = 
      dataset->FindCell(x, 0, this->GenCell, -1, tol2, 
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
      
    if ( this->NormalizeVector == true )
      {
      vtkMath::Normalize( f );
      }  
    }
  // if not, return false
  else
    {
    return 0;
    }

  return 1;
}
//---------------------------------------------------------------------------
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
    this->Weights = new double[size]; 
    }
}
//---------------------------------------------------------------------------
void vtkInterpolatedVelocityField::FastCompute(
  vtkDataArray* vectors, double f[3])
{
  int j,id;
  double vec[3];
  f[0] = f[1] = f[2] = 0.0;
  int numPts = this->GenCell->GetNumberOfPoints();
  // interpolate the vectors
  for (j=0; j < numPts; j++)
    {
    id = this->GenCell->PointIds->GetId(j);
    vectors->GetTuple(id, vec);
    f[0] +=  vec[0] * this->Weights[j];
    f[1] +=  vec[1] * this->Weights[j];
    f[2] +=  vec[2] * this->Weights[j];
    }
}
//---------------------------------------------------------------------------
bool vtkInterpolatedVelocityField::InterpolatePoint(
  vtkPointData *outPD, vtkIdType outIndex)
{
  if (!this->LastDataSet)
    {
    return 0;
    }
  outPD->InterpolatePoint(this->LastDataSet->GetPointData(), 
    outIndex, this->GenCell->PointIds, this->Weights);
  return 1;
}
//---------------------------------------------------------------------------
int vtkInterpolatedVelocityField::GetLastWeights(double* w)
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
//---------------------------------------------------------------------------
int vtkInterpolatedVelocityField::GetLastLocalCoordinates(double pcoords[3])
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
//---------------------------------------------------------------------------
void vtkInterpolatedVelocityField::CopyParameters(
  vtkInterpolatedVelocityField* from)
{
  this->Caching = from->Caching;
}
//---------------------------------------------------------------------------
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
  os << indent << "LastDataSetIndex : "
     << this->LastDataSetIndex << endl;

}
//---------------------------------------------------------------------------
