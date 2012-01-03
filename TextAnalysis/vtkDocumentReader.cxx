/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDocumentReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkArrayData.h>
#include <vtkCommand.h>
#include <vtkDataSetAttributes.h>
#include <vtkDenseArray.h>
#include <vtkDocumentReader.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMimeTypes.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>

class vtkDocumentReader::Implementation
{
public:
  // Converts a filesystem path to a URI
  static const vtkStdString PathToURI(const vtkStdString& path)
  {
    vtkStdString result = path;

    // Get rid of leading and trailing whitespace ...
    boost::trim(result);
    // Make Windoze slashes straighten-up and fly right ...
    boost::replace_all(result, "\\", "/");
    // Ensure that Windoze paths are absolute paths ...
    if(result.size() > 1 && result.at(1) == ':')
      result = "/" + result;

    result = "file://" + result;

    return result;
  }

  std::vector<vtkStdString> Files;
  std::vector<vtkIdType> ID;
};

vtkStandardNewMacro(vtkDocumentReader);

vtkDocumentReader::vtkDocumentReader() :
  Internal(new Implementation())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

vtkDocumentReader::~vtkDocumentReader()
{
  delete this->Internal;
}

void vtkDocumentReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  for(vtkIdType i = 0; static_cast<unsigned int>(i) != this->Internal->Files.size(); ++i)
    {
    os << indent << "File: " << this->Internal->Files[i] << "\n";
    }
}

void vtkDocumentReader::AddFile(const char* file)
{
  if(!file)
    return;

  this->AddFile(vtkStdString(file));
}

void vtkDocumentReader::AddFile(const vtkStdString& file)
{
  this->AddFile(file, this->Internal->Files.size());
}

void vtkDocumentReader::AddFile(const vtkStdString& file, const vtkIdType id)
{
  this->Internal->Files.push_back(file);
  this->Internal->ID.push_back(id);
  this->Modified();
}

void vtkDocumentReader::ClearFiles()
{
  this->Internal->Files.clear();
  this->Internal->ID.clear();
  this->Modified();
}

int vtkDocumentReader::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  try
    {
    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");
  
    vtkStringArray* const uri_array = vtkStringArray::New();
    uri_array->SetName("uri");

    vtkStringArray* const content_array = vtkStringArray::New();
    content_array->SetName("content");

    size_t number_of_files = this->Internal->Files.size();
    for(size_t i = 0; i != this->Internal->Files.size(); ++i)
      {
      const vtkStdString file = this->Internal->Files[i];
      const vtkIdType document = this->Internal->ID[i];
      const vtkStdString uri = Implementation::PathToURI(file);

      ifstream file_stream(file.c_str(), ios::in | ios::binary);
      std::stringstream contents;
      contents << file_stream.rdbuf();

      document_array->InsertNextValue(document);
      uri_array->InsertNextValue(uri);
      content_array->InsertNextValue(contents.str());
      
      //emit event progress...
      double progress = static_cast<double>(i) / static_cast<double>(number_of_files);
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
      }
      
    vtkTable* const output_table = vtkTable::GetData(outputVector, 0);
    output_table->AddColumn(document_array);
    output_table->AddColumn(uri_array);
    output_table->AddColumn(content_array);
    output_table->GetRowData()->SetPedigreeIds(document_array);
    
    document_array->Delete();
    uri_array->Delete();
    content_array->Delete();
    }
  catch(std::exception& e)
    {
    vtkErrorMacro(<< "unhandled exception: " << e.what());
    return 0;
    }
  catch(...)
    {
    vtkErrorMacro(<< "unknown exception");
    return 0;
    }

  return 1;
}

