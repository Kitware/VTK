/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDensifyPointCloudFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDensifyPointCloudFilter.h"

#include "vtkObjectFactory.h"
#include "vtkStaticPointLocator.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkArrayListTemplate.h" // For processing attribute data

vtkStandardNewMacro(vtkDensifyPointCloudFilter);


//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// Count the number of points that need generation
template <typename T>
struct CountPoints
{
  T *InPoints;
  vtkStaticPointLocator *Locator;
  vtkIdType *Count;
  int NeighborhoodType;
  int NClosest;
  double Radius;
  double Distance;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage prevents lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  CountPoints(T *inPts, vtkStaticPointLocator *loc, vtkIdType *count, int ntype,
              int nclose, double r, double d) : InPoints(inPts), Locator(loc),
                                                Count(count), NeighborhoodType(ntype),
                                                NClosest(nclose), Radius(r), Distance(d)
  {
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
  }

  void operator() (vtkIdType pointId, vtkIdType endPointId)
  {
    T *x, *p = this->InPoints + 3*pointId;
    vtkStaticPointLocator *loc = this->Locator;
    vtkIdType *count = this->Count + pointId;
    vtkIdList*& pIds = this->PIds.Local();
    vtkIdType i, id, numIds, numNewPts;
    double px[3], py[3];
    int ntype = this->NeighborhoodType;
    double radius = this->Radius;
    int nclose = this->NClosest;
    double d2 = this->Distance * this->Distance;

    for ( ; pointId < endPointId; ++pointId, p+=3 )
    {
      numNewPts = 0;
      px[0] = static_cast<double>(p[0]);
      px[1] = static_cast<double>(p[1]);
      px[2] = static_cast<double>(p[2]);
      if ( ntype == vtkDensifyPointCloudFilter::N_CLOSEST )
      {
        // use nclose+1 because we want to discount ourselves
        loc->FindClosestNPoints(nclose+1, px, pIds);
      }
      else // ntype == vtkDensifyPointCloudFilter::RADIUS
      {
        loc->FindPointsWithinRadius(radius, px, pIds);
      }
      numIds = pIds->GetNumberOfIds();

      for ( i=0; i < numIds; ++i)
      {
        id = pIds->GetId(i);
        if ( id > pointId ) //only process points of larger id
        {
          x = this->InPoints + 3*id;
          py[0] = static_cast<double>(x[0]);
          py[1] = static_cast<double>(x[1]);
          py[2] = static_cast<double>(x[2]);

          if ( vtkMath::Distance2BetweenPoints(px,py) >= d2 )
          {
            numNewPts++;
          }
        }//larger id
      }//for all neighbors
      *count++ = numNewPts;
    }//for all points in this batch
  }

  void Reduce()
  {
  }

  static void Execute(vtkIdType numPts, T *pts, vtkStaticPointLocator *loc,
                      vtkIdType *count, int ntype, int nclose, double r, double d)
  {
    CountPoints countPts(pts, loc, count, ntype, nclose, r, d);
    vtkSMPTools::For(0, numPts, countPts);
  }

}; //CountPoints

//----------------------------------------------------------------------------
// Count the number of points that need generation
template <typename T>
struct GeneratePoints
{
  T *InPoints;
  vtkStaticPointLocator *Locator;
  const vtkIdType *Offsets;
  int NeighborhoodType;
  int NClosest;
  double Radius;
  double Distance;
  ArrayList Arrays;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage prevents lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;

  GeneratePoints(T *inPts, vtkStaticPointLocator *loc, vtkIdType *offset,
                 int ntype, int nclose, double r, double d, vtkIdType numPts,
                 vtkPointData *attr) : InPoints(inPts), Locator(loc), Offsets(offset),
                                       NeighborhoodType(ntype), NClosest(nclose),
                                       Radius(r), Distance(d)
  {
    this->Arrays.AddSelfInterpolatingArrays(numPts, attr);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
  }

  void operator() (vtkIdType pointId, vtkIdType endPointId)
  {
    T *x, *p = this->InPoints + 3*pointId;
    T *newX;
    vtkStaticPointLocator *loc = this->Locator;
    vtkIdList*& pIds = this->PIds.Local();
    vtkIdType i, id, numIds;
    vtkIdType outPtId = this->Offsets[pointId];
    double px[3], py[3];
    int ntype = this->NeighborhoodType;
    double radius = this->Radius;
    int nclose = this->NClosest;
    double d2 = this->Distance * this->Distance;

    for ( ; pointId < endPointId; ++pointId, p+=3 )
    {
      px[0] = static_cast<double>(p[0]);
      px[1] = static_cast<double>(p[1]);
      px[2] = static_cast<double>(p[2]);
      if ( ntype == vtkDensifyPointCloudFilter::N_CLOSEST )
      {
        // use nclose+1 because we want to discount ourselves
        loc->FindClosestNPoints(nclose+1, px, pIds);
      }
      else // ntype == vtkDensifyPointCloudFilter::RADIUS
      {
        loc->FindPointsWithinRadius(radius, px, pIds);
      }
      numIds = pIds->GetNumberOfIds();

      for ( i=0; i < numIds; ++i)
      {
        id = pIds->GetId(i);
        if ( id > pointId ) //only process points of larger id
        {
          x = this->InPoints + 3*id;
          py[0] = static_cast<double>(x[0]);
          py[1] = static_cast<double>(x[1]);
          py[2] = static_cast<double>(x[2]);

          if ( vtkMath::Distance2BetweenPoints(px,py) >= d2 )
          {
            newX = this->InPoints + 3*outPtId;
            *newX++ = static_cast<T>(0.5 * (px[0]+py[0]));
            *newX++ = static_cast<T>(0.5 * (px[1]+py[1]));
            *newX++ = static_cast<T>(0.5 * (px[2]+py[2]));
            this->Arrays.InterpolateEdge(pointId,id,0.5,outPtId);
            outPtId++;
          }
        }//larger id
      }//for all neighbor points
    }//for all points in this batch
  }

  void Reduce()
  {
  }

  static void Execute(vtkIdType numInPts, T *pts, vtkStaticPointLocator *loc,
                      vtkIdType *offsets, int ntype, int nclose, double r,
                      double d, vtkIdType numOutPts, vtkPointData *PD)
  {
    GeneratePoints genPts(pts, loc, offsets, ntype, nclose, r, d, numOutPts, PD);
    vtkSMPTools::For(0, numInPts, genPts);
  }

}; //GeneratePoints

} //anonymous namespace


//================= Begin VTK class proper =======================================
//----------------------------------------------------------------------------
vtkDensifyPointCloudFilter::vtkDensifyPointCloudFilter()
{

  this->NeighborhoodType = vtkDensifyPointCloudFilter::N_CLOSEST;
  this->Radius = 1.0;
  this->NumberOfClosestPoints = 6;
  this->TargetDistance = 0.5;
  this->MaximumNumberOfIterations = 3;
  this->InterpolateAttributeData = true;
  this->MaximumNumberOfPoints = VTK_ID_MAX;
}

//----------------------------------------------------------------------------
vtkDensifyPointCloudFilter::~vtkDensifyPointCloudFilter()
{
}

//----------------------------------------------------------------------------
// Produce the output data
int vtkDensifyPointCloudFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check the input
  if ( !input || !output )
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if ( numPts < 1 )
  {
    return 1;
  }

  // Start by building the locator, creating the output points and otherwise
  // and prepare for iteration.
  int iterNum;
  vtkStaticPointLocator *locator = vtkStaticPointLocator::New();

  vtkPoints *inPts = input->GetPoints();
  int pointsType = inPts->GetDataType();
  vtkPoints *newPts = inPts->NewInstance();
  newPts->DeepCopy(inPts);
  output->SetPoints(newPts);
  vtkPointData *outPD=NULL;
  if ( this->InterpolateAttributeData )
  {
    outPD = output->GetPointData();
    outPD->DeepCopy(input->GetPointData());
    outPD->InterpolateAllocate(outPD,numPts);
  }

  vtkIdType ptId, numInPts, numNewPts;
  vtkIdType npts, offset, *offsets;
  void *pts=NULL;
  double d = this->TargetDistance;

  // Loop over the data, bisecting connecting edges as required.
  for ( iterNum=0; iterNum < this->MaximumNumberOfIterations; ++iterNum )
  {
    // Prepare to process
    locator->SetDataSet(output);
    locator->Modified();
    locator->BuildLocator();

    // Count the number of points to create
    numInPts = output->GetNumberOfPoints();
    offsets = new vtkIdType [numInPts];
    pts = output->GetPoints()->GetVoidPointer(0);
    switch (pointsType)
    {
      vtkTemplateMacro(CountPoints<VTK_TT>::Execute(numInPts,
                      (VTK_TT *)pts, locator, offsets, this->NeighborhoodType,
                      this->NumberOfClosestPoints, this->Radius, d));
    }

    // Prefix sum to count the number of points created and build offsets
    numNewPts = 0;
    offset = numInPts;
    for (ptId=0; ptId < numInPts; ++ptId)
    {
      npts = offsets[ptId];
      offsets[ptId] = offset;
      offset += npts;
    }
    numNewPts = offset - numInPts;

    // Check convergence
    if ( numNewPts == 0 || offset > this->MaximumNumberOfPoints )
    {
      delete [] offsets;
      break;
    }

    // Now add points and attribute data if requested. Allocate memory
    // for points and attributes.
    newPts->InsertPoint(offset,0.0,0.0,0.0); //side effect reallocs memory
    pts = output->GetPoints()->GetVoidPointer(0);
    switch (pointsType)
    {
      vtkTemplateMacro(GeneratePoints<VTK_TT>::Execute(numInPts,
                       (VTK_TT *)pts, locator, offsets, this->NeighborhoodType,
                       this->NumberOfClosestPoints, this->Radius, d,
                       offset, outPD));
    }

    delete [] offsets;
  } //while max num of iterations not exceeded

  // Clean up
  locator->Delete();
  newPts->Delete();

  return 1;
}


//----------------------------------------------------------------------------
int vtkDensifyPointCloudFilter::
FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDensifyPointCloudFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Neighborhood Type: " << this->GetNeighborhoodType() << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number Of Closest Points: "
     << this->NumberOfClosestPoints << "\n";
  os << indent << "Target Distance: " << this->TargetDistance << endl;
  os << indent << "Maximum Number of Iterations: "
     << this->MaximumNumberOfIterations << "\n";
  os << indent << "Interpolate Attribute Data: "
     << (this->InterpolateAttributeData ? "On\n" : "Off\n");
  os << indent << "Maximum Number Of Points: "
     << this->MaximumNumberOfPoints << "\n";
}
