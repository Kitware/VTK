/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataSetWriter.cxx
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
#include "vtkXMLDataSetWriter.h"

#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"

vtkCxxRevisionMacro(vtkXMLDataSetWriter, "1.1");
vtkStandardNewMacro(vtkXMLDataSetWriter);

//----------------------------------------------------------------------------
vtkXMLDataSetWriter::vtkXMLDataSetWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLDataSetWriter::~vtkXMLDataSetWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLDataSetWriter::SetInput(vtkDataSet* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLDataSetWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkDataSet*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
int vtkXMLDataSetWriter::Write()
{
  vtkDataSet* input = this->GetInput();
  vtkXMLWriter* writer = 0;
  
  // Create a writer based on the data set type.
  switch (input->GetDataObjectType())
    {
    case VTK_IMAGE_DATA:
    case VTK_STRUCTURED_POINTS:
      {
      vtkXMLImageDataWriter* w = vtkXMLImageDataWriter::New();
      w->SetInput(static_cast<vtkImageData*>(input));
      writer = w;
      } break;
    case VTK_STRUCTURED_GRID:
      {
      vtkXMLStructuredGridWriter* w = vtkXMLStructuredGridWriter::New();
      w->SetInput(static_cast<vtkStructuredGrid*>(input));
      writer = w;
      } break;
    case VTK_RECTILINEAR_GRID:
      {
      vtkXMLRectilinearGridWriter* w = vtkXMLRectilinearGridWriter::New();
      w->SetInput(static_cast<vtkRectilinearGrid*>(input));
      writer = w;
      } break;
    case VTK_UNSTRUCTURED_GRID:
      {
      vtkXMLUnstructuredGridWriter* w = vtkXMLUnstructuredGridWriter::New();
      w->SetInput(static_cast<vtkUnstructuredGrid*>(input));
      writer = w;
      } break;
    case VTK_POLY_DATA:
      {
      vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
      w->SetInput(static_cast<vtkPolyData*>(input));
      writer = w;
      } break;
    }
  
  // Make sure we got a valid writer for the data set.
  if(!writer)
    {
    vtkErrorMacro("Cannot write dataset type: "
                  << input->GetDataObjectType());
    return 0;
    }
  
  // Copy the settings to the writer.
  writer->SetDebug(this->GetDebug());
  writer->SetFileName(this->GetFileName());
  writer->SetByteOrder(this->GetByteOrder());
  writer->SetCompressor(this->GetCompressor());
  writer->SetBlockSize(this->GetBlockSize());
  writer->SetDataMode(this->GetDataMode());
  writer->SetEncodeAppendedData(this->GetEncodeAppendedData());
  
  // Try to write.
  int result = writer->Write();
  
  // Cleanup.
  writer->Delete();
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLDataSetWriter::WriteData()
{
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkXMLDataSetWriter::GetDataSetName()
{
  return "DataSet";
}

//----------------------------------------------------------------------------
const char* vtkXMLDataSetWriter::GetDefaultFileExtension()
{
  return "vtk";
}
