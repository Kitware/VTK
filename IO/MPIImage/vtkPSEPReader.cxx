// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPSEPReader.h"

#include "vtkCharArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/SystemTools.hxx>

#include <cctype>
#include <cmath>
#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPSEPReader);
vtkCxxSetObjectMacro(vtkPSEPReader, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkPSEPReader::vtkPSEPReader()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkPSEPReader::~vtkPSEPReader()
{
  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
void vtkPSEPReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: ";
  if (this->Controller)
  {
    os << std::endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << std::endl;
  }
}

//------------------------------------------------------------------------------
bool vtkPSEPReader::ReadHeader()
{
  if (!this->Controller)
  {
    vtkErrorMacro(<< "No MPI Controller available.");
    return false;
  }

  vtkNew<vtkCharArray> headerBuffer;
  // Read the header on process 0 and broadcast to everyone else.
  if (this->Controller->GetLocalProcessId() == 0)
  {
    if (!this->ReadHeaderInternal(headerBuffer))
    {
      return false;
    }
  }

  this->Controller->Broadcast(headerBuffer, 0);

  return this->Superclass::ReadHeader(headerBuffer);
}

VTK_ABI_NAMESPACE_END
