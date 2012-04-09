/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMatlabEngineInterface.cxx

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

#include <vtkMatlabEngineInterface.h>
#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>
#include <vtkArray.h>
#include <vtkDenseArray.h>

#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>
#include <stdio.h>
#include <string.h>
#include <assert.h>

namespace
{

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

bool doubleEquals(double left, double right, double epsilon) {
  return (fabs(left - right) < epsilon);
}

}

int TestMatlabEngineInterface(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    int buf_size = 2000;
    char* out_buffer = new char[buf_size];
    out_buffer[0] = '\0';
    vtkDoubleArray* da = vtkDoubleArray::New();
    vtkDenseArray<double>* dda = vtkDenseArray<double>::New();
    vtkMatlabEngineInterface* mei = vtkMatlabEngineInterface::New();
    mei->SetVisibleOff();
    mei->OutputBuffer(out_buffer, buf_size);
    mei->EvalString("1:10\n");
    test_expression(strlen(out_buffer) > 10);

    da->SetNumberOfComponents(3);
    for( int cc = 0; cc < 10; cc ++ )
      {
      da->InsertNextTuple3( cc + 0.1, cc + 0.2, cc + 0.3);
      }

    mei->PutVtkDataArray("d",da);
    mei->EvalString("d(:,1) = d(:,1) - 0.1;\n\
                     d(:,2) = d(:,2) - 0.2;\n\
                     d(:,3) = d(:,3) - 0.3;\n");
    cout << out_buffer << endl;
    vtkDoubleArray* rda = vtkDoubleArray::SafeDownCast(mei->GetVtkDataArray("d"));
    test_expression(rda);
    for(int i = 0;i<rda->GetNumberOfTuples();i++)
      {
      double* iv = da->GetTuple3(i);
      double* rv = rda->GetTuple3(i);
      test_expression(doubleEquals(iv[0] - 0.1,rv[0],0.001));
      test_expression(doubleEquals(iv[1] - 0.2,rv[1],0.001));
      test_expression(doubleEquals(iv[2] - 0.3,rv[2],0.001));
      }

    dda->Resize(vtkArrayExtents(3, 3, 5));
    dda->Fill(64.0);
    mei->PutVtkArray("a",dda);
    mei->EvalString("a = sqrt(a);\n");
    vtkDenseArray<double>* rdda = vtkDenseArray<double>::SafeDownCast(mei->GetVtkArray("a"));
    assert(rdda->GetExtents().ZeroBased());
    const vtkArrayExtents extents = rdda->GetExtents();
    for(int i = 0; i != extents[0].GetSize(); ++i)
      {
      for(int j = 0; j != extents[1].GetSize(); ++j)
        {
        for(int k = 0; k != extents[2].GetSize(); ++k)
          {
          test_expression(doubleEquals(sqrt(dda->GetValue(vtkArrayCoordinates(i, j, k))),
                                       rdda->GetValue(vtkArrayCoordinates(i, j, k)),
                                       0.001));
          }
        }
      }
    dda->Delete();
    da->Delete();
    mei->Delete();
    delete [] out_buffer;
    return 0;
    }
  catch(std::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

