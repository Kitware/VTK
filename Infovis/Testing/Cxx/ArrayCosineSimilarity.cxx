/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ArrayCosineSimilarity.cxx
  
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

#include <vtkArrayData.h>
#include <vtkArrayPrint.h>
#include <vtkCosineSimilarity.h>
#include <vtkDenseArray.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVariant.h>

#include <vtksys/ios/iostream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    throw vtkstd::runtime_error("Expression failed: " #expression); \
}

static const bool close_enough(const double lhs, const double rhs)
{
  return fabs(lhs - rhs) < 1.0e-12;
}

int ArrayCosineSimilarity(int argc, char* argv[])
{
  cout << setprecision(17);
  
  try
    {
    vtkSmartPointer<vtkDenseArray<double> > source = vtkSmartPointer<vtkDenseArray<double> >::New();
    source->Resize(vtkArrayExtents(2, 4));
    source->SetValue(vtkArrayCoordinates(0, 0), 1);
    source->SetValue(vtkArrayCoordinates(1, 0), 0);
    source->SetValue(vtkArrayCoordinates(0, 1), 1);
    source->SetValue(vtkArrayCoordinates(1, 1), 1);
    source->SetValue(vtkArrayCoordinates(0, 2), 0);
    source->SetValue(vtkArrayCoordinates(1, 2), 1);
    source->SetValue(vtkArrayCoordinates(0, 3), -1);
    source->SetValue(vtkArrayCoordinates(1, 3), 1);

    cout << "source matrix:\n";
    vtkPrintMatrixFormat(cout, source.GetPointer());
    
    vtkSmartPointer<vtkArrayData> source_data = vtkSmartPointer<vtkArrayData>::New();
    source_data->AddArray(source);
     
    vtkSmartPointer<vtkCosineSimilarity> similarity = vtkSmartPointer<vtkCosineSimilarity>::New();
    similarity->AddInputConnection(source_data->GetProducerPort());
    similarity->SetVectorDimension(1);
    similarity->Update();

    vtkTable* const table = similarity->GetOutput();
    test_expression(table->GetNumberOfColumns() == 3);

    cout << "similarity table:\n";
    for(vtkIdType row = 0; row < table->GetNumberOfRows(); ++row)
      {
      cout
        << table->GetValue(row, 0).ToInt()
        << " -> "
        << table->GetValue(row, 1).ToInt()
        << ": "
        << table->GetValue(row, 2).ToDouble()
        << "\n";
      }
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

