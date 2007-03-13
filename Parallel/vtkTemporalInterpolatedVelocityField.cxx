/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalInterpolatedVelocityField.h"

#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkInterpolatedVelocityField.h"

#include <vtkstd/vector>
//---------------------------------------------------------------------------
class vtkTInterpolatedVelocityFieldDataSetsType: 
  public vtkstd::vector< vtkDataSet* > {};
//---------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkTemporalInterpolatedVelocityField, "1.1");
vtkStandardNewMacro(vtkTemporalInterpolatedVelocityField);
//---------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField::vtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs         = 3; // u, v, w
  this->NumIndepVars     = 4; // x, y, z, t
  this->ivf[0] = vtkInterpolatedVelocityField::New();
  this->ivf[1] = vtkInterpolatedVelocityField::New();
  this->DataSets[0] = new vtkTInterpolatedVelocityFieldDataSetsType();
  this->DataSets[1] = new vtkTInterpolatedVelocityFieldDataSetsType();
  this->GeometryFixed = 0;
}
//---------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField::~vtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->SetVectorsSelection(0);
  this->ivf[0]->Delete();
  this->ivf[1]->Delete();
  delete this->DataSets[0];
  delete this->DataSets[1];
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::ClearCache()
{
  this->ivf[0]->SetLastCellId(-1, 0);
  this->ivf[1]->SetLastCellId(-1, 0);
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::SetCachedCellIds(vtkIdType id[2], int ds[2])
{
  if (id[0]!=-1) {
    this->ivf[0]->SetLastCellId(id[0], ds[0]);
  }
  else this->ivf[0]->SetLastCellId(-1, 0);
  if (this->GeometryFixed) return;
  //
  if (id[1]!=-1) {
    this->ivf[1]->SetLastCellId(id[1], ds[1]); 
  }
  else this->ivf[1]->SetLastCellId(-1, 0);
}
//---------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::GetCachedCellIds(vtkIdType id[2], int ds[2])
{
  id[0] = this->ivf[0]->GetLastCellId();
  ds[0] = (id[0]==-1) ? 0 : this->ivf[0]->GetLastDataSetIndex();
  if (this->GeometryFixed) {
    id[1] = id[0];
    ds[1] = ds[0];
    return (id[0]>=0);
  }
  //
  id[1] = this->ivf[1]->GetLastCellId();
  ds[1] = (id[1]==-1) ? 0 : this->ivf[1]->GetLastDataSetIndex();
  return ((id[0]>=0) && (id[1]>=0));
}
//---------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::TestPoint(double* x)
{
  int result = ID_INSIDE_ALL;
  if (!this->ivf[0]->FunctionValues(x, vals1)) 
    result = ID_OUTSIDE_T1;
  if (this->GeometryFixed) {
    return (result == ID_INSIDE_ALL) ? ID_INSIDE_ALL : ID_OUTSIDE_ALL;
  }
  //
  if (!this->ivf[1]->FunctionValues(x, vals2)) 
    result = (result==ID_OUTSIDE_T1) ? ID_OUTSIDE_ALL : ID_OUTSIDE_T2;
  return result;
}
//---------------------------------------------------------------------------
static double WeightTolerance = 1E-3;
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::FunctionValues(double* x, double* u)
{
  double weight = (x[3]-this->times[0])*this->scalecoeff;
  if (weight<(0.0-WeightTolerance)) weight = 0.0;
  if (weight>(1.0+WeightTolerance)) weight = 1.0;
  //
  if (weight==0.0) {
    if (this->ivf[0]->FunctionValues(x, vals1)) {
      for (int i=0; i<this->NumFuncs; i++) {
        this->LastGoodVelocity[i] = u[i] = vals1[i];
      }
      return 1;
    }
    return 0;
  }
  else if (weight==1.0) {
    if (this->ivf[1]->FunctionValues(x, vals2)) {
      for (int i=0; i<this->NumFuncs; i++) {
        this->LastGoodVelocity[i] = u[i] = vals2[i];
      }
      return 1;
    }
    return 0;
  } 
  // if in between T values
  if (!this->GeometryFixed) {
    int result = TestPoint(x);
    if (result==ID_INSIDE_ALL) {
      for (int i=0; i<this->NumFuncs; i++) {
        this->LastGoodVelocity[i] = u[i] = (1.0-weight)*vals1[i] + weight*vals2[i];
      }
    }
    return result==ID_INSIDE_ALL;
  }
  // Geometry is the same for both time steps, so save lots of work
  // by using the weights from the first on the second
  else {
    if (!this->ivf[0]->FunctionValues(x, vals1)) return 0;
    // get the vectors from dataset 1, using the cached data from dataset 0
    vtkDataArray *vectors = 
      this->DataSets[1]->operator[](this->ivf[0]->GetLastDataSetIndex())->
      GetPointData()->GetArray(this->ivf[0]->GetVectorsSelection());
    if (!vectors) {
      // << "Help! " << (this->ivf[0]->GetVectorsSelection() ? this->ivf[0]->GetVectorsSelection() : " NULL");
      return 0;
    }
    ivf[0]->FastCompute(vectors, vals2);
    for (int i=0; i<this->NumFuncs; i++) {
      this->LastGoodVelocity[i] = u[i] = (1.0-weight)*vals1[i] + weight*vals2[i];
    }
  }
  return 1;
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::AddDataSetAtTime(int N, double T, vtkDataSet* dataset)
{
  if (!dataset)
    {
    return;
    }
  //
  this->times[N] = T;
  this->DataSets[N]->push_back(dataset);
  this->ivf[N]->AddDataSet(dataset);
  if ((this->times[1]-this->times[0])>0) 
  {
    this->scalecoeff = 1.0/(this->times[1]-this->times[0]);
  }
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::SetVectorsSelection(const char *v)
{
  this->ivf[0]->SelectVectors(v);
  this->ivf[1]->SelectVectors(v);
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
