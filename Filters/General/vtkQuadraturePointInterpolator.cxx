// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkQuadraturePointInterpolator.h"
#include "vtkQuadratureSchemeDefinition.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridAlgorithm.h"

#include <sstream>
#include <vector>

using std::ostringstream;

#include "vtkQuadraturePointsUtilities.hxx"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQuadraturePointInterpolator);

//------------------------------------------------------------------------------
vtkQuadraturePointInterpolator::vtkQuadraturePointInterpolator()
{
  this->Clear();
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkQuadraturePointInterpolator::~vtkQuadraturePointInterpolator()
{
  this->Clear();
}

//------------------------------------------------------------------------------
int vtkQuadraturePointInterpolator::RequestData(
  vtkInformation*, vtkInformationVector** input, vtkInformationVector* output)
{
  vtkDataObject* tmpDataObj;
  // Get the inputs
  tmpDataObj = input[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkDataSet* datasetIn = vtkDataSet::SafeDownCast(tmpDataObj);
  // Get the outputs
  tmpDataObj = output->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkDataSet* datasetOut = vtkDataSet::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (datasetIn == nullptr || datasetOut == nullptr || datasetIn->GetNumberOfCells() == 0 ||
    datasetIn->GetNumberOfPoints() == 0 || datasetIn->GetPointData() == nullptr ||
    datasetIn->GetPointData()->GetNumberOfArrays() == 0)
  {
    vtkWarningMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
  }

  // Copy the unstructured grid on the input
  datasetOut->ShallowCopy(datasetIn);

  // Interpolate the data arrays, but no points. Results
  // are stored in field data arrays.
  this->InterpolateFields(datasetOut);

  return 1;
}

//------------------------------------------------------------------------------
void vtkQuadraturePointInterpolator::Clear()
{
  // Nothing to do
}

//------------------------------------------------------------------------------
int vtkQuadraturePointInterpolator::InterpolateFields(vtkDataSet* datasetOut)
{
  // Extract info we need for all cells.
  vtkIdType nCells = datasetOut->GetNumberOfCells();

  // For each array we interpolate scalar data to the
  // integration point location. Results are in associated
  // field data arrays.
  int nArrays = datasetOut->GetPointData()->GetNumberOfArrays();

  vtkDataArray* offsets = this->GetInputArrayToProcess(0, datasetOut);
  if (offsets == nullptr)
  {
    vtkWarningMacro("no Offset array, skipping.");
    return 0;
  }

  if (offsets->GetNumberOfComponents() != 1)
  {
    vtkWarningMacro("expected Offset array to be single-component.");
    return 0;
  }

  const char* arrayOffsetName = offsets->GetName();

  vtkInformation* info = offsets->GetInformation();
  vtkInformationQuadratureSchemeDefinitionVectorKey* key =
    vtkQuadratureSchemeDefinition::DICTIONARY();
  if (!key->Has(info))
  {
    vtkWarningMacro("Dictionary is not present in.  Skipping.");
    return 0;
  }
  int dictSize = key->Size(info);
  std::vector<vtkQuadratureSchemeDefinition*> dict(dictSize);
  key->GetRange(info, dict.data(), 0, 0, dictSize);

  // interpolate the arrays
  for (int arrayId = 0; arrayId < nArrays && !this->CheckAbort(); ++arrayId)
  {
    // Grab the next array
    vtkDataArray* V = datasetOut->GetPointData()->GetArray(arrayId);

    // Use two arrays, one with the interpolated values,
    // the other with offsets to the start of each cell's
    // interpolated values.
    int nComps = V->GetNumberOfComponents();
    vtkDoubleArray* interpolated = vtkDoubleArray::New();
    interpolated->SetNumberOfComponents(nComps);
    interpolated->CopyComponentNames(V);
    interpolated->Allocate(nComps * nCells); // at least one qp per cell
    ostringstream interpolatedName;
    interpolatedName << V->GetName(); // << "_QP_Interpolated";
    interpolated->SetName(interpolatedName.str().c_str());
    datasetOut->GetFieldData()->AddArray(interpolated);
    interpolated->GetInformation()->Set(
      vtkQuadratureSchemeDefinition::QUADRATURE_OFFSET_ARRAY_NAME(), arrayOffsetName);
    interpolated->Delete();

    // For all cells interpolate.
    // Don't restrict the value array's type, but only use the fast path for
    // integral offsets.
    using vtkArrayDispatch::AllTypes;
    using vtkArrayDispatch::Integrals;
    using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<AllTypes, Integrals>;

    vtkQuadraturePointsUtilities::InterpolateWorker worker;
    if (!Dispatcher::Execute(V, offsets, worker, datasetOut, nCells, dict, interpolated, this))
    { // fall back to slow path:
      worker(V, offsets, datasetOut, nCells, dict, interpolated, this);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkQuadraturePointInterpolator::InterpolateFields(vtkUnstructuredGrid* usgOut)
{
  vtkDataSet* datasetOut = usgOut;
  return this->InterpolateFields(datasetOut);
}

//------------------------------------------------------------------------------
void vtkQuadraturePointInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "No state." << endl;
}
VTK_ABI_NAMESPACE_END
