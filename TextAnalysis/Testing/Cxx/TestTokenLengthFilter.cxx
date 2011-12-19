/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTokenLengthFilter.cxx
  
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

#include <vtkArrayPrint.h>
#include <vtkDenseArray.h>
#include <vtkFoldCase.h>
#include <vtkIdTypeArray.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkFeatureDictionary.h>
#include <vtkTextExtraction.h>
#include <vtkTokenizer.h>
#include <vtkTokenLengthFilter.h>
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

int TestTokenLengthFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");
      
    vtkStringArray* const uri_array = vtkStringArray::New();
    uri_array->SetName("uri");

    vtkStringArray* const mime_type_array = vtkStringArray::New();
    mime_type_array->SetName("mime_type");

    vtkStringArray* const content_array = vtkStringArray::New();
    content_array->SetName("content");

    document_array->InsertNextValue(0);
    uri_array->InsertNextValue("data:text/plain");
    mime_type_array->InsertNextValue("text/plain");
    content_array->InsertNextValue("The quick brown fox jumped over the lazy dogs.");
   
    vtkSmartPointer<vtkTable> documents = vtkSmartPointer<vtkTable>::New();
    documents->AddColumn(document_array);
    documents->AddColumn(uri_array);
    documents->AddColumn(mime_type_array);
    documents->AddColumn(content_array);

    document_array->Delete();
    uri_array->Delete();
    mime_type_array->Delete();
    content_array->Delete();

    vtkSmartPointer<vtkTextExtraction> text_extraction = vtkSmartPointer<vtkTextExtraction>::New();
    text_extraction->SetInputConnection(0, documents->GetProducerPort());

    vtkSmartPointer<vtkTokenizer> tokenizer = vtkSmartPointer<vtkTokenizer>::New();
    tokenizer->SetInputConnection(0, text_extraction->GetOutputPort());
    tokenizer->AddDroppedDelimiters(vtkTokenizer::Whitespace());
    tokenizer->AddKeptDelimiters(vtkTokenizer::Punctuation());

    vtkSmartPointer<vtkTokenLengthFilter> token_length = vtkSmartPointer<vtkTokenLengthFilter>::New();
    token_length->SetInputConnection(0, tokenizer->GetOutputPort());
    token_length->SetBegin(0);
    token_length->SetEnd(4);
  
    token_length->Update();
    vtkUnicodeStringArray* const tokens = vtkUnicodeStringArray::SafeDownCast(token_length->GetOutput()->GetColumnByName("text"));

    for(vtkIdType i = 0; i != tokens->GetNumberOfTuples(); ++i)
      cerr << tokens->GetValue(i).utf8_str() << endl;

    test_expression(tokens->GetNumberOfTuples() == 6);
    test_expression(tokens->GetValue(0) == vtkUnicodeString::from_utf8("quick"));
    test_expression(tokens->GetValue(5) == vtkUnicodeString::from_utf8("dogs"));
 
    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

