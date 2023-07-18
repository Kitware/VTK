/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFDSReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFDSReader.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFDSReader)

  //------------------------------------------------------------------------------
  vtkFDSReader::vtkFDSReader()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkFDSReader::~vtkFDSReader() = default;

//------------------------------------------------------------------------------
void vtkFDSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkFDSReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (this->FileName.empty())
  {
    vtkErrorMacro("Requires valid input file name");
    return 0;
  }

  cout << "Filename : " << this->FileName << endl;
  return 1;
}

VTK_ABI_NAMESPACE_END
