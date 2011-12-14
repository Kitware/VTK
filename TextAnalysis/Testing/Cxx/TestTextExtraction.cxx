/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTextExtraction.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkAssignMimeType.h>
#include <vtkIdTypeArray.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTextExtraction.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

int TestTextExtraction(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Create sample data ...
    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");

    vtkStringArray* const uri_array = vtkStringArray::New();
    uri_array->SetName("uri");
      
    vtkStringArray* const content_array = vtkStringArray::New();
    content_array->SetName("content");

    document_array->InsertNextValue(0);
    document_array->InsertNextValue(1);
    document_array->InsertNextValue(2);
    document_array->InsertNextValue(3);

    uri_array->InsertNextValue("file:///home/bob/a.foo");
    uri_array->InsertNextValue("file:///home/bob/b.txt");
    uri_array->InsertNextValue("file:///home/bob/c.pdf");
    uri_array->InsertNextValue("file:///home/bob/d.doc");
    
    content_array->InsertNextValue("Howdy, world!");
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
   
    vtkSmartPointer<vtkTable> documents = vtkSmartPointer<vtkTable>::New();
    documents->AddColumn(document_array);
    documents->AddColumn(uri_array);
    documents->AddColumn(content_array);

    document_array->Delete();
    uri_array->Delete();
    content_array->Delete();

    documents->Dump(30);

    // Make it happen ...
    vtkSmartPointer<vtkAssignMimeType> assign_mime_type = vtkSmartPointer<vtkAssignMimeType>::New();
    assign_mime_type->SetInputConnection(0, documents->GetProducerPort());

    vtkSmartPointer<vtkTextExtraction> text_extraction = vtkSmartPointer<vtkTextExtraction>::New();
    text_extraction->SetInputConnection(0, assign_mime_type->GetOutputPort(0));

    text_extraction->Update();
    text_extraction->GetOutput(0)->Dump(30);
    text_extraction->GetOutput(1)->Dump(30);

    vtkTable* const table = text_extraction->GetOutput(0);
    test_expression(table->GetNumberOfRows() == 4);
    test_expression(table->GetValueByName(0, "mime_type").ToString() == "");
    test_expression(table->GetValueByName(1, "mime_type").ToString() == "text/plain");
    test_expression(table->GetValueByName(2, "mime_type").ToString() == "application/pdf");
    test_expression(table->GetValueByName(3, "mime_type").ToString() == "application/msword");
    test_expression(table->GetValueByName(0, "text").ToString() == "");
    test_expression(table->GetValueByName(1, "text").ToString() == "The quick brown fox jumped over the lazy dogs.");

    vtkTable* const tag_table = text_extraction->GetOutput(1);
    test_expression(tag_table->GetNumberOfRows() == 1);
    test_expression(tag_table->GetValueByName(0, "document").ToInt() == 1);
    test_expression(tag_table->GetValueByName(0, "begin").ToInt() == 0);
    test_expression(tag_table->GetValueByName(0, "end").ToInt() == 46);
    test_expression(tag_table->GetValueByName(0, "type").ToString() == "TEXT");

    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

