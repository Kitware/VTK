/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiGroupDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLMultiGroupDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkXMLMultiGroupDataReader, "1.2");
vtkStandardNewMacro(vtkXMLMultiGroupDataReader);

struct vtkXMLMultiGroupDataReaderEntry
{
  const char* extension;
  const char* name;
};

struct vtkXMLMultiGroupDataReaderInternals
{
  vtkstd::vector<vtkXMLDataElement*> DataSets;
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkXMLReader> > ReadersType;
  ReadersType Readers;
  static const vtkXMLMultiGroupDataReaderEntry ReaderList[];
};

//----------------------------------------------------------------------------
vtkXMLMultiGroupDataReader::vtkXMLMultiGroupDataReader()
{
  this->Internal = new vtkXMLMultiGroupDataReaderInternals;
}

//----------------------------------------------------------------------------
vtkXMLMultiGroupDataReader::~vtkXMLMultiGroupDataReader()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkXMLMultiGroupDataReader::GetDataSetName()
{
  return "vtkMultiGroupDataSet";
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataReader::SetupEmptyOutput()
{
  vtkExecutive* exec = this->GetExecutive();
  vtkInformation* info = exec->GetOutputInformation(0);

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkMultiGroupDataSet* hb = 
    vtkMultiGroupDataSet::SafeDownCast(doOutput);
  if (!hb)
    {
    return;
    }
  hb->Initialize();
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkMultiGroupDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkXMLMultiGroupDataReader::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
vtkMultiGroupDataSet* vtkXMLMultiGroupDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkMultiGroupDataSet* vtkXMLMultiGroupDataReader::GetOutput(int port)
{
  vtkDataObject* output = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkMultiGroupDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }

  int numNested = ePrimary->GetNumberOfNestedElements();
  int i;
  this->Internal->DataSets.clear();
  for(i=0; i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "DataSet") == 0) 
      { 
      this->Internal->DataSets.push_back(eNested);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkXMLReader* vtkXMLMultiGroupDataReader::GetReaderOfType(const char* type)
{
  vtkXMLMultiGroupDataReaderInternals::ReadersType::iterator iter =
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
    this->Internal->Readers[type] = reader;
    reader->Delete();
    }
  return reader;
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataReader::ReadXMLData()
{
  vtkExecutive* exec = this->GetExecutive();
  vtkInformation* info = exec->GetOutputInformation(0);

  unsigned int updatePiece = static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  unsigned int updateNumPieces =  static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkMultiGroupDataSet* hb = 
    vtkMultiGroupDataSet::SafeDownCast(doOutput);
  if (!hb)
    {
    return;
    }

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  vtkstd::string filePath = this->FileName;
  vtkstd::string::size_type pos = filePath.find_last_of("/\\");
  if(pos != filePath.npos)
    {
    filePath = filePath.substr(0, pos);
    }
  else
    {
    filePath = "";
    }

  vtkstd::vector<vtkXMLDataElement*>::iterator d;

  vtkstd::vector<unsigned int> numDataSets;

  for(d=this->Internal->DataSets.begin();
      d != this->Internal->DataSets.end(); ++d)
    {
    vtkXMLDataElement* ds = *d;

    int group = 0;
    int dsId = 0;

    if (!ds->GetScalarAttribute("dataset", dsId))
      {
      continue;
      }

    int groupRead;
    if (ds->GetScalarAttribute("group", groupRead))
      {
      group = groupRead;
      }

    if (group >= static_cast<int>(numDataSets.size()))
      {
      numDataSets.resize(group+1);
      numDataSets[group] = 0;
      }
    if (dsId >= static_cast<int>(numDataSets[group]))
      {
      numDataSets[group] = dsId + 1;
      }
    }

  for (unsigned int i=0; i<numDataSets.size(); i++)
    {
    hb->SetNumberOfDataSets(i, numDataSets[i]);
    }

  for(d=this->Internal->DataSets.begin();
      d != this->Internal->DataSets.end(); ++d)
    {
    vtkXMLDataElement* ds = *d;

    int group = 0;
    int dsId = 0;

    if (!ds->GetScalarAttribute("dataset", dsId))
      {
      continue;
      }

    int groupRead;
    if (ds->GetScalarAttribute("group", groupRead))
      {
      group = groupRead;
      }

    unsigned int numDatasets = hb->GetNumberOfDataSets(group);
    unsigned int numDatasetsPerPiece = 1;
    if (updateNumPieces < numDatasets)
      {
      numDatasetsPerPiece = numDatasets / updateNumPieces;
      }
    int minDataset = numDatasetsPerPiece*updatePiece;
    int maxDataset = numDatasetsPerPiece*(updatePiece+1);
    if (updatePiece == updateNumPieces - 1)
      {
      maxDataset = numDatasets;
      }

    vtkDataSet* outputCopy = 0;

    if (dsId >= minDataset && dsId < maxDataset)
      {
      
      // Construct the name of the internal file.
      vtkstd::string fileName;
      const char* file = ds->GetAttribute("file");
      if(!(file[0] == '/' || file[1] == ':'))
        {
        fileName = filePath;
        if(fileName.length())
          {
          fileName += "/";
          }
        }
      fileName += file;
      
      // Get the file extension.
      vtkstd::string ext;
      vtkstd::string::size_type pos2 = fileName.rfind('.');
      if(pos2 != fileName.npos)
        {
        ext = fileName.substr(pos2+1);
        }
      
      // Search for the reader matching this extension.
      const char* rname = 0;
      for(const vtkXMLMultiGroupDataReaderEntry* r = 
            this->Internal->ReaderList;
          !rname && r->extension; ++r)
        {
        if(ext == r->extension)
          {
          rname = r->name;
          }
        }
      vtkXMLReader* reader = this->GetReaderOfType(rname);
      if (!reader)
        {
        vtkErrorMacro("Could not create reader for " << rname);
        continue;
        }
      reader->SetFileName(fileName.c_str());
      reader->Update();
      vtkDataSet* output = reader->GetOutputAsDataSet();
      if (!output)
        {
        continue;
        }
      outputCopy = output->NewInstance();
      outputCopy->ShallowCopy(output);
      output->Initialize();
      }
    this->HandleDataSet(ds, group, dsId, hb, outputCopy);
    if (outputCopy)
      {
      outputCopy->Delete();
      }
    }
}

//----------------------------------------------------------------------------
int vtkXMLMultiGroupDataReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLMultiGroupDataReader::HandleDataSet(vtkXMLDataElement*,
                                               int group, int dsId, 
                                               vtkMultiGroupDataSet* output,
                                               vtkDataSet* data)
{
  output->SetDataSet(group, dsId, data);
}

//----------------------------------------------------------------------------
const vtkXMLMultiGroupDataReaderEntry
vtkXMLMultiGroupDataReaderInternals::ReaderList[] =
{
  {"vtp", "vtkXMLPolyDataReader"},
  {"vtu", "vtkXMLUnstructuredGridReader"},
  {"vti", "vtkXMLImageDataReader"},
  {"vtr", "vtkXMLRectilinearGridReader"},
  {"vts", "vtkXMLStructuredGridReader"},
  {0, 0}
};
