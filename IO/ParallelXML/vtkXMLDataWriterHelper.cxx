/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataWriterHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLDataWriterHelper.h"

#include "vtkCompositeDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLWriter2.h"

vtkStandardNewMacro(vtkXMLDataWriterHelper);
vtkCxxSetObjectMacro(vtkXMLDataWriterHelper, Writer, vtkXMLWriter2);
//----------------------------------------------------------------------------
vtkXMLDataWriterHelper::vtkXMLDataWriterHelper()
  : Writer(nullptr)
  , DataSetVersion{ 0, 0 }
{
}

//----------------------------------------------------------------------------
vtkXMLDataWriterHelper::~vtkXMLDataWriterHelper()
{
  this->SetWriter(nullptr);
}

//----------------------------------------------------------------------------
bool vtkXMLDataWriterHelper::OpenFile()
{
  assert(this->Writer != nullptr);
  this->SetDebug(this->Writer->GetDebug());
  this->SetByteOrder(this->Writer->GetByteOrder());
  this->SetCompressor(this->Writer->GetCompressor());
  this->SetBlockSize(this->Writer->GetBlockSize());
  this->SetDataMode(this->Writer->GetDataMode());
  this->SetEncodeAppendedData(this->Writer->GetEncodeAppendedData());
  this->SetHeaderType(this->Writer->GetHeaderType());
  this->SetIdType(this->Writer->GetIdType());
  this->SetWriteToOutputString(this->Writer->GetWriteToOutputString());
  this->SetFileName(this->Writer->GetFileName());

  return this->Superclass::OpenStream() != 0;
}

//----------------------------------------------------------------------------
bool vtkXMLDataWriterHelper::BeginWriting()
{
  return this->Superclass::StartFile() != 0;
}

//----------------------------------------------------------------------------
bool vtkXMLDataWriterHelper::EndWriting()
{
  const bool status = (this->Superclass::EndFile() != 0);
  this->Superclass::CloseStream();
  return status;
}

//----------------------------------------------------------------------------
bool vtkXMLDataWriterHelper::AddGlobalFieldData(vtkCompositeDataSet* input)
{
  // We want to avoid using appended data mode as it
  // is not supported in meta formats.
  int dataMode = this->DataMode;
  if (dataMode == vtkXMLWriterBase::Appended)
  {
    this->DataMode = vtkXMLWriterBase::Binary;
  }

  auto meta = input->GetInformation();
  const bool hasTime = meta->Has(vtkDataObject::DATA_TIME_STEP()) != 0;
  auto fieldData = input->GetFieldData();
  if ((fieldData && fieldData->GetNumberOfArrays()) || hasTime)
  {
    vtkNew<vtkFieldData> fieldDataCopy;
    fieldDataCopy->ShallowCopy(fieldData);
    if (hasTime)
    {
      vtkNew<vtkDoubleArray> time;
      time->SetNumberOfTuples(1);
      time->SetTypedComponent(0, 0, meta->Get(vtkDataObject::DATA_TIME_STEP()));
      time->SetName("TimeValue");
      fieldDataCopy->AddArray(time);
    }
    this->WriteFieldDataInline(fieldDataCopy, vtkIndent().GetNextIndent());
  }
  this->DataMode = dataMode;
  return true;
}

//----------------------------------------------------------------------------
bool vtkXMLDataWriterHelper::AddXML(vtkXMLDataElement* xmlElement)
{
  if (xmlElement)
  {
    xmlElement->PrintXML((*this->Stream), vtkIndent().GetNextIndent());
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkXMLDataWriterHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
