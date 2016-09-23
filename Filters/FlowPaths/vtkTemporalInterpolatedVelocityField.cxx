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
#include "vtkDoubleArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCachingInterpolatedVelocityField.h"
#include "vtkAbstractCellLocator.h"

#include <vector>
//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalInterpolatedVelocityField);
//---------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField::vtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs         = 3; // u, v, w
  this->NumIndepVars     = 4; // x, y, z, t
  this->IVF[0] = vtkSmartPointer<vtkCachingInterpolatedVelocityField>::New();
  this->IVF[1] = vtkSmartPointer<vtkCachingInterpolatedVelocityField>::New();
  this->LastGoodVelocity[0]=0.0;
  this->LastGoodVelocity[1]=0.0;
  this->LastGoodVelocity[2]=0.0;
  this->CurrentWeight=0.0;
  this->OneMinusWeight=1.0;
  this->ScaleCoeff=1.0;

  this->Vals1[0] = this->Vals1[1] = this->Vals1[2] = 0.0;
  this->Vals2[0] = this->Vals2[1] = this->Vals2[2] = 0.0;
  this->Times[0] = this->Times[1] = 0.0;
}

//---------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField::~vtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->SetVectorsSelection(0);
  this->IVF[0] = NULL;
  this->IVF[1] = NULL;
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::SetDataSetAtTime(int I, int N, double T, vtkDataSet* dataset, bool staticdataset)
{
  this->Times[N] = T;
  if ((this->Times[1]-this->Times[0])>0)
  {
    this->ScaleCoeff = 1.0/(this->Times[1]-this->Times[0]);
  }
  if (N==0)
  {
    this->IVF[N]->SetDataSet(I, dataset, staticdataset, NULL);
  }
  // when the datasets for the second time set are added, set the static flag
  if (N==1)
  {
    bool is_static = staticdataset && this->IVF[0]->CacheList[I].StaticDataSet;
    if (static_cast<size_t>(I)>=this->StaticDataSets.size())
    {
      this->StaticDataSets.resize(I+1,is_static);
    }
    if (is_static)
    {
      this->IVF[N]->SetDataSet(I, dataset, staticdataset, this->IVF[0]->CacheList[I].BSPTree);
    }
    else
    {
      this->IVF[N]->SetDataSet(I, dataset, staticdataset, NULL);
    }
  }
}
//---------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::IsStatic(int datasetIndex)
{
  return this->StaticDataSets[datasetIndex];
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::SetVectorsSelection(const char *v)
{
  this->IVF[0]->SelectVectors(v);
  this->IVF[1]->SelectVectors(v);
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::ClearCache()
{
  this->IVF[0]->SetLastCellInfo(-1, 0);
  this->IVF[1]->SetLastCellInfo(-1, 0);
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::SetCachedCellIds(vtkIdType id[2], int ds[2])
{
  if (id[0]!=-1)
  {
    this->IVF[0]->SetLastCellInfo(id[0], ds[0]);
  }
  else
  {
    this->IVF[0]->SetLastCellInfo(-1, 0);
  }
  //
  if (id[1]!=-1)
  {
    this->IVF[1]->SetLastCellInfo(id[1], ds[1]);
  }
  else
  {
    this->IVF[1]->SetLastCellInfo(-1, 0);
  }
}
//---------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::GetCachedCellIds(vtkIdType id[2], int ds[2])
{
  id[0] = this->IVF[0]->LastCellId;
  ds[0] = (id[0]==-1) ? 0 : this->IVF[0]->LastCacheIndex;
  //
  id[1] = this->IVF[1]->LastCellId;
  ds[1] = (id[1]==-1) ? 0 : this->IVF[1]->LastCacheIndex;
  return ((id[0]>=0) && (id[1]>=0));
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::AdvanceOneTimeStep()
{
  for (unsigned int i=0; i<this->IVF[0]->CacheList.size(); i++)
  {
    if (this->IsStatic(i))
    {
      this->IVF[0]->ClearLastCellInfo();
      this->IVF[1]->ClearLastCellInfo();
    }
    else
    {
      this->IVF[0] = this->IVF[1];
      this->IVF[1] = vtkSmartPointer<vtkCachingInterpolatedVelocityField>::New();
    }
  }
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::ShowCacheResults()
{
  vtkErrorMacro(<< ")\n"
                << "T0 - (cell hit : " << this->IVF[0]->CellCacheHit
                << "  (dataset hit : " << this->IVF[0]->DataSetCacheHit-this->IVF[0]->CellCacheHit
                << "         (miss : " << this->IVF[0]->CacheMiss       << ")\n"
                << "T1 - (cell hit : " << this->IVF[1]->CellCacheHit
                << "  (dataset hit : " << this->IVF[1]->DataSetCacheHit-this->IVF[1]->CellCacheHit
                << "         (miss : " << this->IVF[1]->CacheMiss);
}
//---------------------------------------------------------------------------
static double vtkTIVFWeightTolerance = 1E-3;
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::TestPoint(double* x)
{
  this->CurrentWeight  = (x[3]-this->Times[0])*this->ScaleCoeff;
  this->OneMinusWeight = 1.0 - this->CurrentWeight;
  if (this->CurrentWeight<(0.0+vtkTIVFWeightTolerance))
  {
    this->CurrentWeight = 0.0;
  }
  if (this->CurrentWeight>(1.0-vtkTIVFWeightTolerance))
  {
    this->CurrentWeight = 1.0;
  }
  //
  // are we inside dataset at T0
  //
  if (this->IVF[0]->FunctionValues(x, this->Vals1))
  {
    // if we are inside at T0 and static, we must be inside at T1
    if (this->IsStatic(this->IVF[0]->LastCacheIndex))
    {
      // compute using weights from dataset 0 and vectors from dataset 1
      this->IVF[1]->SetLastCellInfo(this->IVF[0]->LastCellId, this->IVF[0]->LastCacheIndex);
      this->IVF[0]->FastCompute(this->IVF[1]->Cache, this->Vals2);
      for (int i=0; i<this->NumFuncs; i++)
      {
        this->LastGoodVelocity[i] = this->OneMinusWeight*this->Vals1[i] + this->CurrentWeight*this->Vals2[i];
      }
      return ID_INSIDE_ALL;
    }
    // dynamic, we need to test at T1
    if (!this->IVF[1]->FunctionValues(x, this->Vals2))
    {
      // inside at T0, but outside at T1, return velocity for T0
      for (int i=0; i<this->NumFuncs; i++)
      {
        this->LastGoodVelocity[i] = this->Vals1[i];
      }
      return ID_OUTSIDE_T1;
    }
    // both valid, compute correct value
    for (int i=0; i<this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = this->OneMinusWeight*this->Vals1[i] + this->CurrentWeight*this->Vals2[i];
    }
    return ID_INSIDE_ALL;
  }
  // Outside at T0, either abort or use T1
  // if we are outside at T0 and static, we must be outside at T1
  if (this->IsStatic(this->IVF[0]->LastCacheIndex))
  {
    return ID_OUTSIDE_ALL;
  }
  // we are dynamic, so test T1
  if (this->IVF[1]->FunctionValues(x, this->Vals2))
  {
    // inside at T1, but outside at T0, return velocity for T1
    for (int i=0; i<this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = this->Vals2[i];
    }
    return ID_OUTSIDE_T0;
  }
  // failed both, so exit
  return ID_OUTSIDE_ALL;
}
//---------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::QuickTestPoint(double* x)
{
  // if outside, return 0
  if (!this->IVF[0]->InsideTest(x))
  {
    return 0;
  }
  // if inside and static dataset hit, skip next test
  if (!this->IsStatic(this->IVF[0]->LastCacheIndex))
  {
    if (!this->IVF[1]->InsideTest(x))
    {
      return 0;
    }
  }
  return 1;
}
//---------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int vtkTemporalInterpolatedVelocityField::FunctionValues(double* x, double* u)
{
  if (this->TestPoint(x)==ID_OUTSIDE_ALL)
  {
    return 0;
  }
  for (int i=0; i<this->NumFuncs; i++)
  {
    u[i] = this->LastGoodVelocity[i];
  }
  return 1;
}
//---------------------------------------------------------------------------
int vtkTemporalInterpolatedVelocityField::FunctionValuesAtT(int T, double* x, double* u)
{
  //
  // Try velocity at T0
  //
  if (T==0)
  {
    if (!this->IVF[0]->FunctionValues(x, this->Vals1))
    {
      return 0;
    }
    for (int i=0; i<this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = u[i] = this->Vals1[i];
    }
    if (this->IsStatic(this->IVF[0]->LastCacheIndex))
    {
      this->IVF[1]->SetLastCellInfo(this->IVF[0]->LastCellId, this->IVF[0]->LastCacheIndex);
    }
  }
  //
  // Try velocity at T1
  //
  else if (T==1)
  {
    if (!this->IVF[1]->FunctionValues(x, this->Vals2))
    {
      return 0;
    }
    for (int i=0; i<this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = u[i] = this->Vals2[i];
    }
    if (this->IsStatic(this->IVF[1]->LastCacheIndex))
    {
      this->IVF[0]->SetLastCellInfo(this->IVF[1]->LastCellId, this->IVF[1]->LastCacheIndex);
    }
  }
  return 1;
}
//---------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::InterpolatePoint(
  vtkPointData *outPD1, vtkPointData *outPD2,
  vtkIdType outIndex)
{
  bool ok1 = this->IVF[0]->InterpolatePoint(outPD1, outIndex);
  bool ok2 = this->IVF[1]->InterpolatePoint(outPD2, outIndex);
  return (ok1 || ok2);
}
//---------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::InterpolatePoint(
  int T, vtkPointData *outPD1, vtkIdType outIndex)
{
  vtkCachingInterpolatedVelocityField* inivf = this->IVF[T];
  // force use of correct weights/etc if static as only T0 are valid
  if (T==1 && this->IsStatic(this->IVF[T]->LastCacheIndex))
  {
    T=0;
  }
  //
  return this->IVF[T]->InterpolatePoint(inivf, outPD1, outIndex);
}
//---------------------------------------------------------------------------
bool vtkTemporalInterpolatedVelocityField::GetVorticityData(
  int T, double pcoords[3], double *weights,
  vtkGenericCell *&cell, vtkDoubleArray *cellVectors)
{
  // force use of correct weights/etc if static as only T0 are valid
  if (T==1 && this->IsStatic(this->IVF[T]->LastCacheIndex))
  {
    T=0;
  }
  //
  if (this->IVF[T]->GetLastWeights(weights) &&
      this->IVF[T]->GetLastLocalCoordinates(pcoords) &&
      (cell=this->IVF[T]->GetLastCell()) )
  {
    vtkDataSet   *ds = this->IVF[T]->Cache->DataSet;
    vtkPointData *pd = ds->GetPointData();
    vtkDataArray *da = pd->GetVectors(this->IVF[T]->GetVectorsSelection());
    da->GetTuples(cell->PointIds, cellVectors);
    return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
void vtkTemporalInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LastGoodVelocity: "
     << this->LastGoodVelocity[0] << ", "
     << this->LastGoodVelocity[1] << ", "
     << this->LastGoodVelocity[2] << endl;
  os << indent << "CurrentWeight: " << this->CurrentWeight << endl;
}
//---------------------------------------------------------------------------
