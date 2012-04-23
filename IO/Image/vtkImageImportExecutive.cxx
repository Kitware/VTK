/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageImportExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageImportExecutive.h"

#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkImageImport.h"

vtkStandardNewMacro(vtkImageImportExecutive);

//----------------------------------------------------------------------------
int vtkImageImportExecutive::ProcessRequest(vtkInformation* request,
                                            vtkInformationVector** inInfoVec,
                                            vtkInformationVector* outInfoVec)
{
  if(this->Algorithm && request->Has(REQUEST_INFORMATION()))
    {
    // Invoke the callback
    vtkImageImport *ii = vtkImageImport::SafeDownCast(this->Algorithm);
    ii->InvokeUpdateInformationCallbacks();
    }

  return this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec);
}
