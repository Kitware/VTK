/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLImageDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLImageDataWriter.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"

vtkStandardNewMacro(vtkXMLImageDataWriter);

//----------------------------------------------------------------------------
vtkXMLImageDataWriter::vtkXMLImageDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLImageDataWriter::~vtkXMLImageDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLImageDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLImageDataWriter::GetInput()
{
  return static_cast<vtkImageData*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
void vtkXMLImageDataWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//----------------------------------------------------------------------------
const char* vtkXMLImageDataWriter::GetDataSetName()
{
  return "ImageData";
}

//----------------------------------------------------------------------------
const char* vtkXMLImageDataWriter::GetDefaultFileExtension()
{
  return "vti";
}

//----------------------------------------------------------------------------
void vtkXMLImageDataWriter::WritePrimaryElementAttributes(ostream &os, vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  vtkImageData* input = this->GetInput();
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
  this->WriteVectorAttribute("Spacing", 3, input->GetSpacing());
}

//----------------------------------------------------------------------------
int vtkXMLImageDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
