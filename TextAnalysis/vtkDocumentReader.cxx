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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

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

#include <vtkstd/algorithm>
#include <vtkstd/iterator>
#include <vtkstd/stdexcept>
#include <vtkstd/string>
#include <vtkstd/vector>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <vtksys/ios/sstream>

class vtkDocumentReader::Internals
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

  vtkstd::vector<vtkStdString> Files;
  vtkstd::vector<vtkIdType> ID;
};

vtkCxxRevisionMacro(vtkDocumentReader, "1.2");
vtkStandardNewMacro(vtkDocumentReader);

vtkDocumentReader::vtkDocumentReader() :
  Implementation(new Internals()),
  DefaultMimeType(0)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
}

vtkDocumentReader::~vtkDocumentReader()
{
  this->SetDefaultMimeType(0);
  delete this->Implementation;
}

void vtkDocumentReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "DefaultMimeType: " << (this->DefaultMimeType ? this->DefaultMimeType : "(none)") << "\n";
 
  for(vtkIdType i = 0; static_cast<unsigned int>(i) != this->Implementation->Files.size(); ++i)
    {
    os << indent << "File: " << this->Implementation->Files[i] << "\n";
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
  this->AddFile(file, this->Implementation->Files.size());
}

void vtkDocumentReader::AddFile(const vtkStdString& file, const vtkIdType id)
{
  this->Implementation->Files.push_back(file);
  this->Implementation->ID.push_back(id);
  this->Modified();
}

void vtkDocumentReader::ClearFiles()
{
  this->Implementation->Files.clear();
  this->Implementation->ID.clear();
  this->Modified();
}

int vtkDocumentReader::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  try
    {
    vtkSmartPointer<vtkMimeTypes> mime_types = vtkSmartPointer<vtkMimeTypes>::New();
 
    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");
  
    vtkStringArray* const uri_array = vtkStringArray::New();
    uri_array->SetName("uri");

    vtkStringArray* const mime_type_array = vtkStringArray::New();
    mime_type_array->SetName("mime_type");

    vtkStringArray* const content_array = vtkStringArray::New();
    content_array->SetName("content");

    int number_of_files = this->Implementation->Files.size();
    for(vtkIdType i = 0; static_cast<unsigned int>(i) != this->Implementation->Files.size(); ++i)
      {
      const vtkStdString file = this->Implementation->Files[i];
      const vtkIdType document = this->Implementation->ID[i];
      const vtkStdString uri = Internals::PathToURI(file);
      vtkStdString mime_type = mime_types->Lookup(file);

      if(mime_type.empty() && this->DefaultMimeType)
        mime_type = this->DefaultMimeType;

      ifstream file_stream(file.c_str());
      vtkstd::stringstream contents;
      contents << file_stream.rdbuf();

      document_array->InsertNextValue(document);
      uri_array->InsertNextValue(uri);
      mime_type_array->InsertNextValue(mime_type);
      content_array->InsertNextValue(contents.str());
      
      //emit event progress...
      double progress = static_cast<double>(i) / static_cast<double>(number_of_files);
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
      }
      
    vtkTable* const output_table = vtkTable::GetData(outputVector, 0);
    output_table->AddColumn(document_array);
    output_table->AddColumn(uri_array);
    output_table->AddColumn(mime_type_array);
    output_table->AddColumn(content_array);
    output_table->GetRowData()->SetPedigreeIds(document_array);
    
    document_array->Delete();
    uri_array->Delete();
    mime_type_array->Delete();
    content_array->Delete();
    }
  catch(vtkstd::exception& e)
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

