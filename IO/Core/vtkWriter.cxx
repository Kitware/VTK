/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWriter.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include <sstream>


// Construct with no start and end write methods or arguments.
vtkWriter::vtkWriter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

vtkWriter::~vtkWriter()
{
}

void vtkWriter::SetInputData(vtkDataObject *input)
{
  this->SetInputData(0, input);
}

void vtkWriter::SetInputData(int index, vtkDataObject *input)
{
  this->SetInputDataInternal(index, input);
}

vtkDataObject *vtkWriter::GetInput()
{
  return this->GetInput(0);
}

vtkDataObject *vtkWriter::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return NULL;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}


// Write data to output. Method executes subclasses WriteData() method, as
// well as StartMethod() and EndMethod() methods.
int vtkWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    vtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();
  this->UpdateWholeExtent();

  return (this->GetErrorCode() == vtkErrorCode::NoError);
}

int vtkWriter::ProcessRequest(vtkInformation *request,
                              vtkInformationVector **inputVector,
                              vtkInformationVector *outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkWriter::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *)
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkDataObject *input = this->GetInput();
  int idx;

  // make sure input is available
  if ( !input )
  {
    vtkErrorMacro(<< "No input!");
    return 0;
  }

  for (idx = 0; idx < this->GetNumberOfInputPorts(); ++idx)
  {
    if (this->GetInputExecutive(idx, 0) != NULL)
    {
      this->GetInputExecutive(idx, 0)->Update();
    }
  }

  vtkMTimeType lastUpdateTime =  this->GetInput(0)->GetUpdateTime();
  for (idx = 1; idx < this->GetNumberOfInputPorts(); ++idx)
  {
    if (this->GetInput(idx))
    {
      vtkMTimeType updateTime = this->GetInput(idx)->GetUpdateTime();
      if ( updateTime > lastUpdateTime )
      {
        lastUpdateTime = updateTime;
      }
    }
  }

  if (lastUpdateTime < this->WriteTime && this->GetMTime() < this->WriteTime)
  {
    // we are up to date
    return 1;
  }

  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  this->WriteData();
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  this->WriteTime.Modified();

  return 1;
}

void vtkWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

void vtkWriter::EncodeString(char* resname, const char* name, bool doublePercent)
{
  if ( !name || !resname )
  {
    return;
  }
  int cc = 0;
  std::ostringstream str;

  char buffer[10];

  while( name[cc] )
  {
    // Encode spaces and %'s (and most non-printable ascii characters)
    // The reader does not support spaces in strings.
    if ( name[cc] < 33  || name[cc] > 126 ||
         name[cc] == '\"' || name[cc] == '%' )
    {
      snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned char>(name[cc]));
      if (doublePercent)
      {
        str << "%%";
      }
      else
      {
        str << "%";
      }
      str << buffer;
    }
    else
    {
      str << name[cc];
    }
    cc++;
  }
  strcpy(resname, str.str().c_str());
}

void vtkWriter::EncodeWriteString(ostream* out, const char* name, bool doublePercent)
{
  if (!name)
  {
    return;
  }
  int cc = 0;

  char buffer[10];

  while( name[cc] )
  {
    // Encode spaces and %'s (and most non-printable ascii characters)
    // The reader does not support spaces in strings.
    if ( name[cc] < 33  || name[cc] > 126 ||
         name[cc] == '\"' || name[cc] == '%' )
    {
      snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned char>(name[cc]));
      if (doublePercent)
      {
        *out << "%%";
      }
      else
      {
        *out << "%";
      }
      *out << buffer;
    }
    else
    {
      *out << name[cc];
    }
    cc++;
  }
}


