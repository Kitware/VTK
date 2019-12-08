/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTDensityMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOTDensityMap.h"

#include "vtkContourFilter.h"
#include "vtkDataArray.h"
#include "vtkDataArrayCollection.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImagePermute.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTable.h"

#include "vtkOTIncludes.h"
#include "vtkOTUtilities.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

vtkInformationKeyMacro(vtkOTDensityMap, DENSITY, Double);
vtkStandardNewMacro(vtkOTDensityMap);

using namespace OT;

class vtkOTDensityMap::OTDistributionCache
{
public:
  Distribution Cache;
};

class vtkOTDensityMap::OTDensityCache
{
public:
  OTDensityCache(Sample* cache)
    : Cache(cache)
  {
  }

  Sample* Cache;
};

//-----------------------------------------------------------------------------
vtkOTDensityMap::vtkOTDensityMap()
{
  this->SetNumberOfOutputPorts(2);
  this->ContourValues = vtkContourValues::New();
  this->GridSubdivisions = 50;
  this->ContourApproximationNumberOfPoints = 600;
  this->DensityLogPDFSampleCache = new vtkOTDensityMap::OTDensityCache(nullptr);
  this->DensityPDFCache = new vtkOTDensityMap::OTDensityCache(nullptr);
  this->DistributionCache = new vtkOTDensityMap::OTDistributionCache();
}

//-----------------------------------------------------------------------------
vtkOTDensityMap::~vtkOTDensityMap()
{
  this->ContourValues->Delete();
  this->ClearCache();
  delete this->DensityLogPDFSampleCache;
  delete this->DensityPDFCache;
  delete this->DistributionCache;
}

//-----------------------------------------------------------------------------
void vtkOTDensityMap::ClearCache()
{
  if (this->DensityLogPDFSampleCache->Cache != nullptr)
  {
    delete this->DensityLogPDFSampleCache->Cache;
    this->DensityLogPDFSampleCache->Cache = nullptr;
  }
  if (this->DensityPDFCache->Cache != nullptr)
  {
    delete this->DensityPDFCache->Cache;
    this->DensityPDFCache->Cache = nullptr;
  }
  this->DensityLogPDFSampleMTime.Modified();
  this->DensityPDFMTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkOTDensityMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->ContourValues->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GridSubdivisions: " << this->GridSubdivisions << endl;
  os << indent << "ContourApproximationNumberOfPoints: " << this->ContourApproximationNumberOfPoints
     << endl;
}

//-----------------------------------------------------------------------------
int vtkOTDensityMap::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // Input is a table
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkOTDensityMap::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Recover output
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  vtkImageData* imageOutput = vtkImageData::GetData(outputVector, 1);

  // Create Sample from input data array
  vtkDataArray* xArray = this->GetInputArrayToProcess(0, inputVector);
  vtkDataArray* yArray = this->GetInputArrayToProcess(1, inputVector);
  if (xArray == nullptr || yArray == nullptr)
  {
    vtkErrorMacro("Please define numeric arrays to process");
    return 0;
  }
  const char* xArrayName = xArray->GetName();
  const char* yArrayName = yArray->GetName();
  vtkNew<vtkDataArrayCollection> arrays;
  arrays->AddItem(xArray);
  arrays->AddItem(yArray);
  Sample* input = vtkOTUtilities::SingleDimArraysToSample(arrays);

  // Create the PDF Grid
  OT::Indices pointNumber(2, this->GridSubdivisions);
  Point pointMin;
  Point pointMax;
  pointMin = input->getMin();
  pointMax = input->getMax();

  // Check Density PDF cache time
  vtkMTimeType lastBuildTime = this->BuildTime.GetMTime();
  if (this->DensityPDFMTime.GetMTime() > lastBuildTime ||
    inputVector[0]->GetMTime() > lastBuildTime)
  {
    // Clear cache
    this->ClearCache();

    // Compute OpenTURNS PDF
    KernelSmoothing* ks = new KernelSmoothing();
    this->DistributionCache->Cache = ks->build(*input);
    Sample gridX(this->GridSubdivisions * this->GridSubdivisions, 2);
    this->DensityPDFCache->Cache =
      new Sample(this->DistributionCache->Cache.getImplementation()->computePDF(
        pointMin, pointMax, pointNumber, gridX));
    delete ks;
    // this->DensityPDFCache->Cache is now a this->GridSubdivisions*this->GridSubdivisions
    // serialized grid, containing density value for each point of the grid
  }

  // Check Density Log PDF sample cache time
  if (this->DensityLogPDFSampleMTime.GetMTime() > lastBuildTime)
  {
    if (this->DensityLogPDFSampleCache->Cache == nullptr)
    {
      const Sample xSample(
        this->DistributionCache->Cache.getSample(this->ContourApproximationNumberOfPoints));
      this->DensityLogPDFSampleCache->Cache =
        new Sample(this->DistributionCache->Cache.computeLogPDF(xSample));
    }
    else
    {
      // Here we reuse the previous values
      const int oldSize = this->DensityLogPDFSampleCache->Cache->getSize();
      const int newSize = this->ContourApproximationNumberOfPoints;
      // Test if we are asking for more points
      if (newSize > oldSize)
      {
        const Sample xSample(this->DistributionCache->Cache.getSample(newSize - oldSize));
        this->DensityLogPDFSampleCache->Cache->add(
          Sample(this->DistributionCache->Cache.computeLogPDF(xSample)));
      }
      else if (newSize < oldSize)
      {
        // This method keeps only the newSize first elements into the sample and returns the
        // remaining ones
        this->DensityLogPDFSampleCache->Cache->split(newSize);
      }
    }
  }

  // Store the density in a image
  vtkNew<vtkImageData> image;
  image->SetDimensions(this->GridSubdivisions, this->GridSubdivisions, 1);
  image->SetOrigin(pointMin[0], pointMin[1], 0);
  image->SetSpacing((pointMax[0] - pointMin[0]) / this->GridSubdivisions,
    (pointMax[1] - pointMin[1]) / this->GridSubdivisions, 0);

  vtkDataArray* density = vtkOTUtilities::SampleToArray(this->DensityPDFCache->Cache);
  density->SetName("Density");
  image->GetPointData()->SetScalars(density);
  density->Delete();

  // Create contour and set contour values
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(image);
  int numContours = this->ContourValues->GetNumberOfContours();
  contour->SetNumberOfContours(numContours);
  double* contourValues = this->ContourValues->GetValues();
  std::vector<double> densityPDFContourValues(numContours);

  for (int i = 0; i < numContours; i++)
  {
    double val =
      std::exp(this->DensityLogPDFSampleCache->Cache->computeQuantile(1.0 - contourValues[i])[0]);
    contour->SetValue(i, val);
    densityPDFContourValues[i] = val;
  }

  // Compute contour
  contour->Update();
  vtkPolyData* contourPd = contour->GetOutput();

  // A map to temporarily store the output
  std::multimap<double, vtkSmartPointer<vtkTable> > contoursMap;

  // Build contours tables
  this->BuildContours(contourPd, numContours, contourValues, densityPDFContourValues.data(),
    xArrayName, yArrayName, contoursMap);

  // Recover iterator to cache by creating the multiblock tree output
  // Initialize to maximum number of blocks
  output->SetNumberOfBlocks(contoursMap.size());
  int nBlock = 0;
  for (std::multimap<double, vtkSmartPointer<vtkTable> >::iterator it =
         contoursMap.begin(); // Iterate over multimap keys
       it != contoursMap.end(); it = contoursMap.upper_bound(it->first))
  {
    // For each key recover range of tables
    std::pair<std::multimap<double, vtkSmartPointer<vtkTable> >::iterator,
      std::multimap<double, vtkSmartPointer<vtkTable> >::iterator>
      range;
    range = contoursMap.equal_range(it->first);
    vtkNew<vtkMultiBlockDataSet> block;
    block->SetNumberOfBlocks(contoursMap.size());
    int nChildBlock = 0;
    // Put table for the same density in the some block
    for (std::multimap<double, vtkSmartPointer<vtkTable> >::iterator it2 = range.first;
         it2 != range.second; ++it2)
    {
      block->SetBlock(nChildBlock, it2->second);
      block->GetMetaData(nChildBlock)->Set(vtkOTDensityMap::DENSITY(), it2->first);
      nChildBlock++;
    }

    // Store block in output
    block->SetNumberOfBlocks(nChildBlock);
    output->SetBlock(nBlock, block);
    std::ostringstream strs;
    strs << it->first;
    output->GetMetaData(nBlock)->Set(vtkCompositeDataSet::NAME(), strs.str().c_str());
    nBlock++;
  }
  output->SetNumberOfBlocks(nBlock);

  vtkNew<vtkImagePermute> flipImage;
  flipImage->SetInputData(image);
  flipImage->SetFilteredAxes(1, 0, 2);
  flipImage->Update();
  imageOutput->ShallowCopy(flipImage->GetOutput());

  // Store Build Time for cache
  this->BuildTime.Modified();

  delete input;
  return 1;
}

//----------------------------------------------------------------------------
int vtkOTDensityMap::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    return 1;
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
void vtkOTDensityMap::BuildContours(vtkPolyData* contourPd, int numContours,
  const double* contourValues, const double* densityPDFContourValues, const char* xArrayName,
  const char* yArrayName, std::multimap<double, vtkSmartPointer<vtkTable> >& contoursMap)
{
  std::set<vtkIdType> treatedCells;
  vtkNew<vtkIdList> pointIndices;
  vtkPoints* points = contourPd->GetPoints();

  // Try all cells
  for (vtkIdType cellId = 0; cellId < contourPd->GetNumberOfCells(); cellId++)
  {
    // Pick an untreated cell from the contour polydata
    if (treatedCells.find(cellId) != treatedCells.end())
    {
      continue;
    }

    // Create a table containing the X and Y of the point of this contour
    vtkNew<vtkDoubleArray> x;
    vtkNew<vtkDoubleArray> y;
    vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
    table->AddColumn(x);
    table->AddColumn(y);

    // Using neighbor, try to find a cell which is the beginning of the line,
    // or go full circle
    vtkIdType initialCellId = cellId;
    vtkIdType previousCellId = -1;
    vtkIdType nextCellId;
    bool inverted;
    do
    {
      nextCellId = this->FindNextCellId(contourPd, initialCellId, previousCellId, inverted);
      previousCellId = initialCellId;
      initialCellId = nextCellId;
    } while (initialCellId != -1 && initialCellId != cellId);

    // Using this cell, go along the line to fill up the X and Y arrays
    vtkIdType alongCellId = previousCellId;
    vtkIdType pointId = -1;
    previousCellId = -1;
    do
    {
      // Find the next cell and recover current cell point indices
      pointIndices->Reset();
      nextCellId =
        this->FindNextCellId(contourPd, alongCellId, previousCellId, inverted, false, pointIndices);
      vtkIdType nPoints = pointIndices->GetNumberOfIds();

      // If this is the first or final cell, store all points
      // If not, do not store the last point
      bool allPoints = previousCellId == -1 || nextCellId == -1 || alongCellId == initialCellId;
      if (!allPoints)
      {
        nPoints--;
      }
      for (vtkIdType i = 0; i < nPoints; i++)
      {
        double point[3];

        // Some cells may have inverted points
        if (inverted)
        {
          pointId = pointIndices->GetId(pointIndices->GetNumberOfIds() - 1 - i);
        }
        else
        {
          pointId = pointIndices->GetId(i);
        }
        // Store the point in table
        points->GetPoint(pointId, point);
        x->InsertNextTuple1(point[0]);
        y->InsertNextTuple1(point[1]);
      }

      // Add treated cell to set, and go to next cell
      treatedCells.insert(alongCellId);
      previousCellId = alongCellId;
      alongCellId = nextCellId;
    } while (alongCellId != -1 && previousCellId != initialCellId);

    // Recover contour density value using the data at last point
    double densityVal = contourPd->GetPointData()->GetArray(0)->GetTuple1(pointId);
    double contourValue = -1;

    // Recover contour
    for (int i = 0; i < numContours; i++)
    {
      if (densityVal == densityPDFContourValues[i])
      {
        contourValue = contourValues[i];
        break;
      }
    }
    if (contourValue == -1)
    {
      vtkWarningMacro("Cannot find density in inverted values, metadata will be incorrect");
    }

    // Set arrays name
    x->SetName(xArrayName);
    std::stringstream yArrayFullName;
    yArrayFullName << yArrayName << " " << std::setfill(' ') << std::setw(3)
                   << static_cast<int>(contourValue * 100 + 0.5) << "%";
    y->SetName(yArrayFullName.str().c_str());

    // add table to cache
    contoursMap.insert(std::make_pair(contourValue, table));
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkOTDensityMap::FindNextCellId(vtkPolyData* pd, vtkIdType cellId,
  vtkIdType previousCellId, bool& invertedPoints, bool up, vtkIdList* currentCellPoints)
{
  // Initialize
  invertedPoints = false;
  vtkIdList* localCellPoints = currentCellPoints;
  if (localCellPoints == nullptr)
  {
    localCellPoints = vtkIdList::New();
  }

  // Recover current cell points
  pd->GetCellPoints(cellId, localCellPoints);

  vtkNew<vtkIdList> edgePt;
  vtkNew<vtkIdList> edgeCells;
  vtkIdType nCells;
  vtkIdType nextCellId = -1;
  vtkIdType localNextCellId;

  // If up is true
  // First pass we try with the first point
  // Second pass we try with the last point
  // if up is false
  // First pass we try with the last point
  // Second pass we try with the first point
  for (int nPass = 0; nPass < 2; nPass++)
  {
    // Recover a point index at an extremity
    vtkIdType localPtIndex =
      (up ? nPass : (nPass + 1) % 2) * (localCellPoints->GetNumberOfIds() - 1);
    edgePt->InsertNextId(localCellPoints->GetId(localPtIndex));

    // Recover cell neighbors at this extremity
    pd->GetCellNeighbors(cellId, edgePt, edgeCells);
    edgePt->Reset();
    nCells = edgeCells->GetNumberOfIds();

    // If we get a neighbor
    if (nCells >= 1)
    {
      // recover it's id
      localNextCellId = edgeCells->GetId(0);

      // check it is not previous cell
      if (localNextCellId != previousCellId)
      {
        nextCellId = localNextCellId;
        break;
      }
      else
      {
        // First pass did not work, cell is inverted
        invertedPoints = true;
      }
    }
  }
  if (currentCellPoints == nullptr)
  {
    localCellPoints->Delete();
  }
  return nextCellId;
}

//----------------------------------------------------------------------------
void vtkOTDensityMap::SetGridSubdivisions(int gridSubdivisions)
{
  if (this->GridSubdivisions != gridSubdivisions)
  {
    this->GridSubdivisions = gridSubdivisions;
    this->DensityPDFMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOTDensityMap::SetContourApproximationNumberOfPoints(int val)
{
  if (this->ContourApproximationNumberOfPoints != val)
  {
    this->ContourApproximationNumberOfPoints = val;
    this->DensityLogPDFSampleMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOTDensityMap::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

//----------------------------------------------------------------------------
double vtkOTDensityMap::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

//----------------------------------------------------------------------------
double* vtkOTDensityMap::GetValues()
{
  return this->ContourValues->GetValues();
}

//----------------------------------------------------------------------------
void vtkOTDensityMap::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

//----------------------------------------------------------------------------
void vtkOTDensityMap::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

//----------------------------------------------------------------------------
int vtkOTDensityMap::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkOTDensityMap::GetMTime()
{
  return vtkMath::Max(this->Superclass::GetMTime(), this->ContourValues->GetMTime());
}
