/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointCloudFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointCloudFilter.h"

#include "vtkObjectFactory.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkPointSet.h"
#include "vtkDataArray.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMath.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkArrayListTemplate.h" // For processing attribute data


//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// Map input points to output. Basically the third pass of the algorithm.
template <typename T>
struct MapPoints
{
  T *InPoints;
  T *OutPoints;
  const vtkIdType *PointMap;
  ArrayList Arrays;

  MapPoints(vtkIdType, T *inPts, vtkIdType numOutPts, T *outPts,
            vtkIdType *map, vtkPointData *inPD, vtkPointData *outPD) :
    InPoints(inPts), OutPoints(outPts), PointMap(map)
  {
      this->Arrays.AddArrays(numOutPts, inPD, outPD);
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      T *inP, *outP;
      const vtkIdType *map=this->PointMap;
      vtkIdType outPtId;

      for ( ; ptId < endPtId; ++ptId)
      {
        outPtId = map[ptId];
        if ( outPtId != -1 )
        {
          inP = this->InPoints + 3*ptId;
          outP = this->OutPoints + 3*outPtId;
          *outP++ = *inP++;
          *outP++ = *inP++;
          *outP = *inP;
          this->Arrays.Copy(ptId,outPtId);
        }
      }
  }

  static void Execute(vtkIdType numInPts, T *inPts,
                      vtkIdType numOutPts, T *outPts, vtkIdType *map,
                      vtkPointData *inPD, vtkPointData *outPD)
  {
      MapPoints copy(numInPts, inPts, numOutPts, outPts, map, inPD, outPD);
      vtkSMPTools::For(0, numInPts, copy);
  }

}; //MapPoints

//----------------------------------------------------------------------------
// Map outlier points to second output. This is an optional pass of the
// algorithm.
template <typename T>
struct MapOutliers
{
  T *InPoints;
  T *OutPoints;
  const vtkIdType *PointMap;
  ArrayList Arrays;

  MapOutliers(vtkIdType, T *inPts, vtkIdType numOutPts, T *outPts,
              vtkIdType *map, vtkPointData *inPD, vtkPointData *outPD2) :
    InPoints(inPts), OutPoints(outPts), PointMap(map)
  {
      this->Arrays.AddArrays(numOutPts, inPD, outPD2);
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
      T *inP, *outP;
      const vtkIdType *map=this->PointMap;
      vtkIdType outPtId;

      for ( ; ptId < endPtId; ++ptId)
      {
        outPtId = map[ptId];
        if ( outPtId < 0 )
        {
          outPtId = (-outPtId) - 1;
          inP = this->InPoints + 3*ptId;
          outP = this->OutPoints + 3*outPtId;
          *outP++ = *inP++;
          *outP++ = *inP++;
          *outP = *inP;
          this->Arrays.Copy(ptId,outPtId);
        }
      }
  }

  static void Execute(vtkIdType numInPts, T *inPts,
                      vtkIdType numOutPts, T *outPts, vtkIdType *map,
                      vtkPointData *inPD, vtkPointData *outPD2)
  {
      MapOutliers copy(numInPts, inPts, numOutPts, outPts, map, inPD, outPD2);
      vtkSMPTools::For(0, numInPts, copy);
  }

}; //MapOutliers


} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkPointCloudFilter::vtkPointCloudFilter()
{
  this->PointMap = NULL;
  this->NumberOfPointsRemoved = 0;
  this->GenerateOutliers = false;
  this->GenerateVertices = false;

  // Optional second output of outliers
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
vtkPointCloudFilter::~vtkPointCloudFilter()
{
  if ( this->PointMap )
  {
    delete [] this->PointMap;
  }
}

//----------------------------------------------------------------------------
const vtkIdType* vtkPointCloudFilter::GetPointMap()
{
  return this->PointMap;
}

//----------------------------------------------------------------------------
vtkIdType vtkPointCloudFilter::GetNumberOfPointsRemoved()
{
  return this->NumberOfPointsRemoved;
}

//----------------------------------------------------------------------------
// There are three high level passes. First we traverse all the input points
// to see how many neighbors each point has within a specified radius, and a
// map is created indicating whether an input point is to be copied to the
// output. Next a prefix sum is used to count the output points, and to
// update the mapping between the input and the output. Finally, non-removed
// input points (and associated attributes) are copied to the output.
int vtkPointCloudFilter::RequestData(
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

  // Reset the filter
  this->NumberOfPointsRemoved = 0;
  if ( this->PointMap )
  {
    delete [] this->PointMap; //might have executed previously
  }

  // Check input
  if ( !input || !output )
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if ( numPts < 1 )
  {
    return 1;
  }

  // Okay invoke filtering operation. This is always the initial pass.
  this->PointMap = new vtkIdType[numPts];
  if ( ! this->FilterPoints(input) )
  {
    return 1;
  }

  // Count the resulting points (prefix sum). The second pass of the algorithm; it
  // could be threaded but prefix sum does not benefit very much from threading.
  vtkIdType ptId;
  vtkIdType count=0;
  vtkIdType *map = this->PointMap;
  for (ptId=0; ptId < numPts; ++ptId)
  {
    if ( map[ptId] != -1 )
    {
      map[ptId] = count;
      count++;
    }
  }
  this->NumberOfPointsRemoved = numPts - count;

  // If the number of input and output points is the same we short circuit
  // the process. Otherwise, copy the masked input points to the output.
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  if ( this->NumberOfPointsRemoved == 0 )
  {
    output->SetPoints(input->GetPoints());
    outPD->PassData(inPD);
    this->GenerateVerticesIfRequested(output);

    return 1;
  }

  // Okay copy the points from the input to the output. We use a threaded
  // operation that provides a minor benefit (since it's mostly data
  // movement with almost no computation).
  outPD->CopyAllocate(inPD,count);
  vtkPoints *points = input->GetPoints()->NewInstance();
  points->SetDataType(input->GetPoints()->GetDataType());
  points->SetNumberOfPoints(count);
  output->SetPoints(points);

  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  void *outPtr = output->GetPoints()->GetVoidPointer(0);
  switch (output->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(MapPoints<VTK_TT>::Execute(numPts, (VTK_TT *)inPtr, count,
                              (VTK_TT *)outPtr, this->PointMap, inPD, outPD));
  }

  // Generate poly vertex cell if requested
  this->GenerateVerticesIfRequested(output);

  // Clean up. We leave the map in case the user wants to use it.
  points->Delete();

  // Create the second output if requested. Note that we are using a negative
  // count in the map (offset by -1) which indicates the final position of
  // the output point in the second output.
  if ( this->GenerateOutliers && this->NumberOfPointsRemoved > 0 )
  {
    vtkInformation *outInfo2 = outputVector->GetInformationObject(1);
    // get the second output
    vtkPolyData *output2 = vtkPolyData::SafeDownCast(
      outInfo2->Get(vtkDataObject::DATA_OBJECT()));
    vtkPointData *outPD2 = output2->GetPointData();
    outPD2->CopyAllocate(inPD,(count-1));

    // Update map
    count = 1; //offset by one
    map = this->PointMap;
    for (ptId=0; ptId < numPts; ++ptId)
    {
      if ( map[ptId] == -1 )
      {
        map[ptId] = (-count);
        count++;
      }
    }

    // Copy to second output
    vtkPoints *points2 = input->GetPoints()->NewInstance();
    points2->SetDataType(input->GetPoints()->GetDataType());
    points2->SetNumberOfPoints(count-1);
    output2->SetPoints(points2);
    void *outPtr2 = output2->GetPoints()->GetVoidPointer(0);
    switch (output->GetPoints()->GetDataType())
    {
      vtkTemplateMacro(MapOutliers<VTK_TT>::Execute(numPts, (VTK_TT *)inPtr, (count-1),
                                  (VTK_TT *)outPtr2, this->PointMap, inPD, outPD2));
    }
    points2->Delete();

    // Produce poly vertex cell if requested
    this->GenerateVerticesIfRequested(output2);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPointCloudFilter::GenerateVerticesIfRequested(vtkPolyData *output)
{
  vtkIdType numPts;
  if ( ! this->GenerateVertices || output->GetPoints() == NULL ||
       (numPts=output->GetNumberOfPoints()) < 1)
  {
    return;
  }

  // Okay create a cell array and assign it to the output
  vtkCellArray *verts = vtkCellArray::New();
  verts->EstimateSize(1,numPts);

  verts->InsertNextCell(numPts);
  for (vtkIdType ptId=0; ptId < numPts; ++ptId)
  {
    verts->InsertCellPoint(ptId);
  }

  output->SetVerts(verts);
  verts->Delete();
}

//----------------------------------------------------------------------------
int vtkPointCloudFilter::
FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPointCloudFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Points Removed: "
     << this->NumberOfPointsRemoved << "\n";

  os << indent << "Generate Outliers: "
     << (this->GenerateOutliers ? "On\n" : "Off\n");

  os << indent << "Generate Vertices: "
     << (this->GenerateVertices ? "On\n" : "Off\n");

}
