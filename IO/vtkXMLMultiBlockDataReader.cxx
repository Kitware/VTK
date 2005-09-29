/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiBlockDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLMultiBlockDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkXMLMultiBlockDataReader, "1.1");
vtkStandardNewMacro(vtkXMLMultiBlockDataReader);

//----------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::vtkXMLMultiBlockDataReader()
{
}

//----------------------------------------------------------------------------
vtkXMLMultiBlockDataReader::~vtkXMLMultiBlockDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLMultiBlockDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkXMLMultiBlockDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkXMLMultiBlockDataReader::GetOutput(int port)
{
  vtkDataObject* output = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkMultiBlockDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
int vtkXMLMultiBlockDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkXMLMultiBlockDataReader::GetDataSetName()
{
  return "vtkMultiBlockDataSet";
}

