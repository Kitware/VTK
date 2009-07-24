/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNGramExtraction.cxx
  
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
#include <vtkNGramExtraction.h>
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

int TestNGramExtraction(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
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

    // Setup the pipeline ...
    vtkSmartPointer<vtkTokenizer> tokenizer = vtkSmartPointer<vtkTokenizer>::New();
    tokenizer->SetInputConnection(0, documents->GetProducerPort());
    tokenizer->AddDroppedDelimiters(vtkTokenizer::Whitespace());
    tokenizer->AddKeptDelimiters(vtkTokenizer::Punctuation());

    vtkSmartPointer<vtkNGramExtraction> ngram_extraction = vtkSmartPointer<vtkNGramExtraction>::New();
    ngram_extraction->SetInputConnection(0, tokenizer->GetOutputPort());
   
    // Test unigram extraction ... 
    ngram_extraction->SetN(1);
    ngram_extraction->Update();
    ngram_extraction->GetOutput()->Dump(20);

    test_expression(ngram_extraction->GetOutput()->GetValueByName(0, "type").ToString() == "1-gram");
    vtkUnicodeStringArray* const unigrams = vtkUnicodeStringArray::SafeDownCast(ngram_extraction->GetOutput()->GetColumnByName("text"));
    test_expression(unigrams->GetNumberOfTuples() == 10);
    test_expression(unigrams->GetValue(0) == vtkUnicodeString::from_utf8("The"));
    test_expression(unigrams->GetValue(9) == vtkUnicodeString::from_utf8("."));

    // Test bigram extraction ...
    ngram_extraction->SetN(2);
    ngram_extraction->Update();
    ngram_extraction->GetOutput()->Dump(20);

    test_expression(ngram_extraction->GetOutput()->GetValueByName(0, "type").ToString() == "2-gram");
    vtkUnicodeStringArray* const bigrams = vtkUnicodeStringArray::SafeDownCast(ngram_extraction->GetOutput()->GetColumnByName("text"));
    test_expression(bigrams->GetNumberOfTuples() == 9);
    test_expression(bigrams->GetValue(0) == vtkUnicodeString::from_utf8("The quick"));
    test_expression(bigrams->GetValue(8) == vtkUnicodeString::from_utf8("dogs ."));
 
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

