/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOMFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOMFReader.h"
#include "core/OMFProject.h"

#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"

#include <algorithm>

vtkStandardNewMacro(vtkOMFReader);

struct vtkOMFReader::ReaderImpl
{
  omf::OMFProject Project;
  vtkNew<vtkDataArraySelection> DataElementSelection;
  bool FileParsed = false;
};

//------------------------------------------------------------------------------
vtkOMFReader::vtkOMFReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = nullptr;
  this->Impl = new ReaderImpl;
}

//------------------------------------------------------------------------------
vtkOMFReader::~vtkOMFReader()
{
  delete[] this->FileName;
  delete this->Impl;
}

//------------------------------------------------------------------------------
bool vtkOMFReader::GetDataElementArrayStatus(const char* name)
{
  return this->Impl->DataElementSelection->ArrayIsEnabled(name);
}

//------------------------------------------------------------------------------
void vtkOMFReader::SetDataElementArrayStatus(const char* name, int status)
{
  this->Impl->DataElementSelection->SetArraySetting(name, status);
}

//------------------------------------------------------------------------------
int vtkOMFReader::GetNumberOfDataElementArrays()
{
  return this->Impl->DataElementSelection->GetNumberOfArrays();
}

//------------------------------------------------------------------------------
const char* vtkOMFReader::GetDataElementArrayName(int index)
{
  if (index < 0 || index >= this->Impl->DataElementSelection->GetNumberOfArrays())
  {
    return nullptr;
  }
  return this->Impl->DataElementSelection->GetArrayName(index);
}

//------------------------------------------------------------------------------
vtkDataArraySelection* vtkOMFReader::GetDataElementArraySelection()
{
  return this->Impl->DataElementSelection;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkOMFReader::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->Impl->DataElementSelection->GetMTime());
}

//------------------------------------------------------------------------------
int vtkOMFReader::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    output = vtkPartitionedDataSetCollection::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkOMFReader::RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  if (!this->Impl->FileParsed)
  {
    if (!this->Impl->Project.CanParseFile(this->FileName, this->Impl->DataElementSelection))
    {
      vtkErrorMacro(<< "Can't read file " << this->FileName << " with vtkOMFReader");
      return VTK_ERROR;
    }
    this->Impl->FileParsed = true;
  }
  if (!this->Impl->DataElementSelection->GetNumberOfArrays())
  {
    vtkErrorMacro("No data was found in the OMF file. Abort reading");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkOMFReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(request), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkNew<vtkDataAssembly> assembly;
  output->SetDataAssembly(assembly);

  // If parsed successfully into Json, then convert it
  if (!this->Impl->Project.ProcessJSON(output, this->Impl->DataElementSelection,
        this->WriteOutTextures, this->ColumnMajorOrdering))
  {
    vtkErrorMacro(<< "OMF file " << this->FileName << " could not be read correctly");
    return VTK_ERROR;
  }
  return VTK_OK;
}

//------------------------------------------------------------------------------
int vtkOMFReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}

//------------------------------------------------------------------------------
void vtkOMFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
