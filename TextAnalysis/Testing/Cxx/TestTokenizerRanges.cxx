/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTokenizerRanges.cxx
  
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
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

int TestTokenizerRanges(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
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

    // Create some sample ranges ...
    vtkIdTypeArray* const range_document = vtkIdTypeArray::New();
    range_document->SetName("document");
      
    vtkIdTypeArray* const range_begin = vtkIdTypeArray::New();
    range_begin->SetName("begin");

    vtkIdTypeArray* const range_end = vtkIdTypeArray::New();
    range_end->SetName("end");

    range_document->InsertNextValue(0);
    range_begin->InsertNextValue(10);
    range_end->InsertNextValue(24);
   
    vtkSmartPointer<vtkTable> ranges = vtkSmartPointer<vtkTable>::New();
    ranges->AddColumn(range_document);
    ranges->AddColumn(range_begin);
    ranges->AddColumn(range_end);
    range_document->Delete();
    range_begin->Delete();
    range_end->Delete();

    ranges->Dump(20);

    // Make it happen ...
    vtkSmartPointer<vtkTokenizer> tokenizer = vtkSmartPointer<vtkTokenizer>::New();
    tokenizer->SetInputConnection(0, documents->GetProducerPort());
    tokenizer->SetInputConnection(1, ranges->GetProducerPort());
    tokenizer->AddDroppedDelimiters(vtkTokenizer::Whitespace());
    tokenizer->AddKeptDelimiters(vtkTokenizer::Punctuation());

    tokenizer->Update();
    tokenizer->GetOutput()->Dump(20);

    vtkUnicodeStringArray* const tokens = vtkUnicodeStringArray::SafeDownCast(tokenizer->GetOutput()->GetColumnByName("text"));
    test_expression(tokens->GetNumberOfTuples() == 3);
    test_expression(tokens->GetValue(0) == vtkUnicodeString::from_utf8("brown"));
    test_expression(tokens->GetValue(2) == vtkUnicodeString::from_utf8("jump"));
 
    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

