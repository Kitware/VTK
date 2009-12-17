/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMatlabEngineFilter.cxx
  
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

#include <vtkMatlabEngineFilter.h>
#include <vtkSmartPointer.h>
#include <vtkCylinderSource.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkArrayExtents.h>
#include <vtkRRandomTableSource.h>
#include <vtkTable.h>
#include <vtkTableToSparseArray.h>
#include <vtkDenseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

namespace
{

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

bool doubleEquals(double left, double right, double epsilon) {
  return (fabs(left - right) < epsilon);
}

}

int TestMatlabEngineFilter(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    int i;
    vtkCylinderSource* cs = vtkCylinderSource::New();
    vtkMatlabEngineFilter* mef = vtkMatlabEngineFilter::New();
    vtkRRandomTableSource* rts = vtkRRandomTableSource::New();
    vtkMatlabEngineFilter* mef2 = vtkMatlabEngineFilter::New();
    vtkDataSet* ds;
    vtkPointData* pd;
    vtkDoubleArray* da;
    vtkDoubleArray* rda;

    cs->SetResolution(10);
    mef->SetInputConnection(cs->GetOutputPort());
    mef->SetEngineVisible(0);
    mef->SetEngineOutput(0);
    mef->PutArray("Normals", "Norm");
    mef->PutArray("TCoords", "TCoords");
    mef->GetArray("Normalsnew", "Norm");
    mef->GetArray("TCoordsnew", "TCoords");
    mef->SetMatlabScript("Norm = Norm.^2\nTCoords = TCoords + TCoords\n");
    mef->Update();

    ds = vtkDataSet::SafeDownCast(mef->GetOutput());
    pd = ds->GetPointData();
    da = (vtkDoubleArray*) pd->GetArray("Normals");
    rda = (vtkDoubleArray*) pd->GetArray("Normalsnew");

    for(i=0;i<da->GetNumberOfTuples();i++)
      {
      double* itup = da->GetTuple3(i);
      double* rtup = rda->GetTuple3(i);
      test_expression(doubleEquals(rtup[0],pow(itup[0],2),0.0001));
      test_expression(doubleEquals(rtup[1],pow(itup[1],2),0.0001));
      test_expression(doubleEquals(rtup[2],pow(itup[2],2),0.0001));
      }

    da = (vtkDoubleArray*) pd->GetArray("TCoords");
    rda = (vtkDoubleArray*) pd->GetArray("TCoordsnew");

    for(i=0;i<da->GetNumberOfTuples();i++)
      {
      double* itup = da->GetTuple2(i);
      double* rtup = rda->GetTuple2(i);
      test_expression(doubleEquals(rtup[0],itup[0]+itup[0],0.0001));
      test_expression(doubleEquals(rtup[1],itup[1]+itup[1],0.0001));
      }

    rts->SetNumberOfRows(20);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable One",0);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Two",1);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Three",2);
    rts->SetStatisticalDistributionForColumn(vtkRRandomTableSource::NORMAL,0.0,1.0,0.0,"Variable Four",3);
    mef2->SetInputConnection(rts->GetOutputPort());
    mef2->RemoveAllGetVariables();
    mef2->RemoveAllPutVariables();
    mef2->SetEngineVisible(0);
    mef2->SetEngineOutput(0);
    mef2->PutArray("Variable One", "v1");
    mef2->PutArray("Variable Two", "v2");
    mef2->PutArray("Variable Three", "v3");
    mef2->PutArray("Variable Four", "v4");
    mef2->GetArray("Variable One", "v1");
    mef2->GetArray("Variable Two", "v2");
    mef2->GetArray("Variable Three", "v3");
    mef2->GetArray("Variable Four", "v4");
    mef2->SetMatlabScript("v1 = (randperm(20) - 1)'\n\
                           v2 = (randperm(20) - 1)'\n\
                           v3 = (randperm(20) - 1)'\n");
    mef2->Update();
    vtkTable* table = vtkTable::SafeDownCast(mef2->GetOutput());

    vtkSmartPointer<vtkTableToSparseArray> source = vtkSmartPointer<vtkTableToSparseArray>::New();
    source->AddInputConnection(mef2->GetOutputPort());
    source->AddCoordinateColumn("Variable One");
    source->AddCoordinateColumn("Variable Two");
    source->AddCoordinateColumn("Variable Three");
    source->SetValueColumn("Variable Four");
    mef->SetInputConnection(source->GetOutputPort());
    mef->RemoveAllPutVariables();
    mef->RemoveAllGetVariables();
    mef->PutArray("0","a");
    mef->GetArray("1","a");
    mef->SetMatlabScript("a = sqrt(a + 5.0);\n");
    mef->Update();

    vtkDenseArray<double>* const dense_array = 
                 vtkDenseArray<double>::SafeDownCast(vtkArrayData::SafeDownCast(mef->GetOutput())->GetArray(1));
    test_expression(dense_array);

    for(i=0;i<table->GetNumberOfColumns();i++)
      {
      int ind0 = table->GetValue(i,0).ToInt();
      int ind1 = table->GetValue(i,1).ToInt();
      int ind2 = table->GetValue(i,2).ToInt();
      double table_val = rts->GetOutput()->GetValue(i,3).ToDouble();
      double dense_val = dense_array->GetValue(vtkArrayCoordinates(ind0,ind1,ind2));
      test_expression(doubleEquals(sqrt(table_val + 5.0),dense_val,0.0001));
      }

    cs->Delete();
    rts->Delete();
    mef->Delete();
    mef2->Delete();

    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

