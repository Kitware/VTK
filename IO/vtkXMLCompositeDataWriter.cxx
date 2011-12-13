/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLCompositeDataWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCompositeDataWriter.h"

#include "vtkAMRBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPDataWriter.h"
#include "vtkXMLPImageDataWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLPPolyDataWriter.h"
#include "vtkXMLPRectilinearGridWriter.h"
#include "vtkXMLPStructuredGridWriter.h"
#include "vtkXMLPUnstructuredGridWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkXMLWriter.h"
#include "vtkCompositeDataIterator.h"
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>
#include <string>
#include <vector>

//----------------------------------------------------------------------------

class vtkXMLCompositeDataWriterInternals
{
public:
  std::vector< vtkSmartPointer<vtkXMLWriter> > Writers;
  std::string FilePath;
  std::string FilePrefix;
  vtkSmartPointer<vtkXMLDataElement> Root;
  std::vector<int> DataTypes;
};

//----------------------------------------------------------------------------
vtkXMLCompositeDataWriter::vtkXMLCompositeDataWriter()
{
  this->Internal = new vtkXMLCompositeDataWriterInternals;
  this->GhostLevel = 0;
  this->WriteMetaFile = 1;
  
  // Setup a callback for the internal writers to report progress.
  this->ProgressObserver = vtkCallbackCommand::New();
  this->ProgressObserver->SetCallback(&vtkXMLCompositeDataWriter::ProgressCallbackFunction);
  this->ProgressObserver->SetClientData(this);

  this->InputInformation = 0;
}

//----------------------------------------------------------------------------
vtkXMLCompositeDataWriter::~vtkXMLCompositeDataWriter()
{
  this->ProgressObserver->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
unsigned int vtkXMLCompositeDataWriter::GetNumberOfDataTypes()
{
  return static_cast<unsigned int>(this->Internal->DataTypes.size());
}

//----------------------------------------------------------------------------
int* vtkXMLCompositeDataWriter::GetDataTypesPointer()
{
  return &this->Internal->DataTypes[0];
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "WriteMetaFile: " << this->WriteMetaFile<< endl;
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataWriter::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::SetWriteMetaFile(int flag)
{
  if(this->WriteMetaFile != flag)
    {
    this->WriteMetaFile = flag;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataWriter::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 
    this->GhostLevel);
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataWriter::RequestData(vtkInformation*,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  this->InputInformation = inInfo;

  vtkCompositeDataSet *compositeData = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!compositeData) 
    {
    vtkErrorMacro("No hierarchical input has been provided. Cannot write");
    this->InputInformation = 0;
    return 0;
    }

  // Create writers for each input.
  this->CreateWriters(compositeData);

  this->SetErrorCode(vtkErrorCode::NoError);

  // Make sure we have a file to write.
  if(!this->Stream && !this->FileName)
    {
    vtkErrorMacro("Writer called with no FileName set.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    this->InputInformation = 0;
    return 0;
    }

  // We are just starting to write.  Do not call
  // UpdateProgressDiscrete because we want a 0 progress callback the
  // first time.
  this->UpdateProgress(0);

  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = {0,1};
  this->SetProgressRange(wholeProgressRange, 0, 1);

  // Prepare file prefix for creation of internal file names.
  this->SplitFileName();
  
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  
  // Create the subdirectory for the internal files.
  std::string subdir = this->Internal->FilePath;
  subdir += this->Internal->FilePrefix;
  this->MakeDirectory(subdir.c_str());
 
  this->Internal->Root = vtkSmartPointer<vtkXMLDataElement>::New();
  this->Internal->Root->SetName(compositeData->GetClassName());

  int writerIdx=0;
  if (!this->WriteComposite(compositeData, this->Internal->Root, writerIdx))
    {
    this->RemoveWrittenFiles(subdir.c_str());
    return 0;
    }

  if (this->WriteMetaFile)
    {
    this->SetProgressRange(progressRange, this->GetNumberOfInputConnections(0),
                           this->GetNumberOfInputConnections(0)
                           + this->WriteMetaFile);
    int retVal = this->WriteMetaFileIfRequested();
    this->InputInformation = 0;
    return retVal;
    }

  // We have finished writing.
  this->UpdateProgressDiscrete(1);

  this->InputInformation = 0;
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataWriter::WriteNonCompositeData(
  vtkDataObject* dObj, vtkXMLDataElement* datasetXML, int &writerIdx, 
  const char* fileName)
{
  // Write a leaf dataset.
  int myWriterIndex = writerIdx;
  writerIdx++;

  // Locate the actual data writer for this dataset.
  vtkXMLWriter* writer = this->GetWriter(myWriterIndex);
  if (!writer)
    {
    return 0;
    }

  vtkDataSet* curDS = vtkDataSet::SafeDownCast(dObj);
  if (!curDS)
    {
    if (dObj)
      {
      vtkWarningMacro("This writer cannot handle sub-datasets of type: "
        << dObj->GetClassName()
        << " Dataset will be skipped.");
      }
    return 0;
    }

  if (datasetXML)
    {
    // Create the entry for the collection file.
    datasetXML->SetAttribute("file", fileName);
    }

  // FIXME
  // this->SetProgressRange(progressRange, myWriterIndex,
  //                       GetNumberOfInputConnections(0)+writeCollection);

  std::string full = this->Internal->FilePath;
  full += fileName;

  writer->SetFileName(full.c_str());

  // Write the data.
  writer->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);      
  writer->Write();
  writer->RemoveObserver(this->ProgressObserver);

  if (writer->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataWriter::WriteData()
{
  // Write the collection file.
  this->StartFile();
  vtkIndent indent = vtkIndent().GetNextIndent();
 

  // Open the primary element.
  ostream& os = *(this->Stream);

  if (this->Internal->Root)
    {
    this->Internal->Root->PrintXML(os, indent);
    }

  return this->EndFile();
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataWriter::WriteMetaFileIfRequested()
{
  if(this->WriteMetaFile)
    {
    if (!this->Superclass::WriteInternal()) 
      { 
      return 0; 
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::MakeDirectory(const char* name)
{
  if( !vtksys::SystemTools::MakeDirectory(name) )
    {
    vtkErrorMacro( << "Sorry unable to create directory: " << name 
                   << endl << "Last systen error was: " 
                   << vtksys::SystemTools::GetLastSystemError().c_str() );
    }
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::RemoveADirectory(const char* name)
{
  if( !vtksys::SystemTools::RemoveADirectory(name) )
    {
    vtkErrorMacro( << "Sorry unable to remove a directory: " << name 
                   << endl << "Last system error was: " 
                   << vtksys::SystemTools::GetLastSystemError().c_str() );
    }
}

//----------------------------------------------------------------------------
const char* vtkXMLCompositeDataWriter::GetDefaultFileExtension()
{
  return "vtm";
}

//----------------------------------------------------------------------------
const char* vtkXMLCompositeDataWriter::GetDataSetName()
{
  if (!this->InputInformation)
    {
    return "CompositeDataSet";
    }
  vtkDataObject *hdInput = vtkDataObject::SafeDownCast(
    this->InputInformation->Get(vtkDataObject::DATA_OBJECT()));
  if (!hdInput) 
    {
    return 0;
    }
  return hdInput->GetClassName();
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::FillDataTypes(vtkCompositeDataSet* hdInput)
{
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(hdInput->NewIterator());
  iter->VisitOnlyLeavesOn();
  iter->TraverseSubTreeOn();
  iter->SkipEmptyNodesOff();

  this->Internal->DataTypes.clear();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(
      iter->GetCurrentDataObject());
    if (ds)
      {
      this->Internal->DataTypes.push_back(ds->GetDataObjectType());
      }
    else
      {
      this->Internal->DataTypes.push_back(-1);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::CreateWriters(vtkCompositeDataSet* hdInput)
{
  this->Internal->Writers.clear();
  this->FillDataTypes(hdInput);

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(hdInput->NewIterator());
  iter->VisitOnlyLeavesOn();
  iter->TraverseSubTreeOn();
  iter->SkipEmptyNodesOff();

  size_t numDatasets = this->Internal->DataTypes.size();
  this->Internal->Writers.resize(numDatasets);

  int i = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    this->Internal->Writers[i] = NULL;
    vtkDataSet* ds = vtkDataSet::SafeDownCast(
      iter->GetCurrentDataObject());
    if(ds)
      {
      // Create a writer based on the type of this input.
      switch (this->Internal->DataTypes[i])
        {
        case VTK_POLY_DATA:    
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLPolyDataWriter") != 0))
            {
            vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLPolyDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(ds);
          break;
        case VTK_STRUCTURED_POINTS:
        case VTK_IMAGE_DATA:
        case VTK_UNIFORM_GRID:
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLImageDataWriter") != 0))
            {
            vtkXMLImageDataWriter* w = vtkXMLImageDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLImageDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer())
            ->SetInput(ds);
          break;
        case VTK_UNSTRUCTURED_GRID:
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLUnstructuredGridWriter") != 0))
            {
            vtkXMLUnstructuredGridWriter* w = vtkXMLUnstructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLUnstructuredGridWriter::SafeDownCast(
            this->Internal->Writers[i].GetPointer())->SetInput(ds);
          break;
        case VTK_STRUCTURED_GRID:
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLStructuredGridWriter") != 0))
            {
            vtkXMLStructuredGridWriter* w = vtkXMLStructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLStructuredGridWriter::SafeDownCast(
            this->Internal->Writers[i].GetPointer())->SetInput(ds);
          break;
        case VTK_RECTILINEAR_GRID:
          if(!this->Internal->Writers[i].GetPointer() ||
             (strcmp(this->Internal->Writers[i]->GetClassName(),
                     "vtkXMLRectilinearGridWriter") != 0))
            {
            vtkXMLRectilinearGridWriter* w = vtkXMLRectilinearGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
            }
          vtkXMLRectilinearGridWriter::SafeDownCast(
            this->Internal->Writers[i].GetPointer())->SetInput(ds);
          break;
        default:
          this->Internal->Writers[i] = 0;
        }
      
      // Copy settings to the writer.
      if(vtkXMLWriter* w = this->Internal->Writers[i].GetPointer())
        {
        w->SetDebug(this->GetDebug());
        w->SetByteOrder(this->GetByteOrder());
        w->SetCompressor(this->GetCompressor());
        w->SetBlockSize(this->GetBlockSize());
        w->SetDataMode(this->GetDataMode());
        w->SetEncodeAppendedData(this->GetEncodeAppendedData());
        }
      
      // If this is a parallel writer, set the piece information.
      if(vtkXMLPDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer()))
        {
        vtkErrorMacro("Should not have parallel writers here.");
        }
      }
    i++;
    }
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLCompositeDataWriter::GetWriter(int index)
{
  int size = static_cast<int>(this->Internal->Writers.size());
  if(index >= 0 && index < size)
    {
    return this->Internal->Writers[index].GetPointer();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::SplitFileName()
{
  std::string fileName = this->FileName;
  std::string name;
  
  // Split the file name and extension from the path.
  std::string::size_type pos = fileName.find_last_of("/\\");
  if(pos != fileName.npos)
    {
    // Keep the slash in the file path.
    this->Internal->FilePath = fileName.substr(0, pos+1);
    name = fileName.substr(pos+1);
    }
  else
    {
    this->Internal->FilePath = "./";
    name = fileName;
    }
  
  // Split the extension from the file name.
  pos = name.find_last_of(".");
  if(pos != name.npos)
    {
    this->Internal->FilePrefix = name.substr(0, pos);
    }
  else
    {
    this->Internal->FilePrefix = name;
    
    // Since a subdirectory is used to store the files, we need to
    // change its name if there is no file extension.
    this->Internal->FilePrefix += "_data";
    }
}

//----------------------------------------------------------------------------
const char* vtkXMLCompositeDataWriter::GetFilePrefix()
{
  return this->Internal->FilePrefix.c_str();
}

//----------------------------------------------------------------------------
const char* vtkXMLCompositeDataWriter::GetFilePath()
{
  return this->Internal->FilePath.c_str();
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::ProgressCallbackFunction(vtkObject* caller,
                                                        unsigned long,
                                                        void* clientdata,
                                                        void*)
{
  vtkAlgorithm* w = vtkAlgorithm::SafeDownCast(caller);
  if(w)
    {
    reinterpret_cast<vtkXMLCompositeDataWriter*>(clientdata)->ProgressCallback(w);
    }
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::ProgressCallback(vtkAlgorithm* w)
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
vtkStdString vtkXMLCompositeDataWriter::CreatePieceFileName(
  int piece)
{
  std::string fname;
  if(this->Internal->DataTypes[piece] < 0)
    {
    return fname;
    }

  vtksys_ios::ostringstream stream;
  stream << this->Internal->FilePrefix.c_str() << "/"
         << this->Internal->FilePrefix.c_str()
         << "_" << piece << ".";
  switch (this->Internal->DataTypes[piece])
    {
  case VTK_POLY_DATA:    
    stream << "vtp";
    break;
  
  case VTK_STRUCTURED_POINTS:
  case VTK_IMAGE_DATA:
  case VTK_UNIFORM_GRID:
    stream << "vti";
    break;
  
  case VTK_UNSTRUCTURED_GRID:
    stream << "vtu";
    break;

  case VTK_STRUCTURED_GRID:
    stream << "vts";
    break;

  case VTK_RECTILINEAR_GRID:
    stream << "vtr";
    break;

  default:
    return "";
    }
  fname = stream.str();
  return fname;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  int size = static_cast<int>(this->Internal->Writers.size());
  for(int i=0; i < size; ++i)
    {
    vtkGarbageCollectorReport(collector, this->Internal->Writers[i], "Writer");
    }
}

//----------------------------------------------------------------------------
vtkExecutive* vtkXMLCompositeDataWriter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataWriter::RemoveWrittenFiles(const char* SubDirectory)
{
  this->RemoveADirectory(SubDirectory);
  this->DeleteAFile();
  this->InputInformation = 0;
}
