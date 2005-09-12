/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLHierarchicalDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLHierarchicalDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkInformation.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkXMLHierarchicalDataReader, "1.1");
vtkStandardNewMacro(vtkXMLHierarchicalDataReader);

struct vtkXMLHierarchicalDataReaderEntry
{
  const char* extension;
  const char* name;
};

struct vtkXMLHierarchicalDataReaderInternals
{
  vtkstd::vector<vtkXMLDataElement*> DataSets;
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkXMLReader> > ReadersType;
  ReadersType Readers;
  static const vtkXMLHierarchicalDataReaderEntry ReaderList[];
};

//----------------------------------------------------------------------------
vtkXMLHierarchicalDataReader::vtkXMLHierarchicalDataReader()
{
  this->Internal = new vtkXMLHierarchicalDataReaderInternals;
}

//----------------------------------------------------------------------------
vtkXMLHierarchicalDataReader::~vtkXMLHierarchicalDataReader()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkXMLHierarchicalDataReader::GetDataSetName()
{
  return "vtkHierarchicalDataSet";
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalDataReader::SetupEmptyOutput()
{
  vtkExecutive* exec = this->GetExecutive();
  vtkInformation* info = exec->GetOutputInformation(0);

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalDataSet* hb = 
    vtkHierarchicalDataSet::SafeDownCast(doOutput);
  if (!hb)
    {
    return;
    }
  hb->Initialize();
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalDataReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkHierarchicalDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkXMLHierarchicalDataReader::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet* vtkXMLHierarchicalDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet* vtkXMLHierarchicalDataReader::GetOutput(int port)
{
  vtkDataObject* output = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->
    GetCompositeOutputData(port);
  return vtkHierarchicalDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
int vtkXMLHierarchicalDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
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
vtkXMLReader* vtkXMLHierarchicalDataReader::GetReaderOfType(const char* type)
{
  vtkXMLHierarchicalDataReaderInternals::ReadersType::iterator iter =
    this->Internal->Readers.find(type);
  if (iter != this->Internal->Readers.end())
    {
    return iter->second.GetPointer();
    }
  // Use the instantiator to create the reader.
  vtkObject* o = vtkInstantiator::CreateInstance(type);
  vtkXMLReader* reader = vtkXMLReader::SafeDownCast(o);
  if (reader)
    {
    this->Internal->Readers[type] = reader;
    reader->Delete();
    }
  return reader;
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalDataReader::ReadXMLData()
{
  vtkExecutive* exec = this->GetExecutive();
  vtkInformation* info = exec->GetOutputInformation(0);

  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkHierarchicalDataSet* hb = 
    vtkHierarchicalDataSet::SafeDownCast(doOutput);
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
  for(d=this->Internal->DataSets.begin();
      d != this->Internal->DataSets.end(); ++d)
    {
    vtkXMLDataElement* ds = *d;

    int level = 0;
    int dsId = 0;

    if (!ds->GetScalarAttribute("block", dsId))
      {
      continue;
      }

    int levelRead;
    if (ds->GetScalarAttribute("level", levelRead))
      {
      level = levelRead;
      }

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
    vtkstd::string::size_type pos = fileName.rfind('.');
    if(pos != fileName.npos)
      {
      ext = fileName.substr(pos+1);
      }

    // Search for the reader matching this extension.
    const char* rname = 0;
    for(const vtkXMLHierarchicalDataReaderEntry* r = this->Internal->ReaderList;
        !rname && r->extension; ++r)
      {
      if(ext == r->extension)
        {
        rname = r->name;
        }
      }
    vtkXMLReader* reader = this->GetReaderOfType(rname);
    reader->SetFileName(fileName.c_str());
    reader->Update();
    vtkDataSet* output = reader->GetOutputAsDataSet();
    if (!output)
      {
      continue;
      }
    vtkDataSet* outputCopy = output->NewInstance();
    outputCopy->ShallowCopy(output);
    this->HandleBlock(ds, level, dsId, hb, outputCopy);
    output->Initialize();
    outputCopy->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalDataReader::HandleBlock(vtkXMLDataElement*,
                                               int level, int dsId, 
                                               vtkHierarchicalDataSet* output,
                                               vtkDataSet* data)
{
  output->SetDataSet(level, dsId, data);
}

//----------------------------------------------------------------------------
const vtkXMLHierarchicalDataReaderEntry
vtkXMLHierarchicalDataReaderInternals::ReaderList[] =
{
  {"vtp", "vtkXMLPolyDataReader"},
  {"vtu", "vtkXMLUnstructuredGridReader"},
  {"vti", "vtkXMLImageDataReader"},
  {"vtr", "vtkXMLRectilinearGridReader"},
  {"vts", "vtkXMLStructuredGridReader"},
  {0, 0}
};
