/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDocumentReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#include <TextAnalysisTestConfig.h>

#include <vtkArrayData.h>
#include <vtkDenseArray.h>
#include <vtkDocumentReader.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtkstd/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtkstd::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

int TestDocumentReader(int, char*[])
{
  try
    {
    vtkSmartPointer<vtkDocumentReader> reader = vtkSmartPointer<vtkDocumentReader>::New();

    reader->AddFile(VTK_DATA_ROOT "/Data/authors.csv");
    reader->AddFile(VTK_DATA_ROOT "/Data/fruit.csv");
    reader->Update();

    test_expression(reader->GetOutput());
    test_expression(reader->GetOutput()->GetNumberOfRows() == 2);
    test_expression(reader->GetOutput()->GetValueByName(0, "document").ToInt() == 0);
    test_expression(reader->GetOutput()->GetValueByName(1, "document").ToInt() == 1);
    reader->GetOutput(0)->Dump(64);

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

