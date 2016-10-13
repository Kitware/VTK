/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelGrid.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVoxelGrid.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkPointSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStaticPointLocator.h"
#include "vtkLinearKernel.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkArrayListTemplate.h" // For processing attribute data

#include <vector>


vtkStandardNewMacro(vtkVoxelGrid);
vtkCxxSetObjectMacro(vtkVoxelGrid,Kernel,vtkInterpolationKernel);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

//----------------------------------------------------------------------------
// The threaded core of the algorithm (first pass)
template <typename T>
struct Subsample
{
  T *InPoints;
  vtkStaticPointLocator *Locator;
  vtkInterpolationKernel *Kernel;
  const vtkIdType *BinMap;
  ArrayList Arrays;
  T *OutPoints;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage prevents lots of new/delete.
  vtkSMPThreadLocalObject<vtkIdList> PIds;
  vtkSMPThreadLocalObject<vtkDoubleArray> Weights;

  Subsample(T* inPts, vtkPointData *inPD, vtkPointData *outPD,
            vtkStaticPointLocator *loc, vtkInterpolationKernel *k,
            vtkIdType numOutPts, vtkIdType *binMap, T *outPts) :
    InPoints(inPts), Locator(loc), Kernel(k), BinMap(binMap), OutPoints(outPts)
  {
    this->Arrays.AddArrays(numOutPts, inPD, outPD);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
    vtkDoubleArray*& weights = this->Weights.Local();
    weights->Allocate(128);
  }

  void operator() (vtkIdType pointId, vtkIdType endPointId)
  {
    T *px;
    T *py = this->OutPoints + 3*pointId;
    const vtkIdType *map = this->BinMap;
    vtkIdList*& pIds = this->PIds.Local();
    vtkIdType numWeights;
    vtkDoubleArray*& weights = this->Weights.Local();
    double y[3], count;
    vtkIdType numIds, id;
    vtkStaticPointLocator *loc = this->Locator;

    for ( ; pointId < endPointId; ++pointId )
    {
      vtkIdType binId = map[pointId];

      y[0] = y[1] = y[2] = 0.0;
      loc->GetBucketIds(binId,pIds);
      numIds = pIds->GetNumberOfIds();
      for (id=0; id < numIds; ++id)
      {
        px = this->InPoints + 3*pIds->GetId(id);
        y[0] += *px++;
        y[1] += *px++;
        y[2] += *px;
      }
      count = static_cast<double>(numIds);
      y[0] /= count;
      y[1] /= count;
      y[2] /= count;
      *py++ = y[0];
      *py++ = y[1];
      *py++ = y[2];

      // Now interpolate attributes
      numWeights = this->Kernel->ComputeWeights(y, pIds, weights);
      this->Arrays.Interpolate(numWeights, pIds->GetPointer(0),
                               weights->GetPointer(0), pointId);
    }//for all output points in this batch
  }

  void Reduce()
  {
  }

  static void Execute(T *inPts, vtkPointData *inPD, vtkPointData *outPD,
                      vtkStaticPointLocator *loc, vtkInterpolationKernel *k,
                      vtkIdType numOutPts, vtkIdType *binMap, T *outPts)
  {
    Subsample subsample(inPts, inPD, outPD, loc, k, numOutPts, binMap, outPts);
    vtkSMPTools::For(0, numOutPts, subsample);
  }

}; //Subsample

} //anonymous namespace


//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkVoxelGrid::vtkVoxelGrid()
{
  this->Locator = vtkStaticPointLocator::New();
  this->ConfigurationStyle = vtkVoxelGrid::AUTOMATIC;

  this->Divisions[0] = this->Divisions[1] = this->Divisions[2] = 50;
  this->LeafSize[0] = this->LeafSize[1] = this->LeafSize[2] = 1.0;
  this->NumberOfPointsPerBin = 10;

  this->Kernel = vtkLinearKernel::New();
}

//----------------------------------------------------------------------------
vtkVoxelGrid::~vtkVoxelGrid()
{
  this->Locator->UnRegister(this);
  this->Locator = NULL;
  this->SetKernel(NULL);
}

//----------------------------------------------------------------------------
// Produce the output data
int vtkVoxelGrid::RequestData(
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

  // Make sure there is a kernel
  if ( !this->Kernel )
  {
    vtkErrorMacro(<<"Interpolation kernel required\n");
    return 1;
  }

  bool valid = 1;
  if ( this->LeafSize[0] <= 0.0 || this->LeafSize[1] <= 0.0 || this->LeafSize[2] <= 0.0 ||
       this->Divisions[0] < 1 || this->Divisions[1] < 1 || this->Divisions[2] < 1 )
  {
    valid = false;
  }

  // Configure and build the locator
  if ( valid && this->ConfigurationStyle == vtkVoxelGrid::MANUAL )
  {
    this->Locator->AutomaticOff();
    this->Locator->SetDivisions(this->Divisions);
  }
  else if ( valid && this->ConfigurationStyle == vtkVoxelGrid::SPECIFY_LEAF_SIZE )
  {
    double bounds[6];
    int divs[3];
    this->Locator->AutomaticOff();
    input->GetBounds(bounds);
    divs[0] = (bounds[1]-bounds[0]) / this->LeafSize[0];
    divs[1] = (bounds[3]-bounds[2]) / this->LeafSize[1];
    divs[2] = (bounds[5]-bounds[4]) / this->LeafSize[2];
    this->Locator->SetDivisions(divs);
  }
  else // this->ConfigurationStyle == vtkVoxelGrid::AUTOMATIC
  {
    this->Locator->AutomaticOn();
    this->Locator->SetNumberOfPointsPerBucket(this->NumberOfPointsPerBin);
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Run through the locator and compute the number of output points,
  // and build a map of the bin number to output point. This is a prefix sum.
  vtkIdType numOutPts=0;
  vtkIdType binNum, numBins = this->Locator->GetNumberOfBuckets();
  std::vector<vtkIdType> binMap;
  for ( binNum=0; binNum < numBins; ++binNum )
  {
    if ( this->Locator->GetNumberOfPointsInBucket(binNum) > 0 )
    {
      binMap.push_back(binNum);
      ++numOutPts;
    }
  }

  // Grab the point data for interpolation
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,numOutPts);

  // Finally run over all of the bins, and those that are not empty are
  // processed. The processing consists of averaging all of the points found
  // in the bin, and setting the average point position in the output points.
  vtkPoints *points = input->GetPoints()->NewInstance();
  points->SetDataType(input->GetPoints()->GetDataType());
  points->SetNumberOfPoints(numOutPts);
  output->SetPoints(points);

  void *inPtr = input->GetPoints()->GetVoidPointer(0);
  void *outPtr = output->GetPoints()->GetVoidPointer(0);
  switch (output->GetPoints()->GetDataType())
  {
    vtkTemplateMacro(Subsample<VTK_TT>::Execute((VTK_TT *)inPtr, inPD, outPD,
            this->Locator, this->Kernel, numOutPts, &binMap[0], (VTK_TT *)outPtr));
  }

  // Send attributes to output
  int numPtArrays = input->GetPointData()->GetNumberOfArrays();
  for (int i=0; i<numPtArrays; ++i)
  {
    output->GetPointData()->AddArray(input->GetPointData()->GetArray(i));
  }

  // Clean up. The locator needs to be reset.
  this->Locator->Initialize();
  points->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkVoxelGrid::
FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkVoxelGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Configuration Style: " << this->ConfigurationStyle << endl;

  os << indent << "Divisions: ("
     << this->Divisions[0] << ","
     << this->Divisions[1] << ","
     << this->Divisions[2] << ")\n";

  os << indent << "Leaf Size: ("
     << this->LeafSize[0] << ","
     << this->LeafSize[1] << ","
     << this->LeafSize[2] << ")\n";

  os << indent << "Number of Points Per Bin: "
     << this->NumberOfPointsPerBin << endl;
}
