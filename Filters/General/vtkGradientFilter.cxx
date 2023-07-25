// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGradientFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCellLinks.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <limits>
#include <vector>

//------------------------------------------------------------------------------

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkGradientFilter);
VTK_ABI_NAMESPACE_END

namespace
{
// special template macro for only float and double types since we will never
// have other data types for output arrays and this helps with reducing template expansion
#define vtkFloatingPointTemplateMacro(call)                                                        \
  vtkTemplateMacroCase(VTK_DOUBLE, double, call);                                                  \
  vtkTemplateMacroCase(VTK_FLOAT, float, call)

// Helper function to compute the vorticity, divergence, and q criterion from
// the gradient.
//------------------------------------------------------------------------------
template <typename V>
void ComputeVorticityFromGradient(double* gradients, V vorticity)
{
  vorticity[0] = gradients[7] - gradients[5];
  vorticity[1] = gradients[2] - gradients[6];
  vorticity[2] = gradients[3] - gradients[1];
}

template <typename D>
void ComputeDivergenceFromGradient(double* gradients, D divergence)
{
  divergence[0] = gradients[0] + gradients[4] + gradients[8];
}

template <typename Q>
void ComputeQCriterionFromGradient(double* gradients, Q qCriterion)
{
  // see http://public.kitware.com/pipermail/paraview/2015-May/034233.html for
  // paper citation and formula on Q-criterion.
  qCriterion[0] =
    -(gradients[0] * gradients[0] + gradients[4] * gradients[4] + gradients[8] * gradients[8]) /
      2. -
    (gradients[1] * gradients[3] + gradients[2] * gradients[6] + gradients[5] * gradients[7]);
}

// Helper function to determine parametric coordinate of a point
int GetCellParametricData(
  vtkIdType pointId, double pointCoord[3], vtkCell* cell, int& subId, double parametricCoord[3]);

// Functions for image data and structured grids
template <class GridT, class DataT>
void ComputeGradientsSG(GridT* output, int* dims, DataT* array, DataT* gradients,
  int numberOfInputComponents, int fieldAssociation, DataT* vorticity, DataT* qCriterion,
  DataT* divergence, vtkUnsignedCharArray* ghosts, unsigned char hiddenGhost,
  vtkGradientFilter* filter);

bool vtkGradientFilterHasArray(vtkFieldData* fieldData, vtkDataArray* array)
{
  int numarrays = fieldData->GetNumberOfArrays();
  for (int i = 0; i < numarrays; i++)
  {
    if (array == fieldData->GetArray(i))
    {
      return true;
    }
  }
  return false;
}

// generic way to get the coordinate for either a cell (using
// the parametric center) or a point
void GetGridEntityCoordinate(
  vtkDataSet* grid, int fieldAssociation, vtkIdType index, double coords[3], vtkGenericCell* cell)
{
  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    grid->GetPoint(index, coords);
  }
  else
  {
    grid->GetCell(index, cell);
    double pcoords[3];
    int subId = cell->GetParametricCenter(pcoords);
    std::vector<double> weights(cell->GetNumberOfPoints() + 1);
    cell->EvaluateLocation(subId, pcoords, coords, weights.data());
  }
}

template <class DataT>
void Fill(vtkDataArray* array, DataT vtkNotUsed(data), int replacementValueOption)
{
  switch (replacementValueOption)
  {
    case vtkGradientFilter::Zero:
      array->Fill(0);
      return;
    case vtkGradientFilter::NaN:
      array->Fill(vtkMath::Nan());
      return;
    case vtkGradientFilter::DataTypeMin:
      array->Fill(std::numeric_limits<DataT>::min());
      return;
    case vtkGradientFilter::DataTypeMax:
      array->Fill(std::numeric_limits<DataT>::max());
      return;
  }
}
template <class DataT>
int GetOutputDataType(DataT vtkNotUsed(data))
{
  if (sizeof(DataT) >= 8)
  {
    return VTK_DOUBLE;
  }
  return VTK_FLOAT;
}

} // end anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkGradientFilter::vtkGradientFilter()
{
  this->ResultArrayName = nullptr;
  this->DivergenceArrayName = nullptr;
  this->VorticityArrayName = nullptr;
  this->QCriterionArrayName = nullptr;
  this->FasterApproximation = 0;
  this->ComputeGradient = 1;
  this->ComputeDivergence = 0;
  this->ComputeVorticity = 0;
  this->ComputeQCriterion = 0;
  this->ContributingCellOption = vtkGradientFilter::All;
  this->ReplacementValueOption = vtkGradientFilter::Zero;
  this->SetInputScalars(
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
vtkGradientFilter::~vtkGradientFilter()
{
  this->SetResultArrayName(nullptr);
  this->SetDivergenceArrayName(nullptr);
  this->SetVorticityArrayName(nullptr);
  this->SetQCriterionArrayName(nullptr);
}

//------------------------------------------------------------------------------
void vtkGradientFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "ResultArrayName:" << (this->ResultArrayName ? this->ResultArrayName : "Gradients") << endl;
  os << indent << "DivergenceArrayName:"
     << (this->DivergenceArrayName ? this->DivergenceArrayName : "Divergence") << endl;
  os << indent
     << "VorticityArrayName:" << (this->VorticityArrayName ? this->VorticityArrayName : "Vorticity")
     << endl;
  os << indent << "QCriterionArrayName:"
     << (this->QCriterionArrayName ? this->QCriterionArrayName : "Q-criterion") << endl;
  os << indent << "FasterApproximation:" << this->FasterApproximation << endl;
  os << indent << "ComputeGradient:" << this->ComputeGradient << endl;
  os << indent << "ComputeDivergence:" << this->ComputeDivergence << endl;
  os << indent << "ComputeVorticity:" << this->ComputeVorticity << endl;
  os << indent << "ComputeQCriterion:" << this->ComputeQCriterion << endl;
  os << indent << "ContributingCellOption:" << this->ContributingCellOption << endl;
  os << indent << "ReplacementValueOption:" << this->ReplacementValueOption << endl;
}

//------------------------------------------------------------------------------
void vtkGradientFilter::SetInputScalars(int fieldAssociation, const char* name)
{
  if ((fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS) &&
    (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS) &&
    (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS))
  {
    vtkErrorMacro("Input Array must be associated with points or cells.");
    return;
  }

  this->SetInputArrayToProcess(0, 0, 0, fieldAssociation, name);
}

//------------------------------------------------------------------------------
void vtkGradientFilter::SetInputScalars(int fieldAssociation, int fieldAttributeType)
{
  if ((fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS) &&
    (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS) &&
    (fieldAssociation != vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS))
  {
    vtkErrorMacro("Input Array must be associated with points or cells.");
    return;
  }

  this->SetInputArrayToProcess(0, 0, 0, fieldAssociation, fieldAttributeType);
}

//------------------------------------------------------------------------------
int vtkGradientFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Technically, this code is only correct for pieces extent types.  However,
  // since this class is pretty inefficient for data types that use 3D extents,
  // we'll punt on the ghost levels for them, too.
  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

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
int vtkGradientFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro("RequestData");

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataArray* array = this->GetInputArrayToProcess(0, inputVector);

  if (input->GetNumberOfCells() == 0)
  {
    // need cells to compute the gradient so if we don't have cells. we can't compute anything.
    // if we have points and an array with values provide a warning letting the user know that
    // no gradient will be computed because of the lack of cells. otherwise the dataset is
    // assumed empty and we can skip providing a warning message to the user.
    if (input->GetNumberOfPoints() && array && array->GetNumberOfTuples())
    {
      vtkWarningMacro("Cannot compute gradient for datasets without cells");
    }
    output->ShallowCopy(input);
    return 1;
  }

  if (array == nullptr)
  {
    vtkErrorMacro("No input array. If this dataset is part of a composite dataset"
      << " check to make sure that all non-empty blocks have this array.");
    return 0;
  }
  if (array->GetNumberOfComponents() == 0)
  {
    vtkErrorMacro("Input array must have at least one component.");
    return 0;
  }

  // we can only compute vorticity and Q criterion if the input
  // array has 3 components. if we can't compute them because of
  // this we only mark internally the we aren't computing them
  // since we don't want to change the state of the filter.
  bool computeVorticity = this->ComputeVorticity != 0;
  bool computeDivergence = this->ComputeDivergence != 0;
  bool computeQCriterion = this->ComputeQCriterion != 0;
  if ((this->ComputeQCriterion || this->ComputeVorticity || this->ComputeDivergence) &&
    array->GetNumberOfComponents() != 3)
  {
    vtkWarningMacro("Input array must have exactly three components with "
      << "ComputeDivergence, ComputeVorticity or ComputeQCriterion flag enabled."
      << "Skipping divergence, vorticity and Q-criterion computation.");
    computeVorticity = false;
    computeQCriterion = false;
    computeDivergence = false;
  }

  int fieldAssociation;
  if (vtkGradientFilterHasArray(input->GetPointData(), array))
  {
    fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  }
  else if (vtkGradientFilterHasArray(input->GetCellData(), array))
  {
    fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
  }
  else
  {
    vtkErrorMacro("Input arrays do not seem to be either point or cell arrays.");
    return 0;
  }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  if (output->IsA("vtkImageData") || output->IsA("vtkStructuredGrid") ||
    output->IsA("vtkRectilinearGrid"))
  {
    vtkUnsignedCharArray* ghosts = nullptr;
    unsigned char hiddenGhost = 0;
    int dims[3];
    if (auto im = vtkImageData::SafeDownCast(output))
    {
      im->GetDimensions(dims);
    }
    else if (auto rect = vtkRectilinearGrid::SafeDownCast(output))
    {
      rect->GetDimensions(dims);
    }
    else if (auto sg = vtkStructuredGrid::SafeDownCast(output))
    {
      sg->GetDimensions(dims);
    }
    if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      ghosts = input->GetPointData()->GetGhostArray();
      hiddenGhost = vtkDataSetAttributes::HIDDENPOINT;
    }
    if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      ghosts = input->GetCellData()->GetGhostArray();
      hiddenGhost = vtkDataSetAttributes::HIDDENCELL;
      dims[0] -= (dims[0] != 1);
      dims[1] -= (dims[1] != 1);
      dims[2] -= (dims[2] != 1);
    }
    this->ComputeRegularGridGradient(array, dims, fieldAssociation, computeVorticity,
      computeQCriterion, computeDivergence, output, ghosts, hiddenGhost);
  }
  else
  {
    this->ComputeUnstructuredGridGradient(array, fieldAssociation, input, computeVorticity,
      computeQCriterion, computeDivergence, output);
  }

  return 1;
}

// Threaded fast path for unstructured grids.
namespace // begin anonymous namespace
{
template <typename TData>
struct GradientsBase
{
  TData* Array;
  int NumComp;
  TData* Gradients;
  TData* Vorticity;
  TData* QCriterion;
  TData* Divergence;
  vtkGradientFilter* Filter;

  GradientsBase(
    TData* a, int numComp, TData* g, TData* v, TData* q, TData* d, vtkGradientFilter* filter)
    : Array(a)
    , NumComp(numComp)
    , Gradients(g)
    , Vorticity(v)
    , QCriterion(q)
    , Divergence(d)
    , Filter(filter)
  {
  }
};

template <typename TData>
struct PointGradients : public GradientsBase<TData>
{
  vtkDataSet* Input;
  vtkStaticCellLinks* Links;
  int HighestDim;
  int CellOption;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<std::vector<double>> Values;
  vtkSMPThreadLocal<std::vector<double>> Gradient;

  PointGradients(vtkDataSet* input, vtkStaticCellLinks* links, TData* a, int numComp, TData* g,
    TData* v, TData* q, TData* d, int highestDim, int cellOption, vtkGradientFilter* filter)
    : GradientsBase<TData>(a, numComp, g, v, q, d, filter)
    , Input(input)
    , Links(links)
    , HighestDim(highestDim)
    , CellOption(cellOption)
  {
  }

  void Initialize()
  {
    this->Cell.Local().TakeReference(vtkGenericCell::New());
    this->Values.Local().resize(8);
    this->Gradient.Local().resize(this->NumComp * 3);
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto& cell = this->Cell.Local();
    auto& values = this->Values.Local();
    auto& g = this->Gradient.Local();
    const auto array = vtk::DataArrayTupleRange(this->Array);
    vtkDataSet* input = this->Input;
    vtkStaticCellLinks* links = this->Links;
    int numberOfOutputComponents = 3 * this->NumComp;

    // if we are doing patches for contributing cell dimensions we want to keep track of
    // the maximum expected dimension so we can exit out of the check loop quicker
    const int maxCellDimension = input->IsA("vtkPolyData") ? 2 : 3;

    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; ptId < endPtId; ptId++)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }

      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      double pointcoords[3];
      input->GetPoint(ptId, pointcoords);
      // Get all cells touching this point.
      vtkIdType numCellNeighbors = links->GetNcells(ptId);
      vtkIdType* cellsOnPoint = links->GetCells(ptId);

      std::fill_n(g.begin(), numberOfOutputComponents, 0.0);

      if (this->CellOption == vtkGradientFilter::Patch)
      {
        this->HighestDim = 0;
        for (vtkIdType neighbor = 0; neighbor < numCellNeighbors; neighbor++)
        {
          const auto cellType =
            static_cast<unsigned char>(input->GetCellType(cellsOnPoint[neighbor]));
          int cellDimension = vtkCellTypes::GetDimension(cellType);
          if (cellDimension > this->HighestDim)
          {
            this->HighestDim = cellDimension;
            if (this->HighestDim == maxCellDimension)
            {
              break;
            }
          }
        }
      }
      vtkIdType numValidCellNeighbors = 0;

      // Iterate on all cells and find all points connected to current point
      // by an edge.
      for (vtkIdType neighbor = 0; neighbor < numCellNeighbors; neighbor++)
      {
        input->GetCell(cellsOnPoint[neighbor], cell);
        if (cell->GetCellDimension() >= this->HighestDim)
        {
          int subId;
          double parametricCoord[3], derivative[3];
          int nPts = cell->GetNumberOfPoints();
          values.resize(nPts);

          if (GetCellParametricData(ptId, pointcoords, cell, subId, parametricCoord))
          {
            numValidCellNeighbors++;
            for (int comp = 0; comp < this->NumComp; comp++)
            {
              // Get values of array at cell points.
              for (int i = 0; i < nPts; i++)
              {
                auto a = array[cell->GetPointId(i)];
                values[i] = a[comp];
              }

              // Get derivative of cell at point.
              cell->Derivatives(subId, parametricCoord, &values[0], 1, derivative);

              g[comp * 3] += derivative[0];
              g[comp * 3 + 1] += derivative[1];
              g[comp * 3 + 2] += derivative[2];
            } // iterating over Components
          }   // if(GetCellParametricData())
        }     // if(cell->GetCellDimension () >= highestCellDimension
      }       // iterating over neighbors

      if (numValidCellNeighbors > 0)
      {
        for (int i = 0; i < numberOfOutputComponents; i++)
        {
          g[i] /= numValidCellNeighbors;
        }

        if (this->Vorticity)
        {
          auto vorticity = vtk::DataArrayTupleRange(this->Vorticity);
          ComputeVorticityFromGradient(&g[0], vorticity[ptId]);
        }
        if (this->QCriterion)
        {
          auto qCriterion = vtk::DataArrayTupleRange(this->QCriterion);
          ComputeQCriterionFromGradient(&g[0], qCriterion[ptId]);
        }
        if (this->Divergence)
        {
          auto divergence = vtk::DataArrayTupleRange(this->Divergence);
          ComputeDivergenceFromGradient(&g[0], divergence[ptId]);
        }
        if (this->Gradients)
        {
          auto gradients = vtk::DataArrayTupleRange(this->Gradients);
          auto gTuple = gradients[ptId];
          for (int i = 0; i < numberOfOutputComponents; i++)
          {
            gTuple[i] = g[i];
          }
        }
      }
    } // iterating over points in grid
  }

  void Reduce() {}
};

// Analyze points to develop smoothing stencil
struct PointGradientsWorker
{
  template <typename DataT>
  void operator()(DataT* array, vtkDataSet* input, vtkStaticCellLinks* links,
    vtkDataArray* gradients, vtkDataArray* vorticity, vtkDataArray* qCriterion,
    vtkDataArray* divergence, int highestDim, int cellOption, vtkGradientFilter* filter)
  {
    vtkIdType numPts = input->GetNumberOfPoints();
    int numComp = array->GetNumberOfComponents();

    PointGradients<DataT> pg(input, links, array, numComp, (DataT*)gradients, (DataT*)vorticity,
      (DataT*)qCriterion, (DataT*)divergence, highestDim, cellOption, filter);

    vtkSMPTools::For(0, numPts, pg);
  }
};

template <typename TData>
struct CellGradients : public GradientsBase<TData>
{
  vtkDataSet* Input;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<std::vector<double>> Values;
  vtkSMPThreadLocal<std::vector<double>> Gradient;

  CellGradients(vtkDataSet* input, TData* a, int numComp, TData* g, TData* v, TData* q, TData* d,
    vtkGradientFilter* filter)
    : GradientsBase<TData>(a, numComp, g, v, q, d, filter)
    , Input(input)
  {
  }

  void Initialize()
  {
    this->Cell.Local().TakeReference(vtkGenericCell::New());
    this->Values.Local().resize(8);
    this->Gradient.Local().resize(this->NumComp * 3);
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    auto& cell = this->Cell.Local();
    auto& values = this->Values.Local();
    auto& cellGrad = this->Gradient.Local();
    const auto array = vtk::DataArrayTupleRange(this->Array);
    vtkDataSet* input = this->Input;
    int subId = 0;
    double cellCenter[3], derivative[3];
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; cellId < endCellId; ++cellId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      input->GetCell(cellId, cell);
      subId = cell->GetParametricCenter(cellCenter);
      vtkIdType nPts = cell->GetNumberOfPoints();
      values.resize(nPts);

      for (int comp = 0; comp < this->NumComp; comp++)
      {
        for (vtkIdType i = 0; i < nPts; i++)
        {
          auto a = array[cell->GetPointId(i)];
          values[i] = a[comp];
        }

        cell->Derivatives(subId, cellCenter, &values[0], 1, derivative);
        cellGrad[comp * 3] = derivative[0];
        cellGrad[comp * 3 + 1] = derivative[1];
        cellGrad[comp * 3 + 2] = derivative[2];
      }

      if (this->Gradients)
      {
        auto gradients = vtk::DataArrayTupleRange(this->Gradients);
        auto g = gradients[cellId];
        for (int i = 0; i < 3 * this->NumComp; i++)
        {
          g[i] = cellGrad[i];
        }
      }
      if (this->Vorticity)
      {
        auto vorticity = vtk::DataArrayTupleRange(this->Vorticity);
        ComputeVorticityFromGradient(&cellGrad[0], vorticity[cellId]);
      }
      if (this->QCriterion)
      {
        auto qCriterion = vtk::DataArrayTupleRange(this->QCriterion);
        ComputeQCriterionFromGradient(&cellGrad[0], qCriterion[cellId]);
      }
      if (this->Divergence)
      {
        auto divergence = vtk::DataArrayTupleRange(this->Divergence);
        ComputeDivergenceFromGradient(&cellGrad[0], divergence[cellId]);
      }

    } // for all cells
  }   // operator()

  void Reduce() {}
};

struct CellGradientsWorker
{
  template <typename DataT>
  void operator()(DataT* array, vtkDataSet* input, vtkDataArray* gradients, vtkDataArray* vorticity,
    vtkDataArray* qCriterion, vtkDataArray* divergence, vtkGradientFilter* filter)
  {
    vtkIdType numCells = input->GetNumberOfCells();
    int numComp = array->GetNumberOfComponents();

    CellGradients<DataT> cg(input, array, numComp, (DataT*)gradients, (DataT*)vorticity,
      (DataT*)qCriterion, (DataT*)divergence, filter);

    vtkSMPTools::For(0, numCells, cg);
  }
};

// For now, serial computation of structured gradients. This dispatch method
// avoids the GetVoidPointer() method.
struct StructuredGradientsWorker
{
  template <class DataT>
  void operator()(DataT* array, vtkDataSet* output, int* dims, vtkDataArray* gradients,
    vtkDataArray* vorticity, vtkDataArray* qCriterion, vtkDataArray* divergence,
    int fieldAssociation, vtkUnsignedCharArray* ghosts, unsigned char hiddenGhost,
    vtkGradientFilter* filter)
  {
    int numComp = array->GetNumberOfComponents();

    if (vtkStructuredGrid* sGrid = vtkStructuredGrid::SafeDownCast(output))
    {
      ComputeGradientsSG(sGrid, dims, array, (DataT*)gradients, numComp, fieldAssociation,
        (DataT*)vorticity, (DataT*)qCriterion, (DataT*)divergence, ghosts, hiddenGhost, filter);
    }
    else if (vtkImageData* image = vtkImageData::SafeDownCast(output))
    {
      ComputeGradientsSG(image, dims, array, (DataT*)gradients, numComp, fieldAssociation,
        (DataT*)vorticity, (DataT*)qCriterion, (DataT*)divergence, ghosts, hiddenGhost, filter);
    }
    else if (vtkRectilinearGrid* rgrid = vtkRectilinearGrid::SafeDownCast(output))
    {
      ComputeGradientsSG(rgrid, dims, array, (DataT*)gradients, numComp, fieldAssociation,
        (DataT*)vorticity, (DataT*)qCriterion, (DataT*)divergence, ghosts, hiddenGhost, filter);
    }
  }
};

} // end anonymous namespace

//------------------------------------------------------------------------------
int vtkGradientFilter::ComputeUnstructuredGridGradient(vtkDataArray* array, int fieldAssociation,
  vtkDataSet* input, bool computeVorticity, bool computeQCriterion, bool computeDivergence,
  vtkDataSet* output)
{
  int arrayType = this->GetOutputArrayType(array);
  int numberOfInputComponents = array->GetNumberOfComponents();
  using vtkArrayDispatch::Reals;

  vtkSmartPointer<vtkDataArray> gradients = nullptr;
  if (this->ComputeGradient)
  {
    gradients.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    gradients->SetNumberOfComponents(3 * numberOfInputComponents);
    gradients->SetNumberOfTuples(array->GetNumberOfTuples());
    switch (arrayType)
    {
      vtkFloatingPointTemplateMacro(
        Fill(gradients, static_cast<VTK_TT>(0), this->ReplacementValueOption));
    }
    if (this->ResultArrayName)
    {
      gradients->SetName(this->ResultArrayName);
    }
    else
    {
      gradients->SetName("Gradients");
    }
  }
  vtkSmartPointer<vtkDataArray> divergence = nullptr;
  if (computeDivergence)
  {
    divergence.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    divergence->SetNumberOfTuples(array->GetNumberOfTuples());
    switch (arrayType)
    {
      vtkFloatingPointTemplateMacro(
        Fill(divergence, static_cast<VTK_TT>(0), this->ReplacementValueOption));
    }
    if (this->DivergenceArrayName)
    {
      divergence->SetName(this->DivergenceArrayName);
    }
    else
    {
      divergence->SetName("Divergence");
    }
  }
  vtkSmartPointer<vtkDataArray> vorticity;
  if (computeVorticity)
  {
    vorticity.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    vorticity->SetNumberOfComponents(3);
    vorticity->SetNumberOfTuples(array->GetNumberOfTuples());
    switch (arrayType)
    {
      vtkFloatingPointTemplateMacro(
        Fill(vorticity, static_cast<VTK_TT>(0), this->ReplacementValueOption));
    }
    if (this->VorticityArrayName)
    {
      vorticity->SetName(this->VorticityArrayName);
    }
    else
    {
      vorticity->SetName("Vorticity");
    }
  }
  vtkSmartPointer<vtkDataArray> qCriterion;
  if (computeQCriterion)
  {
    qCriterion.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    qCriterion->SetNumberOfTuples(array->GetNumberOfTuples());
    switch (arrayType)
    {
      vtkFloatingPointTemplateMacro(
        Fill(qCriterion, static_cast<VTK_TT>(0), this->ReplacementValueOption));
    }
    if (this->QCriterionArrayName)
    {
      qCriterion->SetName(this->QCriterionArrayName);
    }
    else
    {
      qCriterion->SetName("Q-criterion");
    }
  }

  // The common fast path: ContributingCellOption::All, so the cell dimension
  // is inconsequential. This path is threaded, and uses the modern
  // vtkDataArray, tuple interface approach.
  int highestCellDimension = 0;

  // Slower path
  if (this->ContributingCellOption == vtkGradientFilter::DataSetMax)
  {
    int maxDimension = input->IsA("vtkPolyData") == 1 ? 2 : 3;
    for (vtkIdType i = 0; i < input->GetNumberOfCells(); i++)
    {
      const auto cellType = static_cast<unsigned char>(input->GetCellType(i));
      int dim = vtkCellTypes::GetDimension(cellType);
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

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    if (!this->FasterApproximation)
    {
      // Make sure that the topological links have been built, since that is not
      // thread safe.
      vtkNew<vtkStaticCellLinks> cellLinks;
      cellLinks->SetDataSet(input);
      cellLinks->BuildLinks();

      using PointGradientsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
      PointGradientsWorker pgWorker;
      if (!PointGradientsDispatch::Execute(array, pgWorker, input, cellLinks, gradients, vorticity,
            qCriterion, divergence, highestCellDimension, this->ContributingCellOption, this))
      {
        pgWorker(array, input, cellLinks, gradients, vorticity, qCriterion, divergence,
          highestCellDimension, this->ContributingCellOption, this);
      }

      if (gradients)
      {
        output->GetPointData()->AddArray(gradients);
      }
      if (divergence)
      {
        output->GetPointData()->AddArray(divergence);
      }
      if (vorticity)
      {
        output->GetPointData()->AddArray(vorticity);
      }
      if (qCriterion)
      {
        output->GetPointData()->AddArray(qCriterion);
      }
    }
    else // this->FasterApproximation
    {
      // The cell computation is faster and works off of point data anyway.  The
      // faster approximation is to use the cell algorithm and then convert the
      // result to point data.
      vtkSmartPointer<vtkDataArray> cellGradients = nullptr;
      if (gradients)
      {
        cellGradients.TakeReference(vtkDataArray::CreateDataArray(arrayType));
        cellGradients->SetName(gradients->GetName());
        cellGradients->SetNumberOfComponents(3 * array->GetNumberOfComponents());
        cellGradients->SetNumberOfTuples(input->GetNumberOfCells());
      }
      vtkSmartPointer<vtkDataArray> cellDivergence = nullptr;
      if (divergence)
      {
        cellDivergence.TakeReference(vtkDataArray::CreateDataArray(arrayType));
        cellDivergence->SetName(divergence->GetName());
        cellDivergence->SetNumberOfTuples(input->GetNumberOfCells());
      }
      vtkSmartPointer<vtkDataArray> cellVorticity = nullptr;
      if (vorticity)
      {
        cellVorticity.TakeReference(vtkDataArray::CreateDataArray(arrayType));
        cellVorticity->SetName(vorticity->GetName());
        cellVorticity->SetNumberOfComponents(3);
        cellVorticity->SetNumberOfTuples(input->GetNumberOfCells());
      }
      vtkSmartPointer<vtkDataArray> cellQCriterion = nullptr;
      if (qCriterion)
      {
        cellQCriterion.TakeReference(vtkDataArray::CreateDataArray(arrayType));
        cellQCriterion->SetName(qCriterion->GetName());
        cellQCriterion->SetNumberOfTuples(input->GetNumberOfCells());
      }

      // Compute the gradients and any derived information
      using CellGradientsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
      CellGradientsWorker cgWorker;
      if (!CellGradientsDispatch::Execute(array, cgWorker, output, cellGradients, cellVorticity,
            cellQCriterion, cellDivergence, this))
      {
        cgWorker(array, input, cellGradients, cellVorticity, cellQCriterion, cellDivergence, this);
      }

      // We need to convert cell array to points array.
      vtkSmartPointer<vtkDataSet> dummy;
      dummy.TakeReference(input->NewInstance());
      dummy->CopyStructure(input);
      if (cellGradients)
      {
        dummy->GetCellData()->AddArray(cellGradients);
      }
      if (divergence)
      {
        dummy->GetCellData()->AddArray(cellDivergence);
      }
      if (vorticity)
      {
        dummy->GetCellData()->AddArray(cellVorticity);
      }
      if (qCriterion)
      {
        dummy->GetCellData()->AddArray(cellQCriterion);
      }

      vtkNew<vtkCellDataToPointData> cd2pd;
      cd2pd->SetInputData(dummy);
      cd2pd->PassCellDataOff();
      cd2pd->SetContributingCellOption(this->ContributingCellOption);
      cd2pd->SetContainerAlgorithm(this);
      cd2pd->Update();

      // Set the gradients array in the output and cleanup.
      if (gradients)
      {
        output->GetPointData()->AddArray(
          cd2pd->GetOutput()->GetPointData()->GetArray(gradients->GetName()));
      }
      if (qCriterion)
      {
        output->GetPointData()->AddArray(
          cd2pd->GetOutput()->GetPointData()->GetArray(qCriterion->GetName()));
      }
      if (divergence)
      {
        output->GetPointData()->AddArray(
          cd2pd->GetOutput()->GetPointData()->GetArray(divergence->GetName()));
      }
      if (vorticity)
      {
        output->GetPointData()->AddArray(
          cd2pd->GetOutput()->GetPointData()->GetArray(vorticity->GetName()));
      }
    }
  }
  else // fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS
  {
    // We need to convert cell Array to points Array.
    vtkSmartPointer<vtkDataSet> dummy;
    dummy.TakeReference(input->NewInstance());
    dummy->CopyStructure(input);
    dummy->GetCellData()->SetScalars(array);

    vtkNew<vtkCellDataToPointData> cd2pd;
    cd2pd->SetInputData(dummy);
    cd2pd->PassCellDataOff();
    cd2pd->SetContributingCellOption(this->ContributingCellOption);
    cd2pd->SetContainerAlgorithm(this);
    cd2pd->Update();
    vtkDataArray* pointScalars = cd2pd->GetOutput()->GetPointData()->GetScalars();
    pointScalars->Register(this);

    // Compute the cell gradients
    using CellGradientsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    CellGradientsWorker cgWorker;
    if (!CellGradientsDispatch::Execute(
          pointScalars, cgWorker, input, gradients, vorticity, qCriterion, divergence, this))
    {
      cgWorker(pointScalars, input, gradients, vorticity, qCriterion, divergence, this);
    }

    if (gradients)
    {
      output->GetCellData()->AddArray(gradients);
    }
    if (vorticity)
    {
      output->GetCellData()->AddArray(vorticity);
    }
    if (divergence)
    {
      output->GetCellData()->AddArray(divergence);
    }
    if (qCriterion)
    {
      output->GetCellData()->AddArray(qCriterion);
    }
    pointScalars->UnRegister(this);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkGradientFilter::ComputeRegularGridGradient(vtkDataArray* array, int* dims,
  int fieldAssociation, bool computeVorticity, bool computeQCriterion, bool computeDivergence,
  vtkDataSet* output, vtkUnsignedCharArray* ghosts, unsigned char hiddenGhost)
{
  int arrayType = this->GetOutputArrayType(array);
  int numberOfInputComponents = array->GetNumberOfComponents();
  vtkSmartPointer<vtkDataArray> gradients = nullptr;
  if (this->ComputeGradient)
  {
    gradients.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    gradients->SetNumberOfComponents(3 * numberOfInputComponents);
    gradients->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->ResultArrayName)
    {
      gradients->SetName(this->ResultArrayName);
    }
    else
    {
      gradients->SetName("Gradients");
    }
  }
  vtkSmartPointer<vtkDataArray> divergence = nullptr;
  if (computeDivergence)
  {
    divergence.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    divergence->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->DivergenceArrayName)
    {
      divergence->SetName(this->DivergenceArrayName);
    }
    else
    {
      divergence->SetName("Divergence");
    }
  }
  vtkSmartPointer<vtkDataArray> vorticity;
  if (computeVorticity)
  {
    vorticity.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    vorticity->SetNumberOfComponents(3);
    vorticity->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->VorticityArrayName)
    {
      vorticity->SetName(this->VorticityArrayName);
    }
    else
    {
      vorticity->SetName("Vorticity");
    }
  }
  vtkSmartPointer<vtkDataArray> qCriterion;
  if (computeQCriterion)
  {
    qCriterion.TakeReference(vtkDataArray::CreateDataArray(arrayType));
    qCriterion->SetNumberOfTuples(array->GetNumberOfTuples());
    if (this->QCriterionArrayName)
    {
      qCriterion->SetName(this->QCriterionArrayName);
    }
    else
    {
      qCriterion->SetName("Q-criterion");
    }
  }

  // Dispatch by gradient array type
  using StructuredGradientsDispatch =
    vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
  StructuredGradientsWorker sgWorker;
  if (!StructuredGradientsDispatch::Execute(array, sgWorker, output, dims, gradients, vorticity,
        qCriterion, divergence, fieldAssociation, ghosts, hiddenGhost, this))
  {
    sgWorker(array, output, dims, gradients, vorticity, qCriterion, divergence, fieldAssociation,
      ghosts, hiddenGhost, this);
  }

  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    if (gradients)
    {
      output->GetPointData()->AddArray(gradients);
    }
    if (vorticity)
    {
      output->GetPointData()->AddArray(vorticity);
    }
    if (qCriterion)
    {
      output->GetPointData()->AddArray(qCriterion);
    }
    if (divergence)
    {
      output->GetPointData()->AddArray(divergence);
    }
  }
  else if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    if (gradients)
    {
      output->GetCellData()->AddArray(gradients);
    }
    if (vorticity)
    {
      output->GetCellData()->AddArray(vorticity);
    }
    if (qCriterion)
    {
      output->GetCellData()->AddArray(qCriterion);
    }
    if (divergence)
    {
      output->GetCellData()->AddArray(divergence);
    }
  }
  else
  {
    vtkErrorMacro("Bad fieldAssociation value " << fieldAssociation << endl);
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkGradientFilter::GetOutputArrayType(vtkDataArray* array)
{
  int retType = VTK_DOUBLE;
  switch (array->GetDataType())
  {
    vtkTemplateMacro(retType = GetOutputDataType(static_cast<VTK_TT>(0)));
  }
  return retType;
}

VTK_ABI_NAMESPACE_END

namespace // anonymous
{

//------------------------------------------------------------------------------
int GetCellParametricData(
  vtkIdType pointId, double pointCoord[3], vtkCell* cell, int& subId, double parametricCoord[3])
{
  // Watch out for degenerate cells.  They make the derivative calculation
  // fail.
  vtkIdList* pointIds = cell->GetPointIds();
  int timesPointRegistered = 0;
  for (int i = 0; i < pointIds->GetNumberOfIds(); i++)
  {
    if (pointId == pointIds->GetId(i))
    {
      timesPointRegistered++;
    }
  }
  if (timesPointRegistered != 1)
  {
    // The cell should have the point exactly once.  Not good.
    return 0;
  }

  double dummy;
  int numpoints = cell->GetNumberOfPoints();
  std::vector<double> values(numpoints);
  // Get parametric position of point.
  cell->EvaluatePosition(
    pointCoord, nullptr, subId, parametricCoord, dummy, values.data() /*Really another dummy.*/);

  return 1;
}

//------------------------------------------------------------------------------
void HandleDegenerateDimension(double* xp, double* xm, int numComp, std::vector<double>& plusvalues,
  std::vector<double>& minusvalues, double& factor, int dim)
{
  factor = 1.0;
  for (int ii = 0; ii < 3; ii++)
  {
    xp[ii] = xm[ii] = 0.0;
  }
  xp[dim] = 1.0;
  for (int inputComponent = 0; inputComponent < numComp; inputComponent++)
  {
    plusvalues[inputComponent] = minusvalues[inputComponent] = 0;
  }
}

//------------------------------------------------------------------------------
// Threaded computation (on a slice-by-slice basis) of structured gradients.
template <class GridT, class DataT>
struct ComputeStructuredSlice : public GradientsBase<DataT>
{
  GridT* Grid;
  int* Dims;
  int FieldAssociation;
  vtkUnsignedCharArray* Ghosts;
  unsigned char HiddenGhost;
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell; // prevent repeated instantiation

  ComputeStructuredSlice(GridT* output, int* dims, DataT* array, DataT* g, int numComp,
    int fieldAssociation, DataT* v, DataT* q, DataT* d, vtkUnsignedCharArray* ghosts,
    unsigned char hiddenGhost, vtkGradientFilter* filter)
    : GradientsBase<DataT>(array, numComp, g, v, q, d, filter)
    , Grid(output)
    , Dims(dims)
    , FieldAssociation(fieldAssociation)
    , Ghosts(ghosts)
    , HiddenGhost(hiddenGhost)
  {
  }

  void Initialize() { this->Cell.Local().TakeReference(vtkGenericCell::New()); }

  void operator()(vtkIdType k, vtkIdType kEnd)
  {
    GridT* output = this->Grid;
    int* dims = this->Dims;
    int ijsize = dims[0] * dims[1];
    int numComp = this->NumComp;
    int fieldAssociation = this->FieldAssociation;
    const auto array = vtk::DataArrayTupleRange(this->Array);
    auto& cell = this->Cell.Local();

    int idx, idx2, inputComponent;
    double xp[3], xm[3], factor;
    xp[0] = xp[1] = xp[2] = xm[0] = xm[1] = xm[2] = factor = 0;
    double xxi, yxi, zxi, xeta, yeta, zeta, xzeta, yzeta, zzeta;
    yxi = zxi = xeta = yeta = zeta = xzeta = yzeta = zzeta = 0;
    double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;
    xix = xiy = xiz = etax = etay = etaz = zetax = zetay = zetaz = 0;

    // for finite differencing -- the values on the "plus" side and
    // "minus" side of the point to be computed at
    std::vector<double> plusvalues(numComp);
    std::vector<double> minusvalues(numComp);

    std::vector<double> dValuesdXi(numComp);
    std::vector<double> dValuesdEta(numComp);
    std::vector<double> dValuesdZeta(numComp);
    std::vector<double> localGradients(numComp * 3);

    bool isFirst = vtkSMPTools::GetSingleThread();

    // thread over slices
    for (; k < kEnd && !this->Filter->GetAbortOutput(); k++)
    {
      for (int j = 0; j < dims[1] && !this->Filter->GetAbortOutput(); j++)
      {
        for (int i = 0; i < dims[0]; i++)
        {
          if (isFirst)
          {
            this->Filter->CheckAbort();
          }
          if (this->Filter->GetAbortOutput())
          {
            break;
          }
          if (this->Ghosts &&
            (this->Ghosts->GetValue(i + j * dims[0] + k * ijsize) & this->HiddenGhost))
          {
            continue;
          }
          //  Xi derivatives.
          if (dims[0] == 1) // 2D in this direction
          {
            HandleDegenerateDimension(xp, xm, numComp, plusvalues, minusvalues, factor, 0);
          }
          else
          {
            if (i == 0)
            {
              factor = 1.0;
              idx = (i + 1) + j * dims[0] + k * ijsize;
              idx2 = i + j * dims[0] + k * ijsize;
            }
            else if (i == (dims[0] - 1))
            {
              factor = 1.0;
              idx = i + j * dims[0] + k * ijsize;
              idx2 = i - 1 + j * dims[0] + k * ijsize;
            }
            else
            {
              factor = 0.5;
              idx = (i + 1) + j * dims[0] + k * ijsize;
              idx2 = (i - 1) + j * dims[0] + k * ijsize;
            }
            if (this->Ghosts)
            {
              if (this->Ghosts->GetValue(idx2) & this->HiddenGhost)
              {
                ++idx2;
                factor += 0.5;
              }
              if (this->Ghosts->GetValue(idx) & this->HiddenGhost)
              {
                --idx;
                factor += 0.5;
              }
            }
            if (idx == idx2)
            {
              HandleDegenerateDimension(xp, xm, numComp, plusvalues, minusvalues, factor, 0);
            }
            else
            {
              GetGridEntityCoordinate(output, fieldAssociation, idx, xp, cell);
              GetGridEntityCoordinate(output, fieldAssociation, idx2, xm, cell);
              auto a1 = array[idx];
              auto a2 = array[idx2];
              for (inputComponent = 0; inputComponent < numComp; inputComponent++)
              {
                plusvalues[inputComponent] = a1[inputComponent];
                minusvalues[inputComponent] = a2[inputComponent];
              }
            }
          }

          xxi = factor * (xp[0] - xm[0]);
          yxi = factor * (xp[1] - xm[1]);
          zxi = factor * (xp[2] - xm[2]);
          for (inputComponent = 0; inputComponent < numComp; inputComponent++)
          {
            dValuesdXi[inputComponent] =
              factor * (plusvalues[inputComponent] - minusvalues[inputComponent]);
          }

          //  Eta derivatives.
          if (dims[1] == 1) // 2D in this direction
          {
            HandleDegenerateDimension(xp, xm, numComp, plusvalues, minusvalues, factor, 1);
          }
          else
          {
            if (j == 0)
            {
              factor = 1.0;
              idx = i + (j + 1) * dims[0] + k * ijsize;
              idx2 = i + j * dims[0] + k * ijsize;
            }
            else if (j == (dims[1] - 1))
            {
              factor = 1.0;
              idx = i + j * dims[0] + k * ijsize;
              idx2 = i + (j - 1) * dims[0] + k * ijsize;
            }
            else
            {
              factor = 0.5;
              idx = i + (j + 1) * dims[0] + k * ijsize;
              idx2 = i + (j - 1) * dims[0] + k * ijsize;
            }
            if (this->Ghosts)
            {
              if (this->Ghosts->GetValue(idx2) & this->HiddenGhost)
              {
                idx2 += dims[0];
                factor += 0.5;
              }
              if (this->Ghosts->GetValue(idx) & this->HiddenGhost)
              {
                idx -= dims[0];
                factor += 0.5;
              }
            }
            if (idx == idx2)
            {
              HandleDegenerateDimension(xp, xm, numComp, plusvalues, minusvalues, factor, 1);
            }
            else
            {
              GetGridEntityCoordinate(output, fieldAssociation, idx, xp, cell);
              GetGridEntityCoordinate(output, fieldAssociation, idx2, xm, cell);
              auto a1 = array[idx];
              auto a2 = array[idx2];
              for (inputComponent = 0; inputComponent < numComp; inputComponent++)
              {
                plusvalues[inputComponent] = a1[inputComponent];
                minusvalues[inputComponent] = a2[inputComponent];
              }
            }
          }

          xeta = factor * (xp[0] - xm[0]);
          yeta = factor * (xp[1] - xm[1]);
          zeta = factor * (xp[2] - xm[2]);
          for (inputComponent = 0; inputComponent < numComp; inputComponent++)
          {
            dValuesdEta[inputComponent] =
              factor * (plusvalues[inputComponent] - minusvalues[inputComponent]);
          }

          //  Zeta derivatives.
          if (dims[2] == 1) // 2D in this direction
          {
            HandleDegenerateDimension(xp, xm, numComp, plusvalues, minusvalues, factor, 2);
          }
          else
          {
            if (k == 0)
            {
              factor = 1.0;
              idx = i + j * dims[0] + (k + 1) * ijsize;
              idx2 = i + j * dims[0] + k * ijsize;
            }
            else if (k == (dims[2] - 1))
            {
              factor = 1.0;
              idx = i + j * dims[0] + k * ijsize;
              idx2 = i + j * dims[0] + (k - 1) * ijsize;
            }
            else
            {
              factor = 0.5;
              idx = i + j * dims[0] + (k + 1) * ijsize;
              idx2 = i + j * dims[0] + (k - 1) * ijsize;
            }
            if (this->Ghosts)
            {
              if (this->Ghosts->GetValue(idx2) & this->HiddenGhost)
              {
                idx2 += ijsize;
                factor += 0.5;
              }
              if (this->Ghosts->GetValue(idx) & this->HiddenGhost)
              {
                idx -= ijsize;
                factor += 0.5;
              }
            }
            if (idx == idx2)
            {
              HandleDegenerateDimension(xp, xm, numComp, plusvalues, minusvalues, factor, 2);
            }
            else
            {
              GetGridEntityCoordinate(output, fieldAssociation, idx, xp, cell);
              GetGridEntityCoordinate(output, fieldAssociation, idx2, xm, cell);
              auto a1 = array[idx];
              auto a2 = array[idx2];
              for (inputComponent = 0; inputComponent < numComp; inputComponent++)
              {
                plusvalues[inputComponent] = a1[inputComponent];
                minusvalues[inputComponent] = a2[inputComponent];
              }
            }
          }

          xzeta = factor * (xp[0] - xm[0]);
          yzeta = factor * (xp[1] - xm[1]);
          zzeta = factor * (xp[2] - xm[2]);
          for (inputComponent = 0; inputComponent < numComp; inputComponent++)
          {
            dValuesdZeta[inputComponent] =
              factor * (plusvalues[inputComponent] - minusvalues[inputComponent]);
          }

          // Now calculate the Jacobian.  Grids occasionally have
          // singularities, or points where the Jacobian is infinite (the
          // inverse is zero).  For these cases, we'll set the Jacobian to
          // zero, which will result in a zero derivative.
          //
          aj = xxi * yeta * zzeta + yxi * zeta * xzeta + zxi * xeta * yzeta - zxi * yeta * xzeta -
            yxi * xeta * zzeta - xxi * zeta * yzeta;
          if (aj != 0.0)
          {
            aj = 1. / aj;
          }

          //  Xi metrics.
          xix = aj * (yeta * zzeta - zeta * yzeta);
          xiy = -aj * (xeta * zzeta - zeta * xzeta);
          xiz = aj * (xeta * yzeta - yeta * xzeta);

          //  Eta metrics.
          etax = -aj * (yxi * zzeta - zxi * yzeta);
          etay = aj * (xxi * zzeta - zxi * xzeta);
          etaz = -aj * (xxi * yzeta - yxi * xzeta);

          //  Zeta metrics.
          zetax = aj * (yxi * zeta - zxi * yeta);
          zetay = -aj * (xxi * zeta - zxi * xeta);
          zetaz = aj * (xxi * yeta - yxi * xeta);

          // Finally compute the actual derivatives
          idx = i + j * dims[0] + k * ijsize;
          for (inputComponent = 0; inputComponent < numComp; inputComponent++)
          {
            localGradients[inputComponent * 3] = (xix * dValuesdXi[inputComponent] +
              etax * dValuesdEta[inputComponent] + zetax * dValuesdZeta[inputComponent]);

            localGradients[inputComponent * 3 + 1] = (xiy * dValuesdXi[inputComponent] +
              etay * dValuesdEta[inputComponent] + zetay * dValuesdZeta[inputComponent]);

            localGradients[inputComponent * 3 + 2] = (xiz * dValuesdXi[inputComponent] +
              etaz * dValuesdEta[inputComponent] + zetaz * dValuesdZeta[inputComponent]);
          }

          if (this->Gradients)
          {
            auto grad = vtk::DataArrayTupleRange(this->Gradients);
            auto g = grad[idx];
            for (int ii = 0; ii < 3 * numComp; ii++)
            {
              g[ii] = localGradients[ii];
            }
          }
          if (this->Vorticity)
          {
            auto vort = vtk::DataArrayTupleRange(this->Vorticity);
            ComputeVorticityFromGradient(localGradients.data(), vort[idx]);
          }
          if (this->QCriterion)
          {
            auto qCrit = vtk::DataArrayTupleRange(this->QCriterion);
            ComputeQCriterionFromGradient(localGradients.data(), qCrit[idx]);
          }
          if (this->Divergence)
          {
            auto div = vtk::DataArrayTupleRange(this->Divergence);
            ComputeDivergenceFromGradient(localGradients.data(), div[idx]);
          }
        } // i
      }   // j
    }     // k-slice
  }       // operator()

  void Reduce() {}
};

//------------------------------------------------------------------------------
// Process structured dataset types. Thread slice-by-slice.
template <class GridT, class DataT>
void ComputeGradientsSG(GridT* output, int* dims, DataT* array, DataT* gradients, int numComp,
  int fieldAssociation, DataT* vorticity, DataT* qCriterion, DataT* divergence,
  vtkUnsignedCharArray* ghosts, unsigned char hiddenGhost, vtkGradientFilter* filter)
{
  ComputeStructuredSlice<GridT, DataT> structuredSliceWorker(output, dims, array, gradients,
    numComp, fieldAssociation, vorticity, qCriterion, divergence, ghosts, hiddenGhost, filter);

  vtkSMPTools::For(0, dims[2], structuredSliceWorker);
}

} // end anonymous namespace
