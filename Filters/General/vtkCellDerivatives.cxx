// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellDerivatives.h"

#include "vtkArrayDispatch.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellDerivatives);

//----------------------------------------------------------------------------
vtkCellDerivatives::vtkCellDerivatives()
{
  this->VectorMode = VTK_VECTOR_MODE_COMPUTE_GRADIENT;
  this->TensorMode = VTK_TENSOR_MODE_COMPUTE_GRADIENT;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);

  // by default process active point vectors
  this->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

// Support for algorithm dispatch and threading
namespace
{ // anonymous

// Threaded support for computing cell derivatives
template <typename ScalarsT, typename VectorsT>
struct CellDerivatives
{
  vtkDataSet* Input;
  ScalarsT* InScalars;
  int NumComp;
  VectorsT* InVectors;
  vtkDoubleArray* OutGradients;
  vtkDoubleArray* OutVorticity;
  vtkDoubleArray* OutTensors;
  int TensorMode;
  int ComputeScalarDerivs;
  int ComputeVectorDerivs;
  int ComputeVorticity;

  // A convenience to avoid repeated allocations
  vtkSMPThreadLocal<vtkSmartPointer<vtkGenericCell>> Cell;
  vtkSMPThreadLocal<vtkSmartPointer<vtkDoubleArray>> CellScalars;
  vtkSMPThreadLocal<vtkSmartPointer<vtkDoubleArray>> CellVectors;
  vtkCellDerivatives* Filter;

  CellDerivatives(vtkDataSet* input, ScalarsT* s, VectorsT* v, vtkDoubleArray* g,
    vtkDoubleArray* vort, vtkDoubleArray* t, int tMode, int csd, int cvd, int cv,
    vtkCellDerivatives* filter)
    : Input(input)
    , InScalars(s)
    , InVectors(v)
    , OutGradients(g)
    , OutVorticity(vort)
    , OutTensors(t)
    , TensorMode(tMode)
    , ComputeScalarDerivs(csd)
    , ComputeVectorDerivs(cvd)
    , ComputeVorticity(cv)
    , Filter(filter)
  {
    if (this->ComputeScalarDerivs)
    {
      this->NumComp = this->InScalars->GetNumberOfComponents();
    }

    // build cells for polydata so we don't get a race condition
    if (vtkPolyData* pd = vtkPolyData::SafeDownCast(input))
    {
      if (pd->NeedToBuildCells())
      {
        pd->BuildCells();
      }
    }
  }

  void Initialize()
  {
    this->Cell.Local().TakeReference(vtkGenericCell::New());
    this->CellScalars.Local().TakeReference(vtkDoubleArray::New());
    if (this->ComputeScalarDerivs)
    {
      this->CellScalars.Local()->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
      this->CellScalars.Local()->Allocate(this->NumComp * VTK_CELL_SIZE);
    }
    this->CellVectors.Local().TakeReference(vtkDoubleArray::New());
    this->CellVectors.Local()->SetNumberOfComponents(3);
    this->CellVectors.Local()->Allocate(3 * VTK_CELL_SIZE);
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    int subId;
    double pcoords[3], derivs[9], tens[9], w[3], *scalars, *vectors;
    vtkGenericCell* cell = this->Cell.Local();
    vtkDoubleArray* cellScalars = this->CellScalars.Local();
    vtkDoubleArray* cellVectors = this->CellVectors.Local();
    vtkDoubleArray* outGradients = this->OutGradients;
    vtkDoubleArray* outVorticity = this->OutVorticity;
    vtkDoubleArray* outTensors = this->OutTensors;
    ScalarsT* inScalars = this->InScalars;
    VectorsT* inVectors = this->InVectors;
    int computeScalarDerivs = this->ComputeScalarDerivs;
    int computeVectorDerivs = this->ComputeVectorDerivs;
    int computeVorticity = this->ComputeVorticity;
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
      this->Input->GetCell(cellId, cell);
      subId = cell->GetParametricCenter(pcoords);

      if (computeScalarDerivs)
      {
        inScalars->GetTuples(cell->PointIds, cellScalars);
        scalars = cellScalars->GetPointer(0);
        cell->Derivatives(subId, pcoords, scalars, 1, derivs);
        outGradients->SetTuple(cellId, derivs);
      }

      if (computeVectorDerivs || computeVorticity)
      {
        inVectors->GetTuples(cell->PointIds, cellVectors);
        vectors = cellVectors->GetPointer(0);
        cell->Derivatives(0, pcoords, vectors, 3, derivs);

        // Insert appropriate tensor
        if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GRADIENT)
        {
          outTensors->SetTuple(cellId, derivs);
        }
        else if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN)
        {
          tens[0] = 0.5 * (derivs[0] + derivs[0]);
          tens[1] = 0.5 * (derivs[1] + derivs[3]);
          tens[2] = 0.5 * (derivs[2] + derivs[6]);
          tens[3] = 0.5 * (derivs[3] + derivs[1]);
          tens[4] = 0.5 * (derivs[4] + derivs[4]);
          tens[5] = 0.5 * (derivs[5] + derivs[7]);
          tens[6] = 0.5 * (derivs[6] + derivs[2]);
          tens[7] = 0.5 * (derivs[7] + derivs[5]);
          tens[8] = 0.5 * (derivs[8] + derivs[8]);
          outTensors->SetTuple(cellId, tens);
        }
        else if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN)
        {
          tens[0] = 0.5 *
            (derivs[0] + derivs[0] + derivs[0] * derivs[0] + derivs[3] * derivs[3] +
              derivs[6] * derivs[6]);
          tens[1] = 0.5 *
            (derivs[1] + derivs[3] + derivs[0] * derivs[1] + derivs[3] * derivs[4] +
              derivs[6] * derivs[7]);
          tens[2] = 0.5 *
            (derivs[2] + derivs[6] + derivs[0] * derivs[2] + derivs[3] * derivs[5] +
              derivs[6] * derivs[8]);
          tens[3] = 0.5 *
            (derivs[3] + derivs[1] + derivs[1] * derivs[0] + derivs[4] * derivs[3] +
              derivs[7] * derivs[6]);
          tens[4] = 0.5 *
            (derivs[4] + derivs[4] + derivs[1] * derivs[1] + derivs[4] * derivs[4] +
              derivs[7] * derivs[7]);
          tens[5] = 0.5 *
            (derivs[5] + derivs[7] + derivs[1] * derivs[2] + derivs[4] * derivs[5] +
              derivs[7] * derivs[8]);
          tens[6] = 0.5 *
            (derivs[6] + derivs[2] + derivs[2] * derivs[0] + derivs[5] * derivs[3] +
              derivs[8] * derivs[6]);
          tens[7] = 0.5 *
            (derivs[7] + derivs[5] + derivs[2] * derivs[1] + derivs[5] * derivs[4] +
              derivs[8] * derivs[7]);
          tens[8] = 0.5 *
            (derivs[8] + derivs[8] + derivs[2] * derivs[2] + derivs[5] * derivs[5] +
              derivs[8] * derivs[8]);

          outTensors->SetTuple(cellId, tens);
        }
        else if (this->TensorMode == VTK_TENSOR_MODE_PASS_TENSORS)
        {
          // do nothing.
        }

        if (computeVorticity)
        {
          w[0] = derivs[7] - derivs[5];
          w[1] = derivs[2] - derivs[6];
          w[2] = derivs[3] - derivs[1];
          outVorticity->SetTuple(cellId, w);
        }
      }
    } // for all cells
  }

  void Reduce() {}
};

struct CellDerivativesWorker
{
  template <typename ScalarsT, typename VectorsT>
  void operator()(ScalarsT* s, VectorsT* v, vtkDataSet* input, vtkIdType numCells,
    vtkDoubleArray* g, vtkDoubleArray* vort, vtkDoubleArray* t, int tMode, int csd, int cvd, int cv,
    vtkCellDerivatives* filter)
  {
    CellDerivatives<ScalarsT, VectorsT> cd(input, s, v, g, vort, t, tMode, csd, cvd, cv, filter);
    vtkSMPTools::For(0, numCells, cd);
  }
};

} // end anonymous

//----------------------------------------------------------------------------
int vtkCellDerivatives::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  vtkDataArray* inVectors = this->GetInputArrayToProcess(1, inputVector);
  vtkSmartPointer<vtkDoubleArray> outGradients;
  vtkSmartPointer<vtkDoubleArray> outVorticity;
  vtkSmartPointer<vtkDoubleArray> outTensors;
  vtkIdType numCells = input->GetNumberOfCells();
  int computeScalarDerivs = 1, computeVectorDerivs = 1, computeVorticity = 1;

  // Initialize
  vtkDebugMacro(<< "Computing cell derivatives");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Check input
  if (numCells < 1)
  {
    vtkErrorMacro("No cells to generate derivatives from");
    return 1;
  }

  // Figure out what to compute
  if (inScalars && this->VectorMode == VTK_VECTOR_MODE_COMPUTE_GRADIENT)
  {
    outGradients = vtkSmartPointer<vtkDoubleArray>::New();
    outGradients->SetNumberOfComponents(3);
    outGradients->SetNumberOfTuples(numCells);
    outGradients->SetName("ScalarGradient");
  }
  else
  {
    computeScalarDerivs = 0;
  }

  if (inVectors && this->VectorMode == VTK_VECTOR_MODE_COMPUTE_VORTICITY)
  {
    outVorticity = vtkSmartPointer<vtkDoubleArray>::New();
    outVorticity->SetNumberOfComponents(3);
    outVorticity->SetNumberOfTuples(numCells);
    outVorticity->SetName("Vorticity");
  }
  else
  {
    computeVorticity = 0;
  }

  if (inVectors &&
    (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GRADIENT ||
      this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN ||
      this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN))
  {
    outTensors = vtkSmartPointer<vtkDoubleArray>::New();
    outTensors->SetNumberOfComponents(9);
    outTensors->SetNumberOfTuples(numCells);
    if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN)
    {
      outTensors->SetName("Strain");
    }
    else if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN)
    {
      outTensors->SetName("GreenLagrangeStrain");
    }
    else
    {
      outTensors->SetName("VectorGradient");
    }
  }
  else
  {
    computeVectorDerivs = 0;
  }

  // If just passing data forget the loop
  if (computeScalarDerivs || computeVectorDerivs || computeVorticity)
  {
    // Threaded loop over all cells computing derivatives
    using CellDerivativesDispatch =
      vtkArrayDispatch::Dispatch2ByValueType<vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
    CellDerivativesWorker cdWorker;
    if (!CellDerivativesDispatch::Execute(inScalars, inVectors, cdWorker, input, numCells,
          outGradients, outVorticity, outTensors, this->TensorMode, computeScalarDerivs,
          computeVectorDerivs, computeVorticity, this))
    {
      cdWorker(inScalars, inVectors, input, numCells, outGradients, outVorticity, outTensors,
        this->TensorMode, computeScalarDerivs, computeVectorDerivs, computeVorticity, this);
    }

  } // if something to compute

  // Pass appropriate data through to output
  outPD->PassData(pd);
  outCD->PassData(cd);
  if (outGradients)
  {
    outCD->SetVectors(outGradients);
  }
  if (outVorticity)
  {
    outCD->SetVectors(outVorticity);
  }
  if (outTensors)
  {
    outCD->SetTensors(outTensors);
  }

  return 1;
}

//----------------------------------------------------------------------------
const char* vtkCellDerivatives::GetVectorModeAsString()
{
  if (this->VectorMode == VTK_VECTOR_MODE_PASS_VECTORS)
  {
    return "PassVectors";
  }
  else if (this->VectorMode == VTK_VECTOR_MODE_COMPUTE_GRADIENT)
  {
    return "ComputeGradient";
  }
  else // VTK_VECTOR_MODE_COMPUTE_VORTICITY
  {
    return "ComputeVorticity";
  }
}

//----------------------------------------------------------------------------
const char* vtkCellDerivatives::GetTensorModeAsString()
{
  if (this->TensorMode == VTK_TENSOR_MODE_PASS_TENSORS)
  {
    return "PassTensors";
  }
  else if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GRADIENT)
  {
    return "ComputeGradient";
  }
  else if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN)
  {
    return "ComputeStrain";
  }
  else // VTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN
  {
    return "ComputeGreenLagrangeStrain";
  }
}

//----------------------------------------------------------------------------
void vtkCellDerivatives::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Vector Mode: " << this->GetVectorModeAsString() << endl;

  os << indent << "Tensor Mode: " << this->GetTensorModeAsString() << endl;
}
VTK_ABI_NAMESPACE_END
