//
// DataSet methods
//
#include <math.h>
#include "DataSet.hh"
#define LARGE_NUMBER 1.0e29

vlDataSet::vlDataSet ()
{
  this->PointData = 0;
}
vlDataSet::vlDataSet (const vlDataSet& ds)
{
  this->PointData = ds.PointData;
  this->PointData->Register((void *)this);
}

vlDataSet::~vlDataSet()
{
  vlDataSet::Initialize();
}

void vlDataSet::Initialize()
{
  if ( this->PointData )
    {
    this->PointData->UnRegister((void *)this);
    this->PointData = 0;
    }
};
void vlDataSet::SetPointData (vlPointData* pd)
{
  if ( this->PointData != pd )
    {
    if ( this->PointData ) this->PointData->UnRegister((void *)this);
    this->PointData = pd;
    this->Modified();
    }
}
vlPointData *vlDataSet::GetPointData()
{
  return this->PointData;
}

void vlDataSet::ComputeBounds ()
{
  int i;

  if ( this->Mtime > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  LARGE_NUMBER;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -LARGE_NUMBER;
    for (i=0; i<this->NumPoints(); i++)
      {
        
      }

    this->ComputeTime.Modified();
    }
}

void vlDataSet::GetBounds(float bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++) bounds[i] = this->Bounds[i];
}
  
void vlDataSet::GetCenter(float center[3])
{
  this->ComputeBounds();
  for (int i=0; i<3; i++) 
    center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) / 2.0;
}

float vlDataSet::GetLength()
{
  double diff, l;
  int i;

  this->ComputeBounds();
  for (i=0; i<3; i++)
    {
    diff = this->Bounds[2*i+1] - this->Bounds[2*i];
    l += diff * diff;
    }
 
  return (float)sqrt(l);
}
