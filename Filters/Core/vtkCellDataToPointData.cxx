// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellDataToPointData.h"

#include "vtkAbstractCellLinks.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCell.h"
#include "vtkCellData.h"
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
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <functional>
#include <set>

#define VTK_MAX_CELLS_PER_POINT 4096

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkCellDataToPointData);

namespace
{

//------------------------------------------------------------------------------
// Optimized code for vtkUnstructuredGrid/vtkPolyData. It's waaaay faster than the more
// general path.
template <typename TCellLinks>
struct UnstructuredDataCD2PD
{
  TCellLinks* Links;
  ArrayList Arrays;

  UnstructuredDataCD2PD(vtkIdType numPts, vtkCellData* inDA, vtkPointData* outDA, TCellLinks* links)
    : Links(links)
  {
    this->Arrays.AddArrays(numPts, inDA, outDA);
  }

  void operator()(vtkIdType beginPointId, vtkIdType endPointId)
  {
    vtkIdType ncells;
    for (vtkIdType pointId = beginPointId; pointId < endPointId; ++pointId)
    {
      if ((ncells = this->Links->GetNcells(pointId)) > 0)
      {
        auto cells = this->Links->GetCells(pointId);
        this->Arrays.Average(ncells, cells, pointId);
      }
      else
      {
        this->Arrays.AssignNullValue(pointId);
      }
    }
  }
};

//------------------------------------------------------------------------------
// Take care of dispatching to the functor using an abstract cell links.
void FastUnstructuredDataACL(
  vtkIdType numPts, vtkAbstractCellLinks* links, vtkCellData* cfl, vtkPointData* pd)
{
  assert(links != nullptr);
  if (auto staticCellLinks = vtkStaticCellLinks::SafeDownCast(links))
  {
    UnstructuredDataCD2PD<vtkStaticCellLinks> cd2pd(numPts, cfl, pd, staticCellLinks);
    vtkSMPTools::For(0, numPts, cd2pd);
  }
  else // vtkCellLinks
  {
    auto cellLinks = vtkCellLinks::SafeDownCast(links);
    UnstructuredDataCD2PD<vtkCellLinks> cd2pd(numPts, cfl, pd, cellLinks);
    vtkSMPTools::For(0, numPts, cd2pd);
  }
}

//------------------------------------------------------------------------------
// Take care of dispatching to the functor using a static cell links template instance.
template <typename TInput>
void FastUnstructuredDataSCLT(
  vtkIdType connectivitySize, TInput* input, vtkCellData* cfl, vtkPointData* pd)
{
  const auto numberOfPoints = input->GetNumberOfPoints();
  const auto numberOfCells = input->GetNumberOfCells();
  auto linksType =
    vtkAbstractCellLinks::ComputeType(numberOfPoints - 1, numberOfCells - 1, connectivitySize);
  // build the appropriate static cell links template instance
  if (linksType == vtkAbstractCellLinks::STATIC_CELL_LINKS_USHORT)
  {
    using TCellLinks = vtkStaticCellLinksTemplate<unsigned short>;
    TCellLinks cellLinks;
    cellLinks.BuildLinks(input);
    UnstructuredDataCD2PD<TCellLinks> cd2pd(numberOfPoints, cfl, pd, &cellLinks);
    vtkSMPTools::For(0, numberOfPoints, cd2pd);
  }
#ifdef VTK_USE_64BIT_IDS
  else if (linksType == vtkAbstractCellLinks::STATIC_CELL_LINKS_UINT)
  {
    using TCellLinks = vtkStaticCellLinksTemplate<unsigned int>;
    TCellLinks cellLinks;
    cellLinks.BuildLinks(input);
    UnstructuredDataCD2PD<TCellLinks> cd2pd(numberOfPoints, cfl, pd, &cellLinks);
    vtkSMPTools::For(0, numberOfPoints, cd2pd);
  }
#endif
  else
  {
    using TCellLinks = vtkStaticCellLinksTemplate<vtkIdType>;
    TCellLinks cellLinks;
    cellLinks.BuildLinks(input);
    UnstructuredDataCD2PD<TCellLinks> cd2pd(numberOfPoints, cfl, pd, &cellLinks);
    vtkSMPTools::For(0, numberOfPoints, cd2pd);
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
  void operator()(SrcArrayT* const srcarray, DstArrayT* const dstarray, vtkDataSet* const src,
    vtkUnsignedIntArray* const num, vtkIdType ncells, vtkIdType npoints, vtkIdType ncomps,
    int highestCellDimension, int contributingCellOption, vtkCellDataToPointData* filter) const
  {
    // Both arrays will have the same value type:
    using T = vtk::GetAPIType<SrcArrayT>;

    // zero initialization
    std::fill_n(vtk::DataArrayValueRange(dstarray).begin(), npoints * ncomps, T(0));

    const auto srcTuples = vtk::DataArrayTupleRange(srcarray);
    auto dstTuples = vtk::DataArrayTupleRange(dstarray);
    vtkIdType checkAbortInterval;

    // accumulate
    if (contributingCellOption != vtkCellDataToPointData::Patch)
    {
      vtkNew<vtkIdList> pointIds;
      checkAbortInterval = std::min(ncells / 10 + 1, (vtkIdType)1000);
      for (vtkIdType cid = 0; cid < ncells; ++cid)
      {
        if (cid % checkAbortInterval == 0 && filter->CheckAbort())
        {
          break;
        }
        int dimension = vtkCellTypes::GetDimension(src->GetCellType(cid));
        if (dimension >= highestCellDimension)
        {
          const auto srcTuple = srcTuples[cid];
          src->GetCellPoints(cid, pointIds);
          for (vtkIdType i = 0, I = pointIds->GetNumberOfIds(); i < I; ++i)
          {
            const vtkIdType ptId = pointIds->GetId(i);
            auto dstTuple = dstTuples[ptId];
            // accumulate cell data to point data <==> point_data += cell_data
            std::transform(srcTuple.cbegin(), srcTuple.cend(), dstTuple.cbegin(), dstTuple.begin(),
              std::plus<T>());
          }
        }
      }
      // average

      checkAbortInterval = std::min(npoints / 10 + 1, (vtkIdType)1000);
      for (vtkIdType pid = 0; pid < npoints; ++pid)
      {
        if (pid % checkAbortInterval == 0 && filter->CheckAbort())
        {
          break;
        }
        // guard against divide by zero
        if (unsigned int const denom = num->GetValue(pid))
        {
          // divide point data by the number of cells using it <==>
          // point_data /= denum
          auto dstTuple = dstTuples[pid];
          std::transform(dstTuple.cbegin(), dstTuple.cend(), dstTuple.begin(),
            [denom](T value) { return value / denom; });
        }
      }
    }
    else
    { // compute over cell patches
      vtkNew<vtkIdList> cellsOnPoint;
      std::vector<T> data(4 * ncomps);
      checkAbortInterval = std::min(npoints / 10 + 1, (vtkIdType)1000);
      for (vtkIdType pid = 0; pid < npoints; ++pid)
      {
        if (pid % checkAbortInterval == 0 && filter->CheckAbort())
        {
          break;
        }
        std::fill(data.begin(), data.end(), 0);
        T numPointCells[4] = { 0, 0, 0, 0 };
        // Get all cells touching this point.
        src->GetPointCells(pid, cellsOnPoint);
        vtkIdType numPatchCells = cellsOnPoint->GetNumberOfIds();
        for (vtkIdType pc = 0; pc < numPatchCells; pc++)
        {
          vtkIdType cellId = cellsOnPoint->GetId(pc);
          int cellDimension = src->GetCell(cellId)->GetCellDimension();
          numPointCells[cellDimension] += 1;
          const auto srcTuple = srcTuples[cellId];
          for (int comp = 0; comp < ncomps; comp++)
          {
            data[comp + ncomps * cellDimension] += srcTuple[comp];
          }
        }
        auto dstTuple = dstTuples[pid];
        for (int dimension = 3; dimension >= 0; dimension--)
        {
          if (numPointCells[dimension])
          {
            for (int comp = 0; comp < ncomps; comp++)
            {
              dstTuple[comp] = data[comp + dimension * ncomps] / numPointCells[dimension];
            }
            break;
          }
        }
      }
    }
  }
};

} // end anonymous namespace

//----------------------------------------------------------------------------
// Implementation support
class vtkCellDataToPointData::Internals
{
public:
  std::set<std::string> CellDataArrays;

  // Special traversal algorithm for vtkUniformGrid and vtkRectilinearGrid to support blanking
  // points will not have more than 8 cells for either of these data sets
  template <typename T>
  int InterpolatePointDataWithMask(vtkCellDataToPointData* filter, T* input, vtkDataSet* output)
  {
    vtkNew<vtkIdList> allCellIds;
    allCellIds->Allocate(8);
    vtkNew<vtkIdList> cellIds;
    cellIds->Allocate(8);

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

    double weights[8];

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

      if (numCells > 0)
      {
        double weight = 1.0 / numCells;
        for (vtkIdType cellId = 0; cellId < numCells; cellId++)
        {
          weights[cellId] = weight;
        }
        outPD->InterpolatePoint(processedCellData, ptId, cellIds, weights);
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

  // Do the interpolation, taking care of masked cells if needed.
  vtkStructuredGrid* sGrid = vtkStructuredGrid::SafeDownCast(input);
  vtkUniformGrid* uniformGrid = vtkUniformGrid::SafeDownCast(input);
  int result;
  if (sGrid && sGrid->HasAnyBlankCells())
  {
    result = this->Implementation->InterpolatePointDataWithMask(this, sGrid, output);
  }
  else if (uniformGrid && uniformGrid->HasAnyBlankCells())
  {
    result = this->Implementation->InterpolatePointDataWithMask(this, uniformGrid, output);
  }
  else
  {
    result = this->InterpolatePointData(input, output);
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

  // Now perform the averaging operation.

  // Use a much faster approach for the "All" ContributingCellOption, and
  // unstructured datasets. A common workflow requiring maximum performance.
  if (this->ContributingCellOption == vtkCellDataToPointData::All)
  {
    if (auto uGrid = vtkUnstructuredGrid::SafeDownCast(input))
    {
      if (uGrid->GetLinks()) // if links are present use them
      {
        uGrid->BuildLinks(); // ensure links are up to date
        FastUnstructuredDataACL(numberOfPoints, uGrid->GetLinks(), processedCellData, outPD);
      }
      else // otherwise create links with the minimum size
      {
        vtkIdType connectivitySize = uGrid->GetCells()->GetNumberOfConnectivityIds();
        FastUnstructuredDataSCLT(connectivitySize, uGrid, processedCellData, outPD);
      }
      return 1;
    }
    else // polydata
    {
      auto polyData = vtkPolyData::SafeDownCast(input);
      if (polyData->GetLinks()) // if links are present use them
      {
        polyData->BuildLinks(); // ensure links are up to date
        FastUnstructuredDataACL(numberOfPoints, polyData->GetLinks(), processedCellData, outPD);
      }
      else // otherwise create links with the minimum size
      {
        auto verts = polyData->GetVerts();
        auto lines = polyData->GetLines();
        auto polys = polyData->GetPolys();
        auto strips = polyData->GetStrips();
        vtkIdType connectivitySize = 0;
        connectivitySize += verts ? verts->GetNumberOfConnectivityIds() : 0;
        connectivitySize += lines ? lines->GetNumberOfConnectivityIds() : 0;
        connectivitySize += polys ? polys->GetNumberOfConnectivityIds() : 0;
        connectivitySize += strips ? strips->GetNumberOfConnectivityIds() : 0;
        FastUnstructuredDataSCLT(connectivitySize, polyData, processedCellData, outPD);
      }
      return 1;
    }
  } // fast path

  // If necessary, begin the slow, more general path.

  // To a large extent the loops immediately following are a serial version
  // of BuildLinks() found in vtkUnstructuredGrid and vtkPolyData. The code
  // below could be threaded if necessary. Count the number of cells
  // associated with each point. If we are doing patches though we will do
  // that later on.
  vtkSmartPointer<vtkUnsignedIntArray> num;
  int highestCellDimension = 0;
  if (this->ContributingCellOption != vtkCellDataToPointData::Patch)
  {
    num = vtkSmartPointer<vtkUnsignedIntArray>::New();
    num->SetNumberOfValues(numberOfPoints);
    num->FillValue(0);
    if (this->ContributingCellOption == vtkCellDataToPointData::DataSetMax)
    {
      int maxDimension = input->IsA("vtkPolyData") == 1 ? 2 : 3;
      for (vtkIdType i = 0; i < numberOfCells; i++)
      {
        int dim = vtkCellTypes::GetDimension(input->GetCellType(i));
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
    vtkNew<vtkIdList> pids;
    for (vtkIdType cid = 0; cid < numberOfCells; ++cid)
    {
      if (input->GetCell(cid)->GetCellDimension() >= highestCellDimension)
      {
        input->GetCellPoints(cid, pids);
        for (vtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
        {
          vtkIdType const pid = pids->GetId(i);
          num->SetValue(pid, num->GetValue(pid) + 1);
        }
      }
    }
  }

  const auto nfields = processedCellData->GetNumberOfArrays();
  int fid = 0;
  auto f = [this, &fid, nfields, numberOfPoints, input, num, numberOfCells, highestCellDimension](
             vtkAbstractArray* aa_srcarray, vtkAbstractArray* aa_dstarray)
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
      if (!Dispatcher::Execute(srcarray, dstarray, worker, input, num, numberOfCells,
            numberOfPoints, ncomps, highestCellDimension, this->ContributingCellOption, this))
      { // fallback for unknown arrays:
        worker(srcarray, dstarray, input, num, numberOfCells, numberOfPoints, ncomps,
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
int vtkCellDataToPointData::InterpolatePointData(vtkDataSet* input, vtkDataSet* output)
{
  vtkNew<vtkIdList> cellIds;
  cellIds->Allocate(VTK_MAX_CELLS_PER_POINT);

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

  double weights[VTK_MAX_CELLS_PER_POINT];

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

    if (numCells > 0 && numCells < VTK_MAX_CELLS_PER_POINT)
    {
      double weight = 1.0 / numCells;
      for (vtkIdType cellId = 0; cellId < numCells; cellId++)
      {
        weights[cellId] = weight;
      }
      outPD->InterpolatePoint(processedCellData, ptId, cellIds, weights);
    }
    else
    {
      outPD->NullData(ptId);
    }
  }

  return 1;
}
VTK_ABI_NAMESPACE_END
