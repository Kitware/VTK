/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTermDictionary.cxx
  
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

#include <vtkFoldCase.h>
#include <vtkIdTypeArray.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
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

#if 0
static ostream& operator<<(ostream& stream, const vtkUnicodeString& value)
{
  stream << value.utf8_str();
  return stream;
}
#endif

int TestTermDictionary(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    // Setup sample data ...
    vtkIdTypeArray* const document_array = vtkIdTypeArray::New();
    document_array->SetName("document");
      
    vtkUnicodeStringArray* const text_array = vtkUnicodeStringArray::New();
    text_array->SetName("text");

    document_array->InsertNextValue(0);
    text_array->InsertNextValue(vtkUnicodeString::from_utf8("The quick brown fox jumped over the lazy dogs."));
   
    document_array->InsertNextValue(1);
    text_array->InsertNextValue(vtkUnicodeString::from_utf8("The rain in Spain falls mainly on the plain."));
  
    vtkSmartPointer<vtkTable> documents = vtkSmartPointer<vtkTable>::New();
    documents->AddColumn(document_array);
    documents->AddColumn(text_array);

    document_array->Delete();
    text_array->Delete();

    documents->Dump(20);
 
    // Run it through to vtkFeatureDictionary ...
    vtkSmartPointer<vtkTokenizer> tokenizer = vtkSmartPointer<vtkTokenizer>::New();
    tokenizer->SetInputConnection(0, documents->GetProducerPort());
    tokenizer->AddDroppedDelimiters(vtkTokenizer::Whitespace());
    tokenizer->AddKeptDelimiters(vtkTokenizer::Punctuation());

    vtkSmartPointer<vtkFoldCase> fold_case = vtkSmartPointer<vtkFoldCase>::New();
    fold_case->SetInputConnection(0, tokenizer->GetOutputPort());

    vtkSmartPointer<vtkFeatureDictionary> term_dictionary = vtkSmartPointer<vtkFeatureDictionary>::New();
    term_dictionary->SetInputConnection(0, fold_case->GetOutputPort());

    // Test the results ...
    term_dictionary->Update();
    term_dictionary->GetOutput()->Dump(20);

    vtkStringArray* const dictionary_type_array = vtkStringArray::SafeDownCast(term_dictionary->GetOutput()->GetColumnByName("type"));
    test_expression(dictionary_type_array);

    vtkUnicodeStringArray* const dictionary_text_array = vtkUnicodeStringArray::SafeDownCast(term_dictionary->GetOutput()->GetColumnByName("text"));
    test_expression(dictionary_text_array);

    test_expression(dictionary_text_array->GetNumberOfTuples() == 16);
    test_expression(dictionary_type_array->GetValue(0) == "token");
    test_expression(dictionary_text_array->GetValue(0) == vtkUnicodeString::from_utf8("the"));
    test_expression(dictionary_text_array->GetValue(1) == vtkUnicodeString::from_utf8("quick"));
    test_expression(dictionary_text_array->GetValue(15) == vtkUnicodeString::from_utf8("plain"));
   
    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

