/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAssignMimeType.cxx
  
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
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

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

int TestAssignMimeType(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Create sample data ...
    vtkStringArray* const uri_array = vtkStringArray::New();
    uri_array->SetName("uri");
      
    vtkStringArray* const content_array = vtkStringArray::New();
    content_array->SetName("content");

    uri_array->InsertNextValue("file:///home/bob/a.foo");
    uri_array->InsertNextValue("file:///home/bob/b.txt");
    uri_array->InsertNextValue("file:///home/bob/c.pdf");
    uri_array->InsertNextValue("file:///home/bob/d.doc");
    
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
   
    vtkSmartPointer<vtkTable> documents = vtkSmartPointer<vtkTable>::New();
    documents->AddColumn(uri_array);
    documents->AddColumn(content_array);

    uri_array->Delete();
    content_array->Delete();

    documents->Dump(30);

    // Make it happen ...
    vtkSmartPointer<vtkAssignMimeType> assign_mime_type = vtkSmartPointer<vtkAssignMimeType>::New();
    assign_mime_type->SetInputConnection(0, documents->GetProducerPort());

    assign_mime_type->Update();
    assign_mime_type->GetOutput()->Dump(20);

    vtkTable* const table = assign_mime_type->GetOutput();
    test_expression(table->GetNumberOfRows() == 4);
    test_expression(table->GetValueByName(0, "mime_type").ToString() == "");
    test_expression(table->GetValueByName(1, "mime_type").ToString() == "text/plain");
    test_expression(table->GetValueByName(2, "mime_type").ToString() == "application/pdf");
    test_expression(table->GetValueByName(3, "mime_type").ToString() == "application/msword");
 
    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

