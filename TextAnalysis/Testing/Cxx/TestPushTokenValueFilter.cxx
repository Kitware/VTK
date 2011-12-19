/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPushTokenValueFilter.cxx
  
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2008, 2009 by SCI Institute, University of Utah.
  
  This is part of the Parallel Dataflow System originally developed by
  Huy T. Vo and Claudio T. Silva. For more information, see:

  "Parallel Dataflow Scheme for Streaming (Un)Structured Data" by Huy
  T. Vo, Daniel K. Osmari, Brian Summa, Joao L.D. Comba, Valerio
  Pascucci and Claudio T. Silva, SCI Institute, University of Utah,
  Technical Report #UUSCI-2009-004, 2009.

  "Multi-Threaded Streaming Pipeline For VTK" by Huy T. Vo and Claudio
  T. Silva, SCI Institute, University of Utah, Technical Report
  #UUSCI-2009-005, 2009.
-------------------------------------------------------------------------*/

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
#include <vtkTokenValueFilter.h>
#include <vtkUnicodeStringArray.h>
#include <vtkThreadedStreamingPipeline.h>
#include <vtkExecutionScheduler.h>
#include <vtkInformation.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression)                                     \
  {                                                                     \
    if(!(expression))                                                   \
      {                                                                 \
        std::ostringstream buffer;                                   \
        buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
        throw std::runtime_error(buffer.str());                      \
      }                                                                 \
  }

class vtkStreamAwareFeatureDictionary : public vtkFeatureDictionary {
public:
  static vtkStreamAwareFeatureDictionary* New();
  vtkTypeMacro(vtkStreamAwareFeatureDictionary, vtkFeatureDictionary);
  
protected:
  vtkStreamAwareFeatureDictionary() {}
  ~vtkStreamAwareFeatureDictionary() {}
  
  int RequestData(vtkInformation*request, 
                  vtkInformationVector** inputVector, 
                  vtkInformationVector* outputVector)
  {
    vtkInformation *extra_info = vtkInformation::
      SafeDownCast(this->GetInformation()->Get(vtkThreadedStreamingPipeline::EXTRA_INFORMATION()));
    // Can process extra streaming information here
    if (extra_info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
      fprintf(stderr, "TOTAL NUMBER OF PIECES IS %d\n",
              extra_info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    this->Superclass::RequestData(request, inputVector, outputVector);
  }
  
private:
  vtkStreamAwareFeatureDictionary(const vtkStreamAwareFeatureDictionary&); // Not implemented
  void operator=(const vtkStreamAwareFeatureDictionary&);   // Not implemented
};

vtkStandardNewMacro(vtkStreamAwareFeatureDictionary);

int TestPushTokenValueFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
      vtkAlgorithm::SetDefaultExecutivePrototype(vtkThreadedStreamingPipeline::New());
      vtkThreadedStreamingPipeline::SetAutoPropagatePush(true);
      
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
      char buffer[1024] = {};
      for (int i=0; i<7; i++) {
        char number[20];
        sprintf(number, "%d ", rand());
        strcat(buffer, number);
      }
      content_array->InsertNextValue(buffer);
    
    
      vtkSmartPointer<vtkTable> documents = vtkSmartPointer<vtkTable>::New();
      documents->AddColumn(document_array);
      documents->AddColumn(uri_array);
      documents->AddColumn(mime_type_array);
      documents->AddColumn(content_array);
    
      vtkSmartPointer<vtkTextExtraction> text_extraction = vtkSmartPointer<vtkTextExtraction>::New();
      text_extraction->SetInputConnection(0, documents->GetProducerPort());
    
      vtkSmartPointer<vtkTokenizer> tokenizer = vtkSmartPointer<vtkTokenizer>::New();
      tokenizer->SetInputConnection(0, text_extraction->GetOutputPort());
      tokenizer->AddDroppedDelimiters(vtkTokenizer::Whitespace());
      tokenizer->AddKeptDelimiters(vtkTokenizer::Punctuation());

      vtkSmartPointer<vtkFoldCase> fold_case = vtkSmartPointer<vtkFoldCase>::New();
      fold_case->SetInputConnection(0, tokenizer->GetOutputPort());

      vtkSmartPointer<vtkTokenValueFilter> token_value = vtkSmartPointer<vtkTokenValueFilter>::New();
      token_value->SetInputConnection(0, fold_case->GetOutputPort());
      token_value->AddStopWordValues();

      vtkSmartPointer<vtkStreamAwareFeatureDictionary> term_dictionary = vtkSmartPointer<vtkStreamAwareFeatureDictionary>::New();
      term_dictionary->SetInputConnection(0, token_value->GetOutputPort());

      vtkInformation *extra_info = vtkInformation::New();
      extra_info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 2);
      vtkThreadedStreamingPipeline::Push(text_extraction->GetExecutive(), extra_info);
      vtkExecutionScheduler::GetGlobalScheduler()->WaitUntilAllDone();
      term_dictionary->GetOutput()->Dump(20);

      documents = vtkSmartPointer<vtkTable>::New();
      documents->AddColumn(document_array);
      documents->AddColumn(uri_array);
      documents->AddColumn(mime_type_array);
      documents->AddColumn(content_array);      
      content_array->SetValue(0, "hello");
      text_extraction->SetInputConnection(0, documents->GetProducerPort());
      
      vtkThreadedStreamingPipeline::Push(text_extraction->GetExecutive(), extra_info);
      vtkExecutionScheduler::GetGlobalScheduler()->WaitUntilAllDone();
      term_dictionary->GetOutput()->Dump(20);
    
      return 0;
    }
  catch(std::exception& e)
    {
      cerr << e.what() << endl;
      return 1;
    }
}
