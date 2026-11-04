// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellDataToPointData.h"

#include "vtkAbstractCellLinks.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellTypeUtilities.h"
#include "vtkConstantArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLinks.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <functional>
#include <set>
#include <type_traits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkCellDataToPointData);

namespace
{

//------------------------------------------------------------------------------
// Normalize `weights` so that their sum is 1, return false if their sum is 0.
bool NormalizeWeights(std::vector<double>& weights)
{
  double totalWeight = 0.0;
  for (double weight : weights)
  {
    totalWeight += weight;
  }
  if (totalWeight == 0.0)
  {
    return false;
  }
  for (double& weight : weights)
  {
    weight /= totalWeight;
  }
  return true;
}

//------------------------------------------------------------------------------
// Optimized code for vtkUnstructuredGrid/vtkPolyData. It's waaaay faster than the more
// general path.
template <typename TCellLinks, typename TWeightArray>
struct UnstructuredDataCD2PDFunctor
{
  TCellLinks* Links;
  TWeightArray* WeightArray;
  ArrayList Arrays;

  UnstructuredDataCD2PDFunctor(vtkIdType numPts, vtkCellData* inDA, vtkPointData* outDA,
    TCellLinks* links, TWeightArray* weightArray)
    : Links(links)
    , WeightArray(weightArray)
  {
    this->Arrays.AddArrays(numPts, inDA, outDA);
  }

  void operator()(vtkIdType beginPointId, vtkIdType endPointId)
  {
    auto cellWeights = vtk::DataArrayValueRange<1, double>(this->WeightArray);
    std::vector<double> weights;
    vtkIdType ncells;
    for (vtkIdType pointId = beginPointId; pointId < endPointId; ++pointId)
    {
      if ((ncells = this->Links->GetNcells(pointId)) > 0)
      {
        auto cells = this->Links->GetCells(pointId);
        if constexpr (std::is_base_of_v<vtkConstantArray<double>, TWeightArray>)
        {
          this->Arrays.Average(ncells, cells, pointId);
        }
        else
        {
          weights.resize(ncells);
          for (vtkIdType cellId = 0; cellId < ncells; ++cellId)
          {
            weights[cellId] = cellWeights[cells[cellId]];
          }
          if (NormalizeWeights(weights))
          {
            this->Arrays.WeightedAverage(ncells, cells, weights.data(), pointId);
          }
          else
          {
            // like a point without cells
            this->Arrays.AssignNullValue(pointId);
          }
        }
      }
      else
      {
        this->Arrays.AssignNullValue(pointId);
      }
    }
  }
};

template <typename TCellLinks>
struct UnstructuredDataCD2PDWorker
{
  template <typename TWeightArray>
  void operator()(TWeightArray* weightArray, vtkIdType numPts, vtkCellData* inDA,
    vtkPointData* outDA, TCellLinks* links)
  {
    UnstructuredDataCD2PDFunctor<TCellLinks, TWeightArray> cd2Pd(
      numPts, inDA, outDA, links, weightArray);
    vtkSMPTools::For(0, numPts, cd2Pd);
  }
};

using WeightArrays = vtkTypeList::Create<vtkAOSDataArrayTemplate<float>,
  vtkAOSDataArrayTemplate<double>, vtkConstantArray<float>, vtkConstantArray<double>>;

//------------------------------------------------------------------------------
// Take care of dispatching to the functor using an abstract cell links.
void FastUnstructuredDataACL(vtkIdType numPts, vtkAbstractCellLinks* links, vtkCellData* cfl,
  vtkPointData* pd, vtkDataArray* weightArray)
{
  assert(links != nullptr);
  if (auto staticCellLinks = vtkStaticCellLinks::SafeDownCast(links))
  {
    UnstructuredDataCD2PDWorker<vtkStaticCellLinks> worker;
    if (!vtkArrayDispatch::DispatchByArray<WeightArrays>::Execute(
          weightArray, worker, numPts, cfl, pd, staticCellLinks))
    {
      worker(weightArray, numPts, cfl, pd, staticCellLinks);
    }
  }
  else // vtkCellLinks
  {
    auto cellLinks = vtkCellLinks::SafeDownCast(links);
    UnstructuredDataCD2PDWorker<vtkCellLinks> worker;
    if (!vtkArrayDispatch::DispatchByArray<WeightArrays>::Execute(
          weightArray, worker, numPts, cfl, pd, cellLinks))
    {
      worker(weightArray, numPts, cfl, pd, cellLinks);
    }
  }
}

//------------------------------------------------------------------------------
// Helper template function that implements the major part of the algorithm
// which will be expanded by the vtkTemplateMacro. The template function is
// provided so that coverage test can cover this function. This approach is
// slow: it's non-threaded; uses a slower vtkDataSet API; and most
// unfortunately, accommodates the ContributingCellOption which is not a
// common workflow.
struct Spread
{
  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* const srcArray, DstArrayT* const dstArray,
    vtkDataArray* const weightArray, vtkDataSet* const src, vtkIdType ncells, vtkIdType npoints,
    vtkIdType ncomps, int highestCellDimension, int contributingCellOption,
    vtkCellDataToPointData* filter) const
  {
    // Both arrays will have the same value type:
    using T = vtk::GetAPIType<SrcArrayT>;
    // The weighted sums of integers are accumulated as doubles, the weights being fractional
    using AccumulateT = std::conditional_t<std::is_floating_point_v<T>, T, double>;

    // zero initialization
    std::fill_n(vtk::DataArrayValueRange(dstArray).begin(), npoints * ncomps, T(0));

    const auto srcTuples = vtk::DataArrayTupleRange(srcArray);
    auto dstTuples = vtk::DataArrayTupleRange(dstArray);
    vtkIdType checkAbortInterval;

    // accumulate
    if (contributingCellOption != vtkCellDataToPointData::Patch)
    {
      // point_data += weight * cell_data
      std::vector<AccumulateT> sums(npoints * ncomps, AccumulateT(0));
      std::vector<double> totalWeights(npoints, 0.0);
      vtkNew<vtkIdList> pointIds;
      checkAbortInterval = std::min(ncells / 10 + 1, (vtkIdType)1000);
      for (vtkIdType cid = 0; cid < ncells; ++cid)
      {
        if (cid % checkAbortInterval == 0 && filter->CheckAbort())
        {
          break;
        }
        int dimension = vtkCellTypeUtilities::GetDimension(src->GetCellType(cid));
        if (dimension >= highestCellDimension)
        {
          const double weight = weightArray->GetComponent(cid, 0);
          const auto srcTuple = srcTuples[cid];
          src->GetCellPoints(cid, pointIds);
          for (vtkIdType i = 0, I = pointIds->GetNumberOfIds(); i < I; ++i)
          {
            const vtkIdType ptId = pointIds->GetId(i);
            totalWeights[ptId] += weight;
            for (vtkIdType comp = 0; comp < ncomps; ++comp)
            {
              sums[ptId * ncomps + comp] += static_cast<AccumulateT>(srcTuple[comp] * weight);
            }
          }
        }
      }
      // weighted average

      checkAbortInterval = std::min(npoints / 10 + 1, (vtkIdType)1000);
      for (vtkIdType pid = 0; pid < npoints; ++pid)
      {
        if (pid % checkAbortInterval == 0 && filter->CheckAbort())
        {
          break;
        }
        // guard against divide by zero
        const double totalWeight = totalWeights[pid];
        if (totalWeight != 0.0)
        {
          // point_data = sum / total weight of the cells using the point
          auto dstTuple = dstTuples[pid];
          for (vtkIdType comp = 0; comp < ncomps; ++comp)
          {
            dstTuple[comp] = static_cast<T>(sums[pid * ncomps + comp] / totalWeight);
          }
        }
      }
    }
    else
    { // compute over cell patches
      vtkNew<vtkIdList> cellsOnPoint;
      std::vector<AccumulateT> data(4 * ncomps);
      checkAbortInterval = std::min(npoints / 10 + 1, (vtkIdType)1000);
      for (vtkIdType pid = 0; pid < npoints; ++pid)
      {
        if (pid % checkAbortInterval == 0 && filter->CheckAbort())
        {
          break;
        }
        std::fill(data.begin(), data.end(), AccumulateT(0));
        std::array<double, 4> weightsPointCells = { 0, 0, 0, 0 };
        // Get all cells touching this point.
        src->GetPointCells(pid, cellsOnPoint);
        vtkIdType numPatchCells = cellsOnPoint->GetNumberOfIds();
        for (vtkIdType pc = 0; pc < numPatchCells; pc++)
        {
          vtkIdType cellId = cellsOnPoint->GetId(pc);
          int cellDimension = src->GetCell(cellId)->GetCellDimension();
          const double weight = weightArray->GetComponent(cellId, 0);
          weightsPointCells[cellDimension] += weight;
          const auto srcTuple = srcTuples[cellId];
          for (int comp = 0; comp < ncomps; comp++)
          {
            data[comp + ncomps * cellDimension] +=
              static_cast<AccumulateT>(srcTuple[comp] * weight);
          }
        }
        auto dstTuple = dstTuples[pid];
        for (int dimension = 3; dimension >= 0; dimension--)
        {
          if (weightsPointCells[dimension] != 0)
          {
            for (int comp = 0; comp < ncomps; comp++)
            {
              dstTuple[comp] =
                static_cast<T>(data[comp + dimension * ncomps] / weightsPointCells[dimension]);
            }
            break;
          }
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// A weight array where all the cells have the same weight, for the plain average.
vtkSmartPointer<vtkDataArray> MakeConstantWeights(vtkIdType numberOfCells)
{
  vtkNew<vtkConstantArray<double>> constantWeightArray;
  constantWeightArray->ConstructBackend(1);
  constantWeightArray->SetNumberOfValues(numberOfCells);
  return constantWeightArray;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
// Implementation support
class vtkCellDataToPointData::Internals
{
public:
  std::set<std::string> CellDataArrays;

  // The weights of the cells if EnableWeightedAverage is on, otherwise a constant weight array
  // for a plain average. The weight array must be a cell array with a single component,
  // otherwise a plain average is done, with a warning.
  static vtkSmartPointer<vtkDataArray> GetWeights(
    vtkCellDataToPointData* filter, vtkInformationVector** inputVector, vtkDataSet* input)
  {
    if (!filter->EnableWeightedAverage)
    {
      return MakeConstantWeights(input->GetNumberOfCells());
    }
    int association = vtkDataObject::FIELD_ASSOCIATION_NONE;
    vtkDataArray* weights = filter->GetInputArrayToProcess(0, inputVector, association);
    if (!weights)
    {
      vtkWarningWithObjectMacro(
        filter, "EnableWeightedAverage is on, but no weight array is selected: no weight is used.");
      return MakeConstantWeights(input->GetNumberOfCells());
    }
    // A point array with as many tuples as there are cells is not a weight array either.
    if (association != vtkDataObject::FIELD_ASSOCIATION_CELLS ||
      weights->GetNumberOfComponents() != 1 ||
      weights->GetNumberOfTuples() != input->GetNumberOfCells())
    {
      vtkWarningWithObjectMacro(filter,
        "The weight array " << (weights->GetName() ? weights->GetName() : "(unnamed)")
                            << " must be a cell array with a single component: no weight is used.");
      return MakeConstantWeights(input->GetNumberOfCells());
    }
    return weights;
  }

  // Special traversal algorithm for vtkUniformGrid and vtkRectilinearGrid to support blanking
  // points will not have more than 8 cells for either of these data sets
  template <typename T>
  int InterpolatePointDataWithMask(
    vtkCellDataToPointData* filter, T* input, vtkDataSet* output, vtkDataArray* weightArray)
  {
    vtkNew<vtkIdList> allCellIds;
    allCellIds->Reserve(8);
    vtkNew<vtkIdList> cellIds;
    cellIds->Reserve(8);

    const vtkIdType numberOfPoints = input->GetNumberOfPoints();

    vtkCellData* inCD = input->GetCellData();
    vtkPointData* outPD = output->GetPointData();

    // Copy all existing cell fields into a temporary cell data array,
    // unless the SelectCellDataArrays option is active.
    vtkNew<vtkCellData> processedCellData;
    if (!filter->GetProcessAllArrays())
    {
      for (const auto& name : this->CellDataArrays)
      {
        vtkAbstractArray* array = inCD->GetAbstractArray(name.c_str());
        if (!array)
        {
          vtkWarningWithObjectMacro(filter, "cell data array name not found.");
          continue;
        }
        processedCellData->AddArray(array);
      }
    }
    else
    {
      processedCellData->ShallowCopy(inCD);
    }

    outPD->InterpolateAllocate(processedCellData, numberOfPoints);

    std::vector<double> weights;

    bool abort = false;
    vtkIdType progressInterval = numberOfPoints / 20 + 1;
    for (vtkIdType ptId = 0; ptId < numberOfPoints && !abort; ptId++)
    {
      if (!(ptId % progressInterval))
      {
        filter->UpdateProgress(static_cast<double>(ptId) / numberOfPoints);
        abort = filter->CheckAbort();
      }
      input->GetPointCells(ptId, allCellIds);
      cellIds->Reset();
      // Only consider cells that are not masked:
      for (vtkIdType cId = 0; cId < allCellIds->GetNumberOfIds(); ++cId)
      {
        vtkIdType curCell = allCellIds->GetId(cId);
        if (input->IsCellVisible(curCell))
        {
          cellIds->InsertNextId(curCell);
        }
      }

      vtkIdType numCells = cellIds->GetNumberOfIds();
      weights.resize(numCells);
      for (vtkIdType i = 0; i < numCells; ++i)
      {
        weights[i] = weightArray->GetComponent(cellIds->GetId(i), 0);
      }

      if (numCells > 0 && NormalizeWeights(weights))
      {
        outPD->InterpolatePoint(processedCellData, ptId, cellIds, weights.data());
      }
      else
      {
        outPD->NullData(ptId);
      }
    }

    return 1;
  }
};

//------------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
vtkCellDataToPointData::vtkCellDataToPointData()
{
  this->EnableWeightedAverage = false;
  this->PassCellData = false;
  this->ContributingCellOption = vtkCellDataToPointData::All;
  this->ProcessAllArrays = true;
  this->PieceInvariant = true;
  this->Implementation = new Internals();
}

//------------------------------------------------------------------------------
vtkCellDataToPointData::~vtkCellDataToPointData()
{
  delete this->Implementation;
}

//------------------------------------------------------------------------------
void vtkCellDataToPointData::AddCellDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->CellDataArrays.insert(std::string(name));
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCellDataToPointData::RemoveCellDataArray(const char* name)
{
  if (!name)
  {
    vtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->CellDataArrays.erase(name);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkCellDataToPointData::ClearCellDataArrays()
{
  if (!this->Implementation->CellDataArrays.empty())
  {
    this->Modified();
  }
  this->Implementation->CellDataArrays.clear();
}

//------------------------------------------------------------------------------
vtkIdType vtkCellDataToPointData::GetNumberOfCellArraysToProcess()
{
  return static_cast<vtkIdType>(this->Implementation->CellDataArrays.size());
}

//------------------------------------------------------------------------------
void vtkCellDataToPointData::GetCellArraysToProcess(const char* names[])
{
  for (const auto& n : this->Implementation->CellDataArrays)
  {
    *names = n.c_str();
    ++names;
  }
}

//------------------------------------------------------------------------------
int vtkCellDataToPointData::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  vtkDebugMacro(<< "Mapping cell data to point data");

  // Special traversal algorithm for unstructured data such as vtkPolyData
  // and vtkUnstructuredGrid.
  if (input->IsA("vtkUnstructuredGrid") || input->IsA("vtkPolyData"))
  {
    return this->RequestDataForUnstructuredData(nullptr, inputVector, outputVector);
  }

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();

  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  outPD->PassData(inPD);
  outPD->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

  if (input->GetNumberOfPoints() < 1)
  {
    vtkDebugMacro(<< "No input point data!");
    return 1;
  }

  vtkSmartPointer<vtkDataArray> weightArray = Internals::GetWeights(this, inputVector, input);

  // Do the interpolation, taking care of masked cells if needed.
  vtkStructuredGrid* sGrid = vtkStructuredGrid::SafeDownCast(input);
  vtkUniformGrid* uniformGrid = vtkUniformGrid::SafeDownCast(input);
  int result;
  if (sGrid && sGrid->HasAnyBlankCells())
  {
    result = this->Implementation->InterpolatePointDataWithMask(this, sGrid, output, weightArray);
  }
  else if (uniformGrid && uniformGrid->HasAnyBlankCells())
  {
    result =
      this->Implementation->InterpolatePointDataWithMask(this, uniformGrid, output, weightArray);
  }
  else
  {
    result = this->InterpolatePointData(input, output, weightArray);
  }

  if (result == 0)
  {
    return 0;
  }

  if (!this->PassCellData)
  {
    outCD->CopyAllOff();
    outCD->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  outCD->PassData(inCD);

  return 1;
}

//------------------------------------------------------------------------------
int vtkCellDataToPointData::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->PieceInvariant)
  {
    // I believe the default input update extent
    // is set to the input update extent.
    return 1;
  }

  // Technically, this code is only correct for pieces extent types.  However,
  // since this class is pretty inefficient for data types that use 3D extents,
  // we'll punt on the ghost levels for them, too.

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevels = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    ++ghostLevels;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//------------------------------------------------------------------------------
void vtkCellDataToPointData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableWeightedAverage: " << (this->EnableWeightedAverage ? "On\n" : "Off\n");
  os << indent << "PassCellData: " << (this->PassCellData ? "On\n" : "Off\n");
  os << indent << "ContributingCellOption: " << this->ContributingCellOption << endl;
  os << indent << "PieceInvariant: " << (this->PieceInvariant ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
// In general the method below is quite slow due to ContributingCellOption
// considerations. If the ContributingCellOption is "All", and the dataset
// type is unstructured, then a threaded, tuned approach is used.
int vtkCellDataToPointData::RequestDataForUnstructuredData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkPointSet* input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet* output = vtkPointSet::GetData(outputVector);

  const vtkIdType numberOfCells = input->GetNumberOfCells();
  const vtkIdType numberOfPoints = input->GetNumberOfPoints();
  if (numberOfCells < 1 || numberOfPoints < 1)
  {
    vtkDebugMacro(<< "No input data!");
    return 1;
  }

  // Begin by performing the tasks common to both the slow and fast paths.

  // First, copy the input structure (geometry and topology) to the output as
  // a starting point.
  output->CopyStructure(input);

  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();

  // Pass the point data first. The fields and attributes which also exist in
  // the cell data of the input will be over-written during CopyAllocate
  outPD->CopyGlobalIdsOff();
  outPD->PassData(input->GetPointData());
  outPD->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

  // Copy all existing cell fields into a temporary cell data array,
  // unless the SelectCellDataArrays option is active.
  vtkNew<vtkCellData> processedCellData;
  if (!this->ProcessAllArrays)
  {
    for (const auto& name : this->Implementation->CellDataArrays)
    {
      vtkAbstractArray* array = inCD->GetAbstractArray(name.c_str());
      if (!array)
      {
        vtkWarningMacro("cell data array name not found.");
        continue;
      }
      processedCellData->AddArray(array);
    }
  }
  else
  {
    processedCellData->ShallowCopy(inCD);
  }

  // Remove all fields that are not a data array.
  for (vtkIdType fid = processedCellData->GetNumberOfArrays(); fid--;)
  {
    if (!vtkDataArray::FastDownCast(processedCellData->GetAbstractArray(fid)))
    {
      processedCellData->RemoveArray(fid);
    }
  }

  outPD->InterpolateAllocate(processedCellData, numberOfPoints);

  // Pass the input cell data to the output as appropriate.
  if (!this->PassCellData)
  {
    outCD->CopyAllOff();
    outCD->CopyFieldOn(vtkDataSetAttributes::GhostArrayName());
  }
  outCD->PassData(inCD);

  vtkSmartPointer<vtkDataArray> weightArray = Internals::GetWeights(this, inputVector, input);

  // Now perform the (weighted) averaging operation.

  // Use a much faster approach for the "All" ContributingCellOption, and
  // unstructured datasets. A common workflow requiring maximum performance.
  if (this->ContributingCellOption == vtkCellDataToPointData::All)
  {
    if (auto uGrid = vtkUnstructuredGrid::SafeDownCast(input))
    {
      uGrid->BuildLinks(); // ensure links are built or up to date
      FastUnstructuredDataACL(
        numberOfPoints, uGrid->GetLinks(), processedCellData, outPD, weightArray);
      return 1;
    }
    else // polydata
    {
      auto polyData = vtkPolyData::SafeDownCast(input);
      polyData->BuildLinks(); // ensure links are built or up to date
      FastUnstructuredDataACL(
        numberOfPoints, polyData->GetLinks(), processedCellData, outPD, weightArray);
      return 1;
    }
  } // fast path

  // If necessary, begin the slow, more general path.

  // The cells of the highest dimension of the data set contribute with DataSetMax.
  int highestCellDimension = 0;
  if (this->ContributingCellOption == vtkCellDataToPointData::DataSetMax)
  {
    int maxDimension = input->IsA("vtkPolyData") == 1 ? 2 : 3;
    for (vtkIdType i = 0; i < numberOfCells; i++)
    {
      int dim = vtkCellTypeUtilities::GetDimension(input->GetCellType(i));
      if (dim > highestCellDimension)
      {
        highestCellDimension = dim;
        if (highestCellDimension == maxDimension)
        {
          break;
        }
      }
    }
  }

  const auto nfields = processedCellData->GetNumberOfArrays();
  int fid = 0;
  auto f = [this, &fid, nfields, numberOfPoints, input, numberOfCells, highestCellDimension,
             weightArray](vtkAbstractArray* aa_srcarray, vtkAbstractArray* aa_dstarray)
  {
    // update progress and check for an abort request.
    this->UpdateProgress((fid + 1.0) / nfields);
    ++fid;

    vtkDataArray* const srcarray = vtkDataArray::FastDownCast(aa_srcarray);
    vtkDataArray* const dstarray = vtkDataArray::FastDownCast(aa_dstarray);
    if (srcarray && dstarray)
    {
      dstarray->SetNumberOfTuples(numberOfPoints);
      vtkIdType const ncomps = srcarray->GetNumberOfComponents();

      Spread worker;
      using Dispatcher = vtkArrayDispatch::Dispatch2SameValueType;
      if (!Dispatcher::Execute(srcarray, dstarray, worker, weightArray, input, numberOfCells,
            numberOfPoints, ncomps, highestCellDimension, this->ContributingCellOption, this))
      { // fallback for unknown arrays:
        worker(srcarray, dstarray, weightArray, input, numberOfCells, numberOfPoints, ncomps,
          highestCellDimension, this->ContributingCellOption, this);
      }
    }
  };

  // Cell field list constructed from the filtered cell data array
  vtkDataSetAttributes::FieldList cfl(1);
  cfl.InitializeFieldList(processedCellData);
  if (processedCellData != nullptr && outPD != nullptr)
  {
    cfl.TransformData(0, processedCellData, outPD, f);
  }

  return 1; // slow path
}

//------------------------------------------------------------------------------
int vtkCellDataToPointData::InterpolatePointData(
  vtkDataSet* input, vtkDataSet* output, vtkDataArray* weightArray)
{
  const vtkIdType numberOfPoints = input->GetNumberOfPoints();

  vtkCellData* inCD = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();

  // Copy all existing cell fields into a temporary cell data array,
  // unless the SelectCellDataArrays option is active.
  vtkNew<vtkCellData> processedCellData;
  if (!this->ProcessAllArrays)
  {
    for (const auto& name : this->Implementation->CellDataArrays)
    {
      vtkAbstractArray* array = inCD->GetAbstractArray(name.c_str());
      if (!array)
      {
        vtkWarningMacro("cell data array name not found.");
        continue;
      }
      processedCellData->AddArray(array);
    }
  }
  else
  {
    processedCellData->ShallowCopy(inCD);
  }

  outPD->InterpolateAllocate(processedCellData, numberOfPoints);

  std::vector<double> weights;
  vtkNew<vtkIdList> cellIds;

  bool abort = false;
  vtkIdType progressInterval = numberOfPoints / 20 + 1;
  for (vtkIdType ptId = 0; ptId < numberOfPoints && !abort; ptId++)
  {
    if (!(ptId % progressInterval))
    {
      this->UpdateProgress(static_cast<double>(ptId) / numberOfPoints);
      abort = this->CheckAbort();
    }

    input->GetPointCells(ptId, cellIds);
    vtkIdType numCells = cellIds->GetNumberOfIds();
    weights.resize(numCells);
    for (vtkIdType i = 0; i < numCells; ++i)
    {
      weights[i] = weightArray->GetComponent(cellIds->GetId(i), 0);
    }

    if (numCells > 0 && NormalizeWeights(weights))
    {
      outPD->InterpolatePoint(processedCellData, ptId, cellIds, weights.data());
    }
    else
    {
      outPD->NullData(ptId);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCellDataToPointData::InterpolatePointData(vtkDataSet* input, vtkDataSet* output)
{
  return this->InterpolatePointData(input, output, MakeConstantWeights(input->GetNumberOfCells()));
}
VTK_ABI_NAMESPACE_END
