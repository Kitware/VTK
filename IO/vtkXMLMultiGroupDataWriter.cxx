/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiGroupDataWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLMultiGroupDataWriter.h"

#include "vtkAMRBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkExecutive.h"
#include "vtkErrorCode.h"
#include "vtkGarbageCollector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPDataWriter.h"
#include "vtkXMLPImageDataWriter.h"
#include "vtkXMLPPolyDataWriter.h"
#include "vtkXMLPRectilinearGridWriter.h"
#include "vtkXMLPStructuredGridWriter.h"
#include "vtkXMLPUnstructuredGridWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkXMLWriter.h"
#include <vtksys/SystemTools.hxx>

#include <vtkstd/string>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLMultiGroupDataWriter);
vtkCxxRevisionMacro(vtkXMLMultiGroupDataWriter, "1.2");

class vtkXMLMultiGroupDataWriterInternals
{
public:
  vtkstd::vector< vtkSmartPointer<vtkXMLWriter> > Writers;
  vtkstd::string FilePath;
  vtkstd::string FilePrefix;
  vtkstd::vector<vtkstd::string> Entries;
  vtkstd::string CreatePieceFileName(int index);
  vtkstd::vector<int> DataTypes;
};

//----------------------------------------------------------------------------
vtkXMLMultiGroupDataWriter::vtkXMLMultiGroupDataWriter()
{
  this->Internal = new vtkXMLMultiGroupDataWriterInternals;
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
  this->WriteMetaFileInitialized = 0;
  this->WriteMetaFile = 0;
  
  // Setup a callback for the internal writers to report progress.
  this->ProgressObserver = vtkCallbackCommand::New();
  this->ProgressObserver->SetCallback(&vtkXMLMultiGroupDataWriter::ProgressCallbackFunction);
  this->ProgressObserver->SetClientData(this);

  this->InputInformation = 0;
}

//----------------------------------------------------------------------------
vtkXMLMultiGroupDataWriter::~vtkXMLMultiGroupDataWriter()
{
  this->ProgressObserver->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
unsigned int vtkXMLMultiGroupDataWriter::GetNumberOfDataTypes()
{
  return this->Internal->DataTypes.size();
}

//----------------------------------------------------------------------------
int* vtkXMLMultiGroupDataWriter::GetDataTypesPointer()
{
  return &this->Internal->DataTypes[0];
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces<< endl;
  os << indent << "Piece: " << this->Piece<< endl;
  os << indent << "WriteMetaFile: " << this->WriteMetaFile<< endl;
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataWriter::ProcessRequest(
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
void vtkXMLMultiGroupDataWriter::SetWriteMetaFile(int flag)
{
  this->WriteMetaFileInitialized = 1;
  vtkDebugMacro(<< this->GetClassName() << " ("
                << this << "): setting WriteMetaFile to " << flag);
  if(this->WriteMetaFile != flag)
    {
    this->WriteMetaFile = flag;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataWriter::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataWriter::RequestData(vtkInformation*,
                                              vtkInformationVector** inputVector,
                                              vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  this->InputInformation = inInfo;

  vtkMultiGroupDataSet *hdInput = vtkMultiGroupDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!hdInput) 
    {
    vtkErrorMacro("No hierarchical input has been provided. Cannot write");
    this->InputInformation = 0;
    return 0;
    }

  vtkHierarchicalBoxDataSet* hdBoxInput = 
    vtkHierarchicalBoxDataSet::SafeDownCast(hdInput);

  // Create writers for each input.
  this->CreateWriters(hdInput);

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
  
  // Decide whether to write the collection file.
  int writeCollection = 0;
  if(this->WriteMetaFileInitialized)
    {
    writeCollection = this->WriteMetaFile;
    }
  else if(this->Piece == 0)
    {
    writeCollection = 1;
    }
  
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  
  // Create the subdirectory for the internal files.
  vtkstd::string subdir = this->Internal->FilePath;
  subdir += this->Internal->FilePrefix;
  this->MakeDirectory(subdir.c_str());
 
  this->DeleteAllEntries();

  if (hdBoxInput)
    {
    unsigned int numLevels = hdBoxInput->GetNumberOfLevels();
    for (unsigned int levelId=0; levelId<numLevels-1; levelId++)
      {
      ostrstream entry_with_warning_C4701;
      entry_with_warning_C4701
        << "<RefinementRatio level=\"" << levelId << "\""
        << " refinement=\"" << hdBoxInput->GetRefinementRatio(levelId) << "\""
        << "/>" << ends;
      this->AppendEntry(entry_with_warning_C4701.str());
      delete[] entry_with_warning_C4701.str();
      }
    }

  // Write each input.
  int i, j;
  unsigned int numGroups = hdInput->GetNumberOfGroups();
  i=0;
  for (unsigned int groupId=0; groupId<numGroups; groupId++)
    {
    unsigned int numDataSets = hdInput->GetNumberOfDataSets(groupId);
    for (unsigned int dataSetId=0; dataSetId<numDataSets; dataSetId++)
      {
      vtkXMLWriter* w = this->GetWriter(i);
      if (!w)
        {
        i++;
        continue;
        }
      // Set the file name.
      vtkstd::string fname = this->Internal->CreatePieceFileName(i);
      // Create the entry for the collection file.
      ostrstream entry_with_warning_C4701;
      entry_with_warning_C4701
        << "<DataSet group=\"" << groupId << "\" dataset=\"" << dataSetId << "\"";
      if (hdBoxInput)
        {
        vtkAMRBox box;
        hdBoxInput->GetDataSet(groupId, dataSetId, box);
        entry_with_warning_C4701
            << " amr_box=\"" 
            << box.LoCorner[0] << " "
            << box.HiCorner[0] << " "
            << box.LoCorner[1] << " "
            << box.HiCorner[1] << " "
            << box.LoCorner[2] << " "
            << box.HiCorner[2] << "\"";
        }
      entry_with_warning_C4701
        << " file=\"" << fname.c_str() << "\"/>" << ends;
      this->AppendEntry(entry_with_warning_C4701.str());
      delete[] entry_with_warning_C4701.str();
      
      vtkDataSet* ds = 
        vtkDataSet::SafeDownCast(hdInput->GetDataSet(groupId, dataSetId));
      if (!ds)
        {
        i++;
        continue;
        }
      this->SetProgressRange(progressRange, i,
                             GetNumberOfInputConnections(0)+writeCollection);

      vtkstd::string full = this->Internal->FilePath;
      full += fname;
      w->SetFileName(full.c_str());
      
      // Write the data.
      w->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);      
      w->Write();
      w->RemoveObserver(this->ProgressObserver);
      
      if (w->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
        {
        for (j = 0; j < i; j++)
          {
          fname = this->Internal->CreatePieceFileName(i);
          full = this->Internal->FilePath;
          full += fname;
          vtksys::SystemTools::RemoveFile(full.c_str());
          }
        this->RemoveADirectory(subdir.c_str());
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
        this->DeleteAFile();
        this->InputInformation = 0;
        return 0;
        }
      i++;
      }
    }
  
  // Write the collection file if requested.
  if(writeCollection)
    {
    this->SetProgressRange(progressRange, this->GetNumberOfInputConnections(0),
                           this->GetNumberOfInputConnections(0)
                           + writeCollection);
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
int vtkXMLMultiGroupDataWriter::WriteData()
{
  // Write the collection file.
  this->StartFile();
  vtkIndent indent = vtkIndent().GetNextIndent();
  
  // Open the primary element.
  ostream& os = *(this->Stream);
  os << indent << "<" << this->GetDataSetName() << ">\n";
  
  // Write the set of entries.
  for(vtkstd::vector<vtkstd::string>::const_iterator i =
        this->Internal->Entries.begin();
      i != this->Internal->Entries.end(); ++i)
    {
    os << indent.GetNextIndent() << i->c_str() << "\n";
    }
  
  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";
  return this->EndFile();
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataWriter::WriteMetaFileIfRequested()
{
  // Decide whether to write the collection file.
  int writeCollection = 0;
  if(this->WriteMetaFileInitialized)
    {
    writeCollection = this->WriteMetaFile;
    }
  else if(this->Piece == 0)
    {
    writeCollection = 1;
    }
  
  if(writeCollection)
    {
    if(!this->Superclass::WriteInternal()) { return 0; }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::MakeDirectory(const char* name)
{
  if( !vtksys::SystemTools::MakeDirectory(name) )
    {
    vtkErrorMacro( << "Sorry unable to create directory: " << name 
                   << endl << "Last systen error was: " 
                   << vtksys::SystemTools::GetLastSystemError().c_str() );
    }
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::RemoveADirectory(const char* name)
{
  if( !vtksys::SystemTools::RemoveADirectory(name) )
    {
    vtkErrorMacro( << "Sorry unable to remove a directory: " << name 
                   << endl << "Last systen error was: " 
                   << vtksys::SystemTools::GetLastSystemError().c_str() );
    }

}

//----------------------------------------------------------------------------
const char* vtkXMLMultiGroupDataWriter::GetDefaultFileExtension()
{
  return "vth";
}

//----------------------------------------------------------------------------
const char* vtkXMLMultiGroupDataWriter::GetDataSetName()
{
  if (!this->InputInformation)
    {
    return "MultiGroupDataSet";
    }
  vtkDataObject *hdInput = vtkDataObject::SafeDownCast(
    this->InputInformation->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!hdInput) 
    {
    return 0;
    }
  return hdInput->GetClassName();
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::FillDataTypes(vtkMultiGroupDataSet* hdInput)
{
  unsigned int groupId;
  unsigned int numDatasets = 0;
  unsigned int numGroups = hdInput->GetNumberOfGroups();
  for (groupId=0; groupId<numGroups; groupId++)
    {
    numDatasets += hdInput->GetNumberOfDataSets(groupId);
    }

  this->Internal->DataTypes.resize(numDatasets);
  unsigned int i = 0;
  for (groupId=0; groupId<numGroups; groupId++)
    {
    unsigned int numDataSets = hdInput->GetNumberOfDataSets(groupId);
    for (unsigned int dataSetId=0; dataSetId<numDataSets; dataSetId++)
      {
      vtkDataSet* ds = 
        vtkDataSet::SafeDownCast(hdInput->GetDataSet(groupId, dataSetId));
      if (ds)
        {
        this->Internal->DataTypes[i] = ds->GetDataObjectType();
        }
      else
        {
        this->Internal->DataTypes[i] = -1;
        }
      i++;
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::CreateWriters(vtkMultiGroupDataSet* hdInput)
{
  this->FillDataTypes(hdInput);

  unsigned int groupId;
  unsigned int numGroups = hdInput->GetNumberOfGroups();

  unsigned int numDatasets = this->Internal->DataTypes.size();
  this->Internal->Writers.resize(numDatasets);

  int i = 0;
  for (groupId=0; groupId<numGroups; groupId++)
    {
    unsigned int numDataSets = hdInput->GetNumberOfDataSets(groupId);
    for (unsigned int dataSetId=0; dataSetId<numDataSets; dataSetId++)
      {
      vtkDataSet* ds = 
        vtkDataSet::SafeDownCast(hdInput->GetDataSet(groupId, dataSetId));
      
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
      if(vtkXMLPDataWriter* w = 
         vtkXMLPDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer()))
        {
        w->SetStartPiece(this->Piece);
        w->SetEndPiece(this->Piece);
        w->SetNumberOfPieces(this->NumberOfPieces);
        w->SetGhostLevel(this->GhostLevel);
        if(this->WriteMetaFileInitialized)
          {
          w->SetWriteSummaryFile(this->WriteMetaFile);
          }
        else
          {
          w->SetWriteSummaryFile((this->Piece == 0)? 1:0);
          }
        }
      i++;
      }
    }
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLMultiGroupDataWriter::GetWriter(int index)
{
  int size = static_cast<int>(this->Internal->Writers.size());
  if(index >= 0 && index < size)
    {
    return this->Internal->Writers[index].GetPointer();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::SplitFileName()
{
  vtkstd::string fileName = this->FileName;
  vtkstd::string name;
  
  // Split the file name and extension from the path.
  vtkstd::string::size_type pos = fileName.find_last_of("/\\");
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
const char* vtkXMLMultiGroupDataWriter::GetFilePrefix()
{
  return this->Internal->FilePrefix.c_str();
}

//----------------------------------------------------------------------------
const char* vtkXMLMultiGroupDataWriter::GetFilePath()
{
  return this->Internal->FilePath.c_str();
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::ProgressCallbackFunction(vtkObject* caller,
                                                        unsigned long,
                                                        void* clientdata,
                                                        void*)
{
  vtkAlgorithm* w = vtkAlgorithm::SafeDownCast(caller);
  if(w)
    {
    reinterpret_cast<vtkXMLMultiGroupDataWriter*>(clientdata)->ProgressCallback(w);
    }
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::ProgressCallback(vtkAlgorithm* w)
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
void vtkXMLMultiGroupDataWriter::AppendEntry(const char* entry)
{
  this->Internal->Entries.push_back(entry);
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::DeleteAllEntries()
{
  this->Internal->Entries.clear();
}

//----------------------------------------------------------------------------
vtkstd::string vtkXMLMultiGroupDataWriterInternals::CreatePieceFileName(int index)
{
  vtkstd::string fname;
  ostrstream fn_with_warning_C4701;
  fn_with_warning_C4701
    << this->FilePrefix.c_str() << "/"
    << this->FilePrefix.c_str() << "_" << index << "."
    << this->Writers[index]->GetDefaultFileExtension() << ends;
  fname = fn_with_warning_C4701.str();
  fn_with_warning_C4701.rdbuf()->freeze(0);
  return fname;
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataWriter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  int size = static_cast<int>(this->Internal->Writers.size());
  for(int i=0; i < size; ++i)
    {
    vtkGarbageCollectorReport(collector, this->Internal->Writers[i], "Writer");
    }
}

//----------------------------------------------------------------------------
vtkExecutive* vtkXMLMultiGroupDataWriter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkMultiGroupDataSet");
  return 1;
}
