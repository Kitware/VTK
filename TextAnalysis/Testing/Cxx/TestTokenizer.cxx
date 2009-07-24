/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTokenizer.cxx
  
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

#include <vtkIdTypeArray.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkFeatureDictionary.h>
#include <vtkTokenizer.h>
#include <vtkUnicodeStringArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtkstd::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

int TestTokenizer(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Create sample data ...
    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");
      
    vtkUnicodeStringArray* const text_array = vtkUnicodeStringArray::New();
    text_array->SetName("text");

    document_array->InsertNextValue(0);
    text_array->InsertNextValue(vtkUnicodeString::from_utf8("The quick brown fox jumped over the lazy dogs."));
   
    vtkSmartPointer<vtkTable> documents = vtkSmartPointer<vtkTable>::New();
    documents->AddColumn(document_array);
    documents->AddColumn(text_array);

    document_array->Delete();
    text_array->Delete();

    documents->Dump(20);

    // Make it happen ...
    vtkSmartPointer<vtkTokenizer> tokenizer = vtkSmartPointer<vtkTokenizer>::New();
    tokenizer->SetInputConnection(0, documents->GetProducerPort());
    tokenizer->AddDroppedDelimiters(vtkTokenizer::Whitespace());
    tokenizer->AddKeptDelimiters(vtkTokenizer::Punctuation());

    tokenizer->Update();
    tokenizer->GetOutput()->Dump(20);

    vtkUnicodeStringArray* const tokens = vtkUnicodeStringArray::SafeDownCast(tokenizer->GetOutput()->GetColumnByName("text"));
    test_expression(tokens->GetNumberOfTuples() == 10);
    test_expression(tokens->GetValue(0) == vtkUnicodeString::from_utf8("The"));
    test_expression(tokens->GetValue(9) == vtkUnicodeString::from_utf8("."));
 
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

