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

#include "vtkAlgorithmOutput.h"
#include "vtkCallbackCommand.h"
#include "vtkDataSet.h"
#include "vtkHyperOctree.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLHyperOctreeWriter.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"

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
vtkXMLWriter* vtkXMLDataSetWriter::NewWriter(int dataset_type)
{
  // Create a writer based on the data set type.
  switch (dataset_type)
  {
    case VTK_UNIFORM_GRID:
    case VTK_IMAGE_DATA:
    case VTK_STRUCTURED_POINTS:
      return vtkXMLImageDataWriter::New();
    case VTK_STRUCTURED_GRID:
      return vtkXMLStructuredGridWriter::New();
    case VTK_RECTILINEAR_GRID:
      return vtkXMLRectilinearGridWriter::New();
    case VTK_UNSTRUCTURED_GRID:
      return vtkXMLUnstructuredGridWriter::New();
    case VTK_POLY_DATA:
      return vtkXMLPolyDataWriter::New();
    case VTK_HYPER_OCTREE:
      return vtkXMLHyperOctreeWriter::New();
  }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkXMLDataSetWriter::WriteInternal()
{
  // Create a writer based on the data set type.
  vtkXMLWriter* writer =
    vtkXMLDataSetWriter::NewWriter(this->GetInput()->GetDataObjectType());
  if (writer)
  {
    writer->SetInputConnection(this->GetInputConnection(0, 0));

    // Copy the settings to the writer.
    writer->SetDebug(this->GetDebug());
    writer->SetFileName(this->GetFileName());
    writer->SetByteOrder(this->GetByteOrder());
    writer->SetCompressor(this->GetCompressor());
    writer->SetBlockSize(this->GetBlockSize());
    writer->SetDataMode(this->GetDataMode());
    writer->SetEncodeAppendedData(this->GetEncodeAppendedData());
    writer->SetHeaderType(this->GetHeaderType());
    writer->SetIdType(this->GetIdType());
    writer->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);

    // Try to write.
    int result = writer->Write();

    // Cleanup.
    writer->RemoveObserver(this->ProgressObserver);
    writer->Delete();
    return result;
  }

  // Make sure we got a valid writer for the data set.
  vtkErrorMacro("Cannot write dataset type: "
                << this->GetInput()->GetDataObjectType() << " which is a "
                << this->GetInput()->GetClassName());
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

//----------------------------------------------------------------------------
void vtkXMLDataSetWriter::ProgressCallbackFunction(vtkObject* caller,
                                                   unsigned long,
                                                   void* clientdata, void*)
{
  vtkAlgorithm* w = vtkAlgorithm::SafeDownCast(caller);
  if (w)
  {
    reinterpret_cast<vtkXMLDataSetWriter*>(clientdata)->ProgressCallback(w);
  }
}

//----------------------------------------------------------------------------
void vtkXMLDataSetWriter::ProgressCallback(vtkAlgorithm* w)
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress*width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
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
