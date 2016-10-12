/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLCompositeDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLCompositeDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkEventForwarderCommand.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

struct vtkXMLCompositeDataReaderEntry
{
  const char* extension;
  const char* name;
};

struct vtkXMLCompositeDataReaderInternals
{
  vtkSmartPointer<vtkXMLDataElement> Root;
  typedef std::map<std::string, vtkSmartPointer<vtkXMLReader> > ReadersType;
  ReadersType Readers;
  static const vtkXMLCompositeDataReaderEntry ReaderList[];
  unsigned int MinDataset;
  unsigned int MaxDataset;
  vtkXMLCompositeDataReaderInternals()
  {
    this->MinDataset = 0;
    this->MaxDataset = 0;
  }
  std::set<int> UpdateIndices;
  bool HasUpdateRestriction;
};

//----------------------------------------------------------------------------
vtkXMLCompositeDataReader::vtkXMLCompositeDataReader()
{
  this->Internal = new vtkXMLCompositeDataReaderInternals;
}

//----------------------------------------------------------------------------
vtkXMLCompositeDataReader::~vtkXMLCompositeDataReader()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkXMLCompositeDataReader::GetDataSetName()
{
  return "vtkCompositeDataSet";
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkXMLCompositeDataReader::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkXMLCompositeDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkCompositeDataSet* vtkXMLCompositeDataReader::GetOutput(int port)
{
  vtkDataObject* output =
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkCompositeDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Simply save the XML tree. We'll iterate over it later.
  this->Internal->Root = ePrimary;
  return 1;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLCompositeDataReader::GetPrimaryElement()
{
  return this->Internal->Root;
}

//----------------------------------------------------------------------------
vtkXMLReader* vtkXMLCompositeDataReader::GetReaderOfType(const char* type)
{
  if (!type)
  {
    return 0;
  }

  vtkXMLCompositeDataReaderInternals::ReadersType::iterator iter =
    this->Internal->Readers.find(type);
  if (iter != this->Internal->Readers.end())
  {
    return iter->second.GetPointer();
  }

  vtkXMLReader* reader = 0;
  if (strcmp(type, "vtkXMLImageDataReader") == 0)
  {
    reader = vtkXMLImageDataReader::New();
  }
  else if (strcmp(type,"vtkXMLUnstructuredGridReader") == 0)
  {
    reader = vtkXMLUnstructuredGridReader::New();
  }
  else if (strcmp(type,"vtkXMLPolyDataReader") == 0)
  {
    reader = vtkXMLPolyDataReader::New();
  }
  else if (strcmp(type,"vtkXMLRectilinearGridReader") == 0)
  {
    reader = vtkXMLRectilinearGridReader::New();
  }
  else if (strcmp(type,"vtkXMLStructuredGridReader") == 0)
  {
    reader = vtkXMLStructuredGridReader::New();
  }
  if (!reader)
  {
    // If all fails, Use the instantiator to create the reader.
    reader = vtkXMLReader::SafeDownCast(vtkInstantiator::CreateInstance(type));
  }
  if (reader)
  {
    if (this->GetParserErrorObserver())
    {
      reader->SetParserErrorObserver(this->GetParserErrorObserver());
    }
    if (this->HasObserver("ErrorEvent"))
    {
      vtkNew<vtkEventForwarderCommand> fwd;
      fwd->SetTarget(this);
      reader->AddObserver("ErrorEvent", fwd.GetPointer());
    }
    this->Internal->Readers[type] = reader;
    reader->Delete();
  }
  return reader;
}

//----------------------------------------------------------------------------
unsigned int vtkXMLCompositeDataReader::CountLeaves(vtkXMLDataElement* elem)
{
  unsigned int count = 0;
  if (elem)
  {
    unsigned int max = elem->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < max; ++cc)
    {
      vtkXMLDataElement* child = elem->GetNestedElement(cc);
      if (child && child->GetName())
      {
        if (strcmp(child->GetName(), "DataSet")==0)
        {
          count++;
        }
        else
        {
          count += this->CountLeaves(child);
        }
      }
    }
  }
  return count;
}

//----------------------------------------------------------------------------
void vtkXMLCompositeDataReader::ReadXMLData()
{
  vtkInformation* info = this->GetCurrentOutputInformation();

  unsigned int updatePiece = static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  unsigned int updateNumPieces =  static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));

  vtkDataObject* doOutput =
    info->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* composite =
    vtkCompositeDataSet::SafeDownCast(doOutput);
  if (!composite)
  {
    return;
  }

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->FileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != filePath.npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }

  // In earlier implementation only dataset with a group were distributed among
  // the processes. In this implementation, we distribute all leaf datasets
  // among the processes.

  // Determine the leaves that this process is going to read.
  unsigned int numDatasets = this->CountLeaves(this->GetPrimaryElement());
  unsigned int numDatasetsPerPiece = 1;

  unsigned int remaining_blocks = 0;
  if (updateNumPieces < numDatasets)
  {
    numDatasetsPerPiece = numDatasets / updateNumPieces;
    remaining_blocks = numDatasets % updateNumPieces;
  }

  if (updatePiece < remaining_blocks)
  {
    this->Internal->MinDataset = (numDatasetsPerPiece+1)*updatePiece;
    this->Internal->MaxDataset = this->Internal->MinDataset + numDatasetsPerPiece + 1;
  }
  else
  {
    this->Internal->MinDataset = (numDatasetsPerPiece +1)* remaining_blocks +
      numDatasetsPerPiece * (updatePiece-remaining_blocks);
    this->Internal->MaxDataset = this->Internal->MinDataset + numDatasetsPerPiece;
  }

  vtkInformation* outInfo = this->GetCurrentOutputInformation();
  if (outInfo->Has(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES()))
  {
    this->Internal->HasUpdateRestriction = true;
    this->Internal->UpdateIndices = std::set<int>();
    int length = outInfo->Length(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    if (length > 0)
    {
      int* idx = outInfo->Get(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
      this->Internal->UpdateIndices = std::set<int>(idx, idx+length);
    }
  }
  else
  {
    this->Internal->HasUpdateRestriction = false;
  }

  // All process create the  entire tree structure however, only each one only
  // reads the datasets assigned to it.
  unsigned int dataSetIndex=0;
  this->ReadComposite(this->GetPrimaryElement(), composite, filePath.c_str(), dataSetIndex);
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::ShouldReadDataSet(unsigned int dataSetIndex)
{
  bool shouldRead =
    (dataSetIndex >= this->Internal->MinDataset &&
     dataSetIndex < this->Internal->MaxDataset);

  if (shouldRead && this->Internal->HasUpdateRestriction)
  {
    if (this->Internal->UpdateIndices.find(dataSetIndex) ==
        this->Internal->UpdateIndices.end())
    {
      shouldRead = false;
    }
  }
  return shouldRead;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLCompositeDataReader::ReadDataset(vtkXMLDataElement* xmlElem,
  const char* filePath)
{
  // Construct the name of the internal file.
  const char* file = xmlElem->GetAttribute("file");
  if (!file)
  {
    return 0;
  }

  std::string fileName;
  if (!(file[0] == '/' || file[1] == ':'))
  {
    fileName = filePath;
    if (fileName.length())
    {
      fileName += "/";
    }
  }
  fileName += file;

  // Get the file extension.
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(fileName);
  if (ext.size() > 0)
  {
    // remote "." from the extension.
    ext = &(ext.c_str()[1]);
  }

  // Search for the reader matching this extension.
  const char* rname = 0;
  for(const vtkXMLCompositeDataReaderEntry* readerEntry =
    this->Internal->ReaderList;
    !rname && readerEntry->extension; ++readerEntry)
  {
    if (ext == readerEntry->extension)
    {
      rname = readerEntry->name;
    }
  }
  vtkXMLReader* reader = this->GetReaderOfType(rname);
  if (!reader)
  {
    vtkErrorMacro("Could not create reader for " << rname);
    return 0;
  }
  reader->SetFileName(fileName.c_str());
  // initialize array selection so we don't have any residual array selections
  // from previous use of the reader.
  reader->GetPointDataArraySelection()->RemoveAllArrays();
  reader->GetCellDataArraySelection()->RemoveAllArrays();
  reader->Update();
  vtkDataSet* output = reader->GetOutputAsDataSet();
  if (!output)
  {
    return 0;
  }

  vtkDataSet* outputCopy = output->NewInstance();
  outputCopy->ShallowCopy(output);
  return outputCopy;
}

//----------------------------------------------------------------------------
int vtkXMLCompositeDataReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  vtkInformation* info = outputVector->GetInformationObject(0);
   info->Set(
     CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
const vtkXMLCompositeDataReaderEntry
  vtkXMLCompositeDataReaderInternals::ReaderList[] =
{
  {"vtp", "vtkXMLPolyDataReader"},
  {"vtu", "vtkXMLUnstructuredGridReader"},
  {"vti", "vtkXMLImageDataReader"},
  {"vtr", "vtkXMLRectilinearGridReader"},
  {"vts", "vtkXMLStructuredGridReader"},
  {0, 0}
};
