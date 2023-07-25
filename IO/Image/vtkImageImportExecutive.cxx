// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageImportExecutive.h"

#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkImageImport.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageImportExecutive);

//------------------------------------------------------------------------------
void vtkImageImportExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkTypeBool vtkImageImportExecutive::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inInfoVec, vtkInformationVector* outInfoVec)
{
  if (this->Algorithm && request->Has(REQUEST_INFORMATION()))
  {
    // Invoke the callback
    vtkImageImport* ii = vtkImageImport::SafeDownCast(this->Algorithm);
    ii->InvokeUpdateInformationCallbacks();
  }

  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}
VTK_ABI_NAMESPACE_END
