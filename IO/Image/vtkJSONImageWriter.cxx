/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJSONImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJSONImageWriter.h"

#include "vtkCharArray.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkJSONImageWriter);

//----------------------------------------------------------------------------
vtkJSONImageWriter::vtkJSONImageWriter()
{
  this->FileName = NULL;
  this->ArrayName = NULL;
  this->Slice = -1;
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkJSONImageWriter::~vtkJSONImageWriter()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
void vtkJSONImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------

int vtkJSONImageWriter::RequestData(
  vtkInformation* vtkNotUsed( request ),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector) )
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *input =
    vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Error checking
  if (input == NULL )
    {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return 0;
    }
  if ( !this->FileName)
    {
    vtkErrorMacro(<<"Write:Please specify either a FileName or a file prefix and pattern");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
    }

  // Write
  this->InvokeEvent(vtkCommand::StartEvent);
  vtkCharArray *validMask = vtkCharArray::SafeDownCast(input->GetPointData()->GetArray("vtkValidPointMask"));

  ofstream file(this->FileName, ios::out);
  if (file.fail())
    {
    vtkErrorMacro("RecursiveWrite: Could not open file " <<
                  this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    return 0;
    }
  file << "{"
       << "\"filename\" : \"" << this->FileName << "\""
       << ",\n\"dimensions\": [" << input->GetDimensions()[0] << ", " << input->GetDimensions()[1] << ", " << input->GetDimensions()[2] << "]"
       << ",\n\"origin\": [" << input->GetOrigin()[0] << ", " << input->GetOrigin()[1] << ", " << input->GetOrigin()[2] << "]"
       << ",\n\"spacing\": [" << input->GetSpacing()[0] << ", " << input->GetSpacing()[1] << ", " << input->GetSpacing()[2] << "]";

  // Write all arrays
  int nbArrays = input->GetPointData()->GetNumberOfArrays();
  for(int i=0; i < nbArrays; ++i)
    {
    vtkDataArray* array = input->GetPointData()->GetArray(i);
    // We only dump scalar values
    if(array->GetNumberOfComponents() != 1 || !strcmp(array->GetName(),"vtkValidPointMask"))
      {
      continue;
      }
    if(this->ArrayName && strlen(this->ArrayName) > 0 && strcmp(array->GetName(),this->ArrayName))
      {
      continue;
      }
    file << ",\n\"" << array->GetName() << "\": [";
    vtkIdType startIdx = 0;
    vtkIdType endIdx = array->GetNumberOfTuples();
    if(this->Slice >= 0)
      {
      vtkIdType sliceSize = input->GetDimensions()[0] * input->GetDimensions()[1];
      startIdx = sliceSize * this->Slice;
      endIdx = startIdx + sliceSize;
      }
    for(vtkIdType idx = startIdx; idx < endIdx; ++idx)
      {
      if(idx % 50 == 0)
        {
        file << "\n";
        file.flush();
        }
      if(idx != startIdx)
        {
        file << ", ";
        }
      if((validMask == NULL) || (validMask && validMask->GetValue(idx)))
        {
        file << array->GetVariantValue(idx).ToString();
        }
      else
        {
        file << "null";
        }

      }
    file << "]";
    }

  file << "\n}" << endl;
  file.close();
  file.flush();

  this->InvokeEvent(vtkCommand::EndEvent);
  return 1;
}

//----------------------------------------------------------------------------
void vtkJSONImageWriter::Write()
{
  this->Modified();
  this->UpdateInformation();
  vtkInformation* inInfo = this->GetInputInformation(0, 0);
  vtkStreamingDemandDrivenPipeline::SetUpdateExtent(inInfo,
    vtkStreamingDemandDrivenPipeline::GetWholeExtent(inInfo));
  this->Update();
}
