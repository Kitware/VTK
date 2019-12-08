/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraturePointInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkQuadraturePointInterpolator.h"
#include "vtkQuadratureSchemeDefinition.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridAlgorithm.h"

#include <sstream>
#include <vector>

using std::ostringstream;

#include "vtkQuadraturePointsUtilities.hxx"

vtkStandardNewMacro(vtkQuadraturePointInterpolator);

//-----------------------------------------------------------------------------
vtkQuadraturePointInterpolator::vtkQuadraturePointInterpolator()
{
  this->Clear();
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkQuadraturePointInterpolator::~vtkQuadraturePointInterpolator()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointInterpolator::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
      break;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointInterpolator::FillOutputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
      break;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointInterpolator::RequestData(
  vtkInformation*, vtkInformationVector** input, vtkInformationVector* output)
{
  vtkDataObject* tmpDataObj;
  // Get the inputs
  tmpDataObj = input[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkUnstructuredGrid* usgIn = vtkUnstructuredGrid::SafeDownCast(tmpDataObj);
  // Get the outputs
  tmpDataObj = output->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  vtkUnstructuredGrid* usgOut = vtkUnstructuredGrid::SafeDownCast(tmpDataObj);

  // Quick sanity check.
  if (usgIn == nullptr || usgOut == nullptr || usgIn->GetNumberOfCells() == 0 ||
    usgIn->GetNumberOfPoints() == 0 || usgIn->GetPointData() == nullptr ||
    usgIn->GetPointData()->GetNumberOfArrays() == 0)
  {
    vtkWarningMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
  }

  // Copy the unstructured grid on the input
  usgOut->ShallowCopy(usgIn);

  // Interpolate the data arrays, but no points. Results
  // are stored in field data arrays.
  this->InterpolateFields(usgOut);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointInterpolator::Clear()
{
  // Nothing to do
}

//-----------------------------------------------------------------------------
int vtkQuadraturePointInterpolator::InterpolateFields(vtkUnstructuredGrid* usgOut)
{
  // Extract info we need for all cells.
  vtkIdType nCells = usgOut->GetNumberOfCells();

  // For each array we interpolate scalar data to the
  // integration point location. Results are in associated
  // field data arrays.
  int nArrays = usgOut->GetPointData()->GetNumberOfArrays();

  vtkDataArray* offsets = this->GetInputArrayToProcess(0, usgOut);
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
  for (int arrayId = 0; arrayId < nArrays; ++arrayId)
  {
    // Grab the next array
    vtkDataArray* V = usgOut->GetPointData()->GetArray(arrayId);

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
    usgOut->GetFieldData()->AddArray(interpolated);
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
    if (!Dispatcher::Execute(V, offsets, worker, usgOut, nCells, dict, interpolated))
    { // fall back to slow path:
      worker(V, offsets, usgOut, nCells, dict, interpolated);
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkQuadraturePointInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "No state." << endl;
}
