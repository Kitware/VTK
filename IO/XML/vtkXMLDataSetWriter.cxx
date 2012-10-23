/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLDataSetWriter.h"

#include "vtkCallbackCommand.h"
#include "vtkDataSet.h"
#include "vtkHyperOctree.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLHyperOctreeWriter.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkAlgorithmOutput.h"

vtkStandardNewMacro(vtkXMLDataSetWriter);

//----------------------------------------------------------------------------
vtkXMLDataSetWriter::vtkXMLDataSetWriter()
{
  // Setup a callback for the internal writer to report progress.
  this->ProgressObserver = vtkCallbackCommand::New();
  this->ProgressObserver->SetCallback(&vtkXMLDataSetWriter::ProgressCallbackFunction);
  this->ProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
vtkXMLDataSetWriter::~vtkXMLDataSetWriter()
{
  this->ProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLDataSetWriter::GetInput()
{
  return static_cast<vtkDataSet*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
int vtkXMLDataSetWriter::WriteInternal()
{
  vtkAlgorithmOutput* input = this->GetInputConnection(0, 0);
  vtkXMLWriter* writer = 0;

  // Create a writer based on the data set type.
  switch (this->GetInput()->GetDataObjectType())
    {
    case VTK_UNIFORM_GRID:
    case VTK_IMAGE_DATA:
    case VTK_STRUCTURED_POINTS:
      {
      vtkXMLImageDataWriter* w = vtkXMLImageDataWriter::New();
      w->SetInputConnection(input);
      writer = w;
      } break;
    case VTK_STRUCTURED_GRID:
      {
      vtkXMLStructuredGridWriter* w = vtkXMLStructuredGridWriter::New();
      w->SetInputConnection(input);
      writer = w;
      } break;
    case VTK_RECTILINEAR_GRID:
      {
      vtkXMLRectilinearGridWriter* w = vtkXMLRectilinearGridWriter::New();
      w->SetInputConnection(input);
      writer = w;
      } break;
    case VTK_UNSTRUCTURED_GRID:
      {
      vtkXMLUnstructuredGridWriter* w = vtkXMLUnstructuredGridWriter::New();
      w->SetInputConnection(input);
      writer = w;
      } break;
    case VTK_POLY_DATA:
      {
      vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
      w->SetInputConnection(input);
      writer = w;
      } break;
    case VTK_HYPER_OCTREE:
      {
      vtkXMLHyperOctreeWriter* w = vtkXMLHyperOctreeWriter::New();
      w->SetInputConnection(input);
      writer = w;
      } break;
    }

  // Make sure we got a valid writer for the data set.
  if(!writer)
    {
    vtkErrorMacro("Cannot write dataset type: "
                  << this->GetInput()->GetDataObjectType() << " which is a "
                  << this->GetInput()->GetClassName());
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
  writer->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);

  // Try to write.
  int result = writer->Write();

  // Cleanup.
  writer->RemoveObserver(this->ProgressObserver);
  writer->Delete();
  return result;
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

//----------------------------------------------------------------------------
void vtkXMLDataSetWriter::ProgressCallbackFunction(vtkObject* caller,
                                                   unsigned long,
                                                   void* clientdata, void*)
{
  vtkAlgorithm* w = vtkAlgorithm::SafeDownCast(caller);
  if(w)
    {
    reinterpret_cast<vtkXMLDataSetWriter*>(clientdata)->ProgressCallback(w);
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataSetWriter::ProgressCallback(vtkAlgorithm* w)
{
  float width = this->ProgressRange[1]-this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress*width;
  this->UpdateProgressDiscrete(progress);
  if(this->AbortExecute)
    {
    w->SetAbortExecute(1);
    }
}

//----------------------------------------------------------------------------
int vtkXMLDataSetWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
