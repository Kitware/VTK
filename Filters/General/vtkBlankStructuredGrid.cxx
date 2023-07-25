// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBlankStructuredGrid.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBlankStructuredGrid);

// Construct object to extract all of the input data.
vtkBlankStructuredGrid::vtkBlankStructuredGrid()
{
  this->MinBlankingValue = VTK_FLOAT_MAX;
  this->MaxBlankingValue = VTK_FLOAT_MAX;
  this->ArrayName = nullptr;
  this->ArrayId = -1;
  this->Component = 0;
}

vtkBlankStructuredGrid::~vtkBlankStructuredGrid()
{
  delete[] this->ArrayName;
  this->ArrayName = nullptr;
}

template <class T>
void vtkBlankStructuredGridExecute(vtkBlankStructuredGrid* self, T* dptr, int numPts, int numComp,
  int comp, double min, double max, vtkUnsignedCharArray* ghosts)
{
  T compValue;
  dptr += comp;

  for (int ptId = 0; ptId < numPts; ptId++, dptr += numComp)
  {
    if (self->CheckAbort())
    {
      break;
    }
    compValue = *dptr;
    unsigned char value = 0;
    if (compValue >= min && compValue <= max)
    {
      value |= vtkDataSetAttributes::HIDDENPOINT;
    }
    ghosts->SetValue(ptId, value);
  }
}

int vtkBlankStructuredGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkStructuredGrid* input =
    vtkStructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkStructuredGrid* output =
    vtkStructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData* pd = input->GetPointData();
  vtkCellData* cd = input->GetCellData();
  vtkPointData* outPD = output->GetPointData();
  vtkCellData* outCD = output->GetCellData();
  int numPts = input->GetNumberOfPoints();
  vtkDataArray* dataArray = nullptr;
  int numComp;

  vtkDebugMacro(<< "Blanking Grid");

  // Pass input to output
  //
  output->CopyStructure(input);
  outPD->PassData(pd);
  outCD->PassData(cd);

  // Get the appropriate data array
  //
  if (this->ArrayName != nullptr)
  {
    dataArray = pd->GetArray(this->ArrayName);
  }
  else if (this->ArrayId >= 0)
  {
    dataArray = pd->GetArray(this->ArrayId);
  }

  if (!dataArray || (numComp = dataArray->GetNumberOfComponents()) <= this->Component)
  {
    vtkWarningMacro(<< "Data array not found");
    return 1;
  }
  void* dptr = dataArray->GetVoidPointer(0);

  // Loop over the data array setting anything within the data range specified
  // to be blanked.
  //
  vtkUnsignedCharArray* ghosts = vtkUnsignedCharArray::New();
  ghosts->SetNumberOfValues(numPts);
  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
  switch (dataArray->GetDataType())
  {
    vtkTemplateMacro(vtkBlankStructuredGridExecute(this, static_cast<VTK_TT*>(dptr), numPts,
      numComp, this->Component, this->MinBlankingValue, this->MaxBlankingValue, ghosts));
    default:
      break;
  }
  output->GetPointData()->AddArray(ghosts);
  ghosts->Delete();

  this->CheckAbort();

  return 1;
}

void vtkBlankStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Min Blanking Value: " << this->MinBlankingValue << "\n";
  os << indent << "Max Blanking Value: " << this->MaxBlankingValue << "\n";
  os << indent << "Array Name: ";
  if (this->ArrayName)
  {
    os << this->ArrayName << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Array ID: " << this->ArrayId << "\n";
  os << indent << "Component: " << this->Component << "\n";
}
VTK_ABI_NAMESPACE_END
