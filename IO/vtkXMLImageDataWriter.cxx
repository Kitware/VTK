/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLImageDataWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLImageDataWriter.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"

vtkCxxRevisionMacro(vtkXMLImageDataWriter, "1.1");
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
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLImageDataWriter::SetInput(vtkImageData* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLImageDataWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkImageData*>(this->Inputs[0]);
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
void vtkXMLImageDataWriter::WritePrimaryElementAttributes()
{
  this->Superclass::WritePrimaryElementAttributes();
  vtkImageData* input = this->GetInput();
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
  this->WriteVectorAttribute("Spacing", 3, input->GetSpacing());
}
