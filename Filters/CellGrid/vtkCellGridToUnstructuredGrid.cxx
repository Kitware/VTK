// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridToUnstructuredGrid.h"

#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <new>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridToUnstructuredGrid);
vtkStandardNewMacro(vtkCellGridToUnstructuredGrid::Query);

void vtkCellGridToUnstructuredGrid::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Output: " << this->Output << "\n";
  os << indent << "OutputOffsets: " << this->OutputOffsets.size() << " output cell types\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (const auto& entry : this->OutputOffsets)
  {
    os << i2 << entry.first.Data() << " to cell type " << entry.second.CellType << ".\n";
  }
  os << indent << "AttributeMap: " << this->AttributeMap.size() << " entries\n";
  os << indent << "SubdivisionLevel: " << this->SubdivisionLevel << "\n";
  os << indent << "PointMergeTolerance: " << this->PointMergeTolerance << "\n";
}

bool vtkCellGridToUnstructuredGrid::Query::Initialize()
{
  this->Superclass::Initialize();
  this->OutputOffsets.clear();
  this->AttributeMap.clear();
  this->ConnectivityCount.clear();
  this->ConnectivityWeights.clear();
  this->SampleToPoint->Initialize();
  this->TotalCells = 0;
  this->TotalConnectivityIds = 0;
  this->SampleCoordinates->SetNumberOfComponents(3);
  this->SampleCoordinates->SetNumberOfTuples(0);
  if (!this->Input || !this->Output)
  {
    vtkErrorMacro("Input or output grid is null.");
    return false;
  }

  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> ugcells;
  vtkNew<vtkUnsignedCharArray> ugtypes;
  points->SetDataTypeToDouble();
  this->Output->SetPoints(points);
  this->Output->SetCells(ugtypes, ugcells);

  this->AttributeMap[this->Input->GetShapeAttribute()] = points->GetData();

  for (const auto& inputAtt : this->Input->GetCellAttributeList())
  {
    if (this->Input->GetShapeAttribute() == inputAtt)
    {
      continue;
    }

    vtkNew<vtkDoubleArray> outputArr;
    outputArr->SetName(inputAtt->GetName().Data().c_str());
    outputArr->SetNumberOfComponents(inputAtt->GetNumberOfComponents());
    // Note that we do not allocate memory yet.
    this->Output->GetPointData()->AddArray(outputArr);
    this->AttributeMap[inputAtt] = outputArr;
  }
  return true;
}

void vtkCellGridToUnstructuredGrid::Query::MergeSamplesIntoPoints()
{
  vtkIdType numSamples = this->SampleCoordinates->GetNumberOfTuples();
  auto* points = this->Output->GetPoints();
  this->SampleToPoint->SetNumberOfValues(numSamples);
  if (numSamples == 0)
  {
    points->SetNumberOfPoints(0);
    return;
  }
  vtkIdType* sampleToPoint = this->SampleToPoint->GetPointer(0);

  // Neighboring cells sample the face they share at exactly the same places,
  // since they interpolate the same shape attribute over the same parametric
  // points. Welding those samples together is what makes the output watertight
  // instead of a pile of disconnected cells.
  //
  // The tolerance has to be small compared to the distance between two samples
  // of the same cell, or those sub-cells collapse instead. That distance halves
  // with every level and scales with the model, so the tolerance is relative to
  // the input's diagonal.
  std::array<double, 6> bounds;
  this->Input->GetBounds(bounds.data());
  vtkBoundingBox bbox(bounds.data());
  double diagonal = bbox.GetDiagonalLength();
  double tolerance = diagonal > 0. ? diagonal * this->PointMergeTolerance : 0.;

  vtkNew<vtkPoints> samplePoints;
  samplePoints->SetData(this->SampleCoordinates);
  vtkNew<vtkPolyData> sampleSet;
  sampleSet->SetPoints(samplePoints);

  std::vector<vtkIdType> mergeMap(static_cast<std::size_t>(numSamples));
  vtkNew<vtkStaticPointLocator> locator;
  locator->SetDataSet(sampleSet);
  // Both traversal orders give a deterministic answer, but only BIN_ORDER is
  // threaded: POINT_ORDER walks the points one at a time, on purpose, to keep
  // the lowest-numbered sample of every group as its representative. Which
  // sample represents a group is of no consequence here -- coincident samples
  // hold the same coordinates by construction, and the numbering below does not
  // depend on it -- so take the threaded one.
  locator->SetTraversalOrderToBinOrder();
  locator->BuildLocator();
  locator->MergePoints(tolerance, mergeMap.data());

  // Number the surviving samples in the order they appear, then send every
  // merged sample to whichever point its representative became. Numbering in
  // sample order is what makes the output point IDs reproducible: the merge map
  // itself only says which samples are the same, not what to call them.
  vtkIdType numPoints = 0;
  for (vtkIdType ii = 0; ii < numSamples; ++ii)
  {
    if (mergeMap[ii] == ii)
    {
      sampleToPoint[ii] = numPoints++;
    }
  }
  vtkSMPTools::For(0, numSamples,
    [&mergeMap, sampleToPoint](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        if (mergeMap[ii] != ii)
        {
          sampleToPoint[ii] = sampleToPoint[mergeMap[ii]];
        }
      }
    });

  points->SetNumberOfPoints(numPoints);
  auto* outputCoords = vtkDoubleArray::SafeDownCast(points->GetData());
  const double* sampleCoords = this->SampleCoordinates->GetPointer(0);
  double* pointCoords = outputCoords->GetPointer(0);
  vtkSMPTools::For(0, numSamples,
    [&mergeMap, sampleToPoint, sampleCoords, pointCoords](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        if (mergeMap[ii] == ii)
        {
          std::copy_n(sampleCoords + 3 * ii, 3, pointCoords + 3 * sampleToPoint[ii]);
        }
      }
    });

  // Each sample contributes its cell's value to the point it merged into, so a
  // point interior to one cell keeps that cell's value while a point on a shared
  // face ends up with the average of the cells meeting there. That is how a
  // discontinuous attribute becomes a continuous approximation. Count the
  // contributions here, where the map that decides them is at hand; the
  // point-data pass inverts them into the weights the averaging divides by.
  this->ConnectivityCount = ConnectivityCountType(static_cast<std::size_t>(numPoints));
  vtkSMPTools::For(0, numSamples,
    [this, sampleToPoint](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        this->ConnectivityCount[sampleToPoint[ii]].fetch_add(1, std::memory_order_relaxed);
      }
    });

  // The samples have served their purpose; only the map out of them is still
  // needed, so give the coordinates back before the output arrays are sized.
  this->SampleCoordinates->Initialize();
  this->SampleCoordinates->SetNumberOfComponents(3);
}

void vtkCellGridToUnstructuredGrid::Query::StartPass()
{
  this->Superclass::StartPass();
  switch (this->GetPass())
  {
    case PassType::CountOutputs:
      // Do nothing.
      break;
    case PassType::GenerateSamples:
    {
      vtkIdType totalSamples = 0;
      this->TotalCells = 0;
      this->TotalConnectivityIds = 0;
      for (auto& entry : this->OutputOffsets)
      {
        auto& alloc = entry.second;
        alloc.SampleOffset = totalSamples;
        alloc.CellOffset = this->TotalCells;
        alloc.ConnOffset = this->TotalConnectivityIds;
        totalSamples += alloc.NumberOfSamples;
        this->TotalCells += alloc.NumberOfCells;
        this->TotalConnectivityIds += alloc.NumberOfConnectivityIds;
      }

      // Every sample is a candidate output point. They are held all at once because
      // merging them is a question about the whole set, so this is the high-water
      // mark of the conversion: three doubles per sample of every cell and side.
      this->SampleCoordinates->SetNumberOfComponents(3);
      this->SampleCoordinates->SetNumberOfTuples(totalSamples);
    }
    break;
    case PassType::GenerateConnectivity:
    {
      this->MergeSamplesIntoPoints();

      // Allocate storage for cells.
      auto* cellTypes = vtkUnsignedCharArray::FastDownCast(this->Output->GetCellTypes());
      auto* cellArray = this->Output->GetCells();
      cellTypes->SetNumberOfValues(this->TotalCells);
      this->CellArrayOffsets->SetNumberOfValues(this->TotalCells + 1);
      this->CellArrayConnectivity->SetNumberOfValues(this->TotalConnectivityIds);
      this->CellArrayOffsets->SetValue(this->TotalCells, this->TotalConnectivityIds);
      cellArray->SetData(this->CellArrayOffsets, this->CellArrayConnectivity);
    }
    break;
    case PassType::GeneratePointData:
    {
      auto* pointData = this->Output->GetPointData();
      auto numArrays = pointData->GetNumberOfArrays();
      vtkIdType nn = this->Output->GetPoints()->GetNumberOfPoints();
      for (int ii = 0; ii < numArrays; ++ii)
      {
        auto* array = pointData->GetArray(ii);
        int nc = array->GetNumberOfComponents();
        array->SetNumberOfTuples(nn);
        for (int jj = 0; jj < nc; ++jj)
        {
          array->FillComponent(jj, 0.);
        }
      }
      if (!this->ConnectivityCount.empty())
      {
        // Invert the connectivity counts into weights:
        vtkIdType np = static_cast<vtkIdType>(this->ConnectivityCount.size());
        this->ConnectivityWeights.resize(static_cast<std::size_t>(np));
        vtkSMPTools::For(0, np,
          [this](vtkIdType begin, vtkIdType end)
          {
            for (vtkIdType ii = begin; ii < end; ++ii)
            {
              const int count = this->ConnectivityCount[ii].load(std::memory_order_relaxed);
              this->ConnectivityWeights[ii] = count > 0 ? 1.f / static_cast<float>(count) : 1.f;
            }
          });
        this->ConnectivityCount.clear();
      }
    }
    break;
    default:
      vtkErrorMacro("Unknown pass " << this->GetPass());
      break;
  }
}

vtkDataArray* vtkCellGridToUnstructuredGrid::Query::GetOutputArray(vtkCellAttribute* inputAttribute)
{
  if (!inputAttribute)
  {
    return nullptr;
  }

  auto nit = this->AttributeMap.find(inputAttribute);
  if (nit == this->AttributeMap.end())
  {
    return nullptr;
  }
  return nit->second;
}

bool vtkCellGridToUnstructuredGrid::Query::Finalize()
{
  // Responders write their output by index, so giving up part way through leaves
  // holes in the connectivity rather than a shorter grid. Hand back nothing
  // instead of something that looks like a grid but is not.
  if (this->IsAborted())
  {
    this->Output->Initialize();
  }
  this->SampleToPoint->Initialize();
  this->ConnectivityCount.clear();
  this->ConnectivityWeights.clear();
  return true;
}

void vtkCellGridToUnstructuredGrid::SetSubdivisionLevel(int level)
{
  level = level < 0 ? 0 : level;
  if (this->Request->GetSubdivisionLevel() == level)
  {
    return;
  }
  this->Request->SetSubdivisionLevel(level);
  this->Modified();
}

int vtkCellGridToUnstructuredGrid::GetSubdivisionLevel() const
{
  return this->Request->GetSubdivisionLevel();
}

void vtkCellGridToUnstructuredGrid::SetPointMergeTolerance(double tolerance)
{
  tolerance = tolerance < 0. ? 0. : tolerance;
  if (this->Request->GetPointMergeTolerance() == tolerance)
  {
    return;
  }
  this->Request->SetPointMergeTolerance(tolerance);
  this->Modified();
}

double vtkCellGridToUnstructuredGrid::GetPointMergeTolerance() const
{
  return this->Request->GetPointMergeTolerance();
}

void vtkCellGridToUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SubdivisionLevel: " << this->GetSubdivisionLevel() << "\n";
  os << indent << "PointMergeTolerance: " << this->GetPointMergeTolerance() << "\n";
  os << indent << "Query:\n";
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

int vtkCellGridToUnstructuredGrid::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
    return 1;
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

int vtkCellGridToUnstructuredGrid::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  auto* input = vtkCellGrid::GetData(inInfo[0]);
  auto* output = vtkUnstructuredGrid::GetData(ouInfo);
  if (!input)
  {
    vtkWarningMacro("Empty input.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  output->Initialize();
  this->Request->Input = input;
  this->Request->Output = output;
  this->Request->Algorithm = this;
  // Run the cell-center query on the request.
  //
  // A high subdivision level can ask for more memory than exists. A level of 10
  // turns a single hexahedron into a billion cells, and the allocation happens
  // deep inside the responders, so catch the failure here and report it rather
  // than letting it escape the pipeline.
  bool ok = false;
  try
  {
    ok = input->Query(this->Request);
  }
  catch (const std::bad_alloc&)
  {
    vtkErrorMacro("Ran out of memory converting at subdivision level "
      << this->GetSubdivisionLevel() << ". Try a lower level.");
  }
  this->Request->Algorithm = nullptr;
  if (!ok)
  {
    output->Initialize();
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
