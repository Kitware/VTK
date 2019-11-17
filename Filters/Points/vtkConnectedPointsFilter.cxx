/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConnectedPointsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConnectedPointsFilter.h"

#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStaticPointLocator.h"

vtkStandardNewMacro(vtkConnectedPointsFilter);
vtkCxxSetObjectMacro(vtkConnectedPointsFilter, Locator, vtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Construct with default extraction mode to extract largest regions.
vtkConnectedPointsFilter::vtkConnectedPointsFilter()
{
  // How to define local neighborhood
  this->Radius = 1.0;

  // How to extract points
  this->ExtractionMode = VTK_EXTRACT_ALL_REGIONS;

  // Seeding the extracted regions
  this->Seeds = vtkIdList::New();
  this->SpecifiedRegionIds = vtkIdList::New();

  // Support extraction of region closest to specified point
  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;

  // Segmenting based on normal alignment
  this->AlignedNormals = 0;
  this->NormalAngle = 10.0;
  this->NormalThreshold = cos(vtkMath::RadiansFromDegrees(this->NormalAngle));

  // If segmenting based on scalar values
  this->ScalarConnectivity = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  // Perform local operations efficiently
  this->Locator = vtkStaticPointLocator::New();

  // The labeling of points (i.e., their associated regions)
  this->CurrentRegionNumber = 0;
  this->RegionLabels = nullptr;

  // Keep track of region sizes
  this->NumPointsInRegion = 0;
  this->RegionSizes = vtkIdTypeArray::New();

  // Processing waves
  this->NeighborPointIds = vtkIdList::New();
  this->Wave = nullptr;
  this->Wave2 = nullptr;
}

//----------------------------------------------------------------------------
vtkConnectedPointsFilter::~vtkConnectedPointsFilter()
{
  this->Seeds->Delete();
  this->SpecifiedRegionIds->Delete();

  if (this->RegionLabels != nullptr)
  {
    this->RegionLabels->Delete();
  }
  this->RegionSizes->Delete();
  this->NeighborPointIds->Delete();

  this->SetLocator(nullptr);
}

//----------------------------------------------------------------------------
int vtkConnectedPointsFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Executing point connectivity filter.");

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    vtkDebugMacro(<< "No data to connect!");
    return 1;
  }
  vtkPoints* inPts = input->GetPoints();

  // Attribute data
  vtkPointData *pd = input->GetPointData(), *outputPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outputCD = output->GetCellData();

  // Grab normals if available and needed
  vtkFloatArray* normals = vtkFloatArray::SafeDownCast(pd->GetNormals());
  float* n = nullptr;
  if (normals && this->AlignedNormals)
  {
    n = static_cast<float*>(normals->GetVoidPointer(0));
    this->NormalThreshold = cos(vtkMath::RadiansFromDegrees(this->NormalAngle));
  }

  // Start by building the locator.
  if (!this->Locator)
  {
    vtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // See whether to consider scalar connectivity
  //
  vtkDataArray* inScalars = input->GetPointData()->GetScalars();
  if (!this->ScalarConnectivity)
  {
    inScalars = nullptr;
  }
  else
  {
    if (this->ScalarRange[1] < this->ScalarRange[0])
    {
      this->ScalarRange[1] = this->ScalarRange[0];
    }
  }

  // Initialize.  Keep track of points and cells visited.
  //
  this->RegionSizes->Reset();
  this->RegionLabels = vtkIdTypeArray::New();
  this->RegionLabels->SetName("RegionLabels");
  this->RegionLabels->SetNumberOfTuples(numPts);
  vtkIdType* labels = static_cast<vtkIdType*>(this->RegionLabels->GetVoidPointer(0));
  std::fill_n(labels, numPts, -1);

  // This is an incremental (propagating wave) traversal of the points. The
  // traversal is a function of proximity, planarity, and/or position on a
  // plane
  vtkIdType ptId;
  this->Wave = vtkIdList::New();
  this->Wave->Allocate(numPts / 4 + 1, numPts);
  this->Wave2 = vtkIdList::New();
  this->Wave2->Allocate(numPts / 4 + 1, numPts);

  // Traverse all points, and label all points
  if (this->ExtractionMode == VTK_EXTRACT_ALL_REGIONS ||
    this->ExtractionMode == VTK_EXTRACT_LARGEST_REGION ||
    this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS)
  {
    this->CurrentRegionNumber = 0;

    for (ptId = 0; ptId < numPts; ++ptId)
    {
      if (labels[ptId] < 0) // not yet visited
      {
        this->Wave->InsertNextId(ptId); // begin next connected wave
        this->NumPointsInRegion = 1;
        labels[ptId] = this->CurrentRegionNumber;
        this->TraverseAndMark(inPts, inScalars, n, labels);
        this->RegionSizes->InsertValue(this->CurrentRegionNumber++, this->NumPointsInRegion);
        this->Wave->Reset();
        this->Wave2->Reset();
      }
    } // for all points

    if (this->ExtractionMode == VTK_EXTRACT_ALL_REGIONS)
    {
      // Can just copy input to output, add label array
      output->CopyStructure(input);
      outputPD->PassData(pd);
      outputCD->PassData(cd);

      outputPD->AddArray(this->RegionLabels);
      outputPD->SetActiveScalars("RegionLabels");
      this->RegionLabels->Delete();
      this->RegionLabels = nullptr;
    }

    else if (this->ExtractionMode == VTK_EXTRACT_LARGEST_REGION)
    {
      vtkIdType regionSize, largestRegion = 0, largestRegionSize = 0;
      vtkIdType numRegions = this->RegionSizes->GetNumberOfTuples();
      for (vtkIdType regNum = 0; regNum < numRegions; ++regNum)
      {
        regionSize = this->RegionSizes->GetValue(regNum);
        if (regionSize > largestRegionSize)
        {
          largestRegionSize = regionSize;
          largestRegion = regNum;
        }
      }

      // Now create output: loop over points and find those that are in largest
      // region
      vtkPoints* outPts = vtkPoints::New(inPts->GetDataType());
      outputPD->CopyAllocate(pd);

      vtkIdType newId;
      for (ptId = 0; ptId < numPts; ++ptId)
      {
        // valid region ids (non-negative) are output.
        if (labels[ptId] == largestRegion)
        {
          newId = outPts->InsertNextPoint(inPts->GetPoint(ptId));
          outputPD->CopyData(pd, ptId, newId);
        }
      }
      output->SetPoints(outPts);
      outPts->Delete();
    }

    else // if ( this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS )
    {
      vtkPoints* outPts = vtkPoints::New(inPts->GetDataType());
      outputPD->CopyAllocate(pd);

      vtkIdType newId;
      for (ptId = 0; ptId < numPts; ++ptId)
      {
        // valid region ids (non-negative) are output.
        if (labels[ptId] >= 0 && this->SpecifiedRegionIds->IsId(labels[ptId]) >= 0)
        {
          newId = outPts->InsertNextPoint(inPts->GetPoint(ptId));
          outputPD->CopyData(pd, ptId, newId);
        }
      }
      output->SetPoints(outPts);
      outPts->Delete();
    }
  } // need to process all regions

  // Otherwise just a subset of points is extracted and labeled
  else
  {
    this->CurrentRegionNumber = 0;
    this->NumPointsInRegion = 0;
    if (this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS)
    {
      for (int i = 0; i < this->Seeds->GetNumberOfIds(); i++)
      {
        ptId = this->Seeds->GetId(i);
        if (ptId >= 0)
        {
          labels[ptId] = this->CurrentRegionNumber;
          this->NumPointsInRegion++;
          this->Wave->InsertNextId(ptId);
        }
      }
    }
    else if (this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION)
    {
      ptId = this->Locator->FindClosestPoint(this->ClosestPoint);
      if (ptId >= 0)
      {
        labels[ptId] = this->CurrentRegionNumber;
        this->NumPointsInRegion++;
        this->Wave->InsertNextId(ptId);
      }
    }

    // Mark all seeded regions
    this->TraverseAndMark(inPts, inScalars, n, labels);
    this->RegionSizes->InsertValue(this->CurrentRegionNumber, this->NumPointsInRegion);

    // Now create output: loop over points and find those that are marked.
    vtkPoints* outPts = vtkPoints::New(inPts->GetDataType());
    outputPD->CopyAllocate(pd);

    vtkIdType newId;
    for (ptId = 0; ptId < numPts; ++ptId)
    {
      // valid region ids (non-negative) are output.
      if (labels[ptId] >= 0)
      {
        newId = outPts->InsertNextPoint(inPts->GetPoint(ptId));
        outputPD->CopyData(pd, ptId, newId);
      }
    }
    output->SetPoints(outPts);
    outPts->Delete();
  }

  vtkDebugMacro(<< "Extracted " << output->GetNumberOfPoints() << " points");

  // Clean up
  this->Wave->Delete();
  this->Wave2->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Mark current points as visited and assign region number.  Note:
// traversal occurs across neighboring points.
//
void vtkConnectedPointsFilter::TraverseAndMark(
  vtkPoints* inPts, vtkDataArray* inScalars, float* normals, vtkIdType* labels)
{
  vtkIdType i, j, numPts, numIds, ptId, neiId;
  vtkIdList* tmpWave;
  double x[3];
  float *n, *n2;
  this->NeighborPointIds->Reset();
  vtkIdList* wave = this->Wave;
  vtkIdList* wave2 = this->Wave2;
  bool connectedPoint;

  while ((numIds = wave->GetNumberOfIds()) > 0)
  {
    for (i = 0; i < numIds; i++) // for all points in this wave
    {
      ptId = wave->GetId(i);
      inPts->GetPoint(ptId, x);
      this->Locator->FindPointsWithinRadius(this->Radius, x, this->NeighborPointIds);

      numPts = this->NeighborPointIds->GetNumberOfIds();
      for (j = 0; j < numPts; ++j)
      {
        neiId = this->NeighborPointIds->GetId(j);
        if (labels[neiId] < 0)
        {
          // proximial to the current point
          connectedPoint = true;

          // Does it satisfy scalar connectivity?
          if (inScalars != nullptr)
          {
            double s = inScalars->GetComponent(neiId, 0);
            if (s < this->ScalarRange[0] || s > this->ScalarRange[1])
            {
              connectedPoint = false;
            }
          }

          // Does it satisfy normal connectivity?
          if (normals != nullptr)
          {
            n = normals + 3 * ptId; // in case normals are used
            n2 = normals + 3 * neiId;
            if (vtkMath::Dot(n, n2) < this->NormalThreshold)
            {
              connectedPoint = false;
            }
          }

          // If all criterion are satisfied
          if (connectedPoint)
          {
            labels[neiId] = this->CurrentRegionNumber;
            this->NumPointsInRegion++;
            wave2->InsertNextId(neiId);
          }
        } // if point not yet visited
      }   // for all neighbors
    }     // for all cells in this wave

    tmpWave = wave;
    wave = wave2;
    wave2 = tmpWave;
    tmpWave->Reset();
  } // while wave is not empty
}

//----------------------------------------------------------------------------
// Obtain the number of connected regions.
int vtkConnectedPointsFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes->GetMaxId() + 1;
}

//----------------------------------------------------------------------------
// Initialize list of point ids used to seed regions.
void vtkConnectedPointsFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds->Reset();
}

//----------------------------------------------------------------------------
// Add a seed id (point id). Note: ids are 0-offset.
void vtkConnectedPointsFilter::AddSeed(vtkIdType id)
{
  if (id < 0)
  {
    return;
  }
  this->Modified();
  this->Seeds->InsertNextId(id);
}

//----------------------------------------------------------------------------
// Delete a seed id (point or cell id). Note: ids are 0-offset.
void vtkConnectedPointsFilter::DeleteSeed(vtkIdType id)
{
  this->Modified();
  this->Seeds->DeleteId(id);
}

//----------------------------------------------------------------------------
// Initialize list of region ids to extract.
void vtkConnectedPointsFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds->Reset();
}

//----------------------------------------------------------------------------
// Add a region id to extract. Note: ids are 0-offset.
void vtkConnectedPointsFilter::AddSpecifiedRegion(vtkIdType id)
{
  if (id < 0)
  {
    return;
  }
  this->Modified();
  this->SpecifiedRegionIds->InsertNextId(id);
}

//----------------------------------------------------------------------------
// Delete a region id to extract. Note: ids are 0-offset.
void vtkConnectedPointsFilter::DeleteSpecifiedRegion(vtkIdType id)
{
  this->Modified();
  this->SpecifiedRegionIds->DeleteId(id);
}

//----------------------------------------------------------------------------
int vtkConnectedPointsFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkConnectedPointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";

  os << indent << "Extraction Mode: ";
  os << this->GetExtractionModeAsString() << "\n";

  vtkIdType num;
  os << indent << "Point seeds: ";
  if ((num = this->Seeds->GetNumberOfIds()) > 1)
  {
    os << "(" << num << " seeds specified)\n";
  }
  else
  {
    os << "(no seeds specified)\n";
  }

  os << indent << "Specified regions: ";
  if ((num = this->SpecifiedRegionIds->GetNumberOfIds()) > 1)
  {
    os << "(" << num << " regions specified)\n";
  }
  else
  {
    os << "(no regions specified)\n";
  }

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " << this->ClosestPoint[1]
     << ", " << this->ClosestPoint[2] << ")\n";

  os << indent << "Scalar Connectivity: " << (this->ScalarConnectivity ? "On\n" : "Off\n");
  double* range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  os << indent << "Aligned Normals: " << (this->AlignedNormals ? "On\n" : "Off\n");
  os << indent << "Normal Angle: " << this->NormalAngle << "\n";

  os << indent << "Locator: " << this->Locator << "\n";
}
