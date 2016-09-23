/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLookupTableThreaded.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLookupTable.h"
#include "vtkMultiThreader.h"
#include "vtkNew.h"

namespace {

vtkLookupTable * lut;

VTK_THREAD_RETURN_TYPE ThreadedMethod(void *)
{
  int numberOfValues = 25;
  double* input = new double[numberOfValues];
  for (int i = 0; i < numberOfValues; ++i)
  {
    input[i] = static_cast<double>(i);
  }
  unsigned char* output = new unsigned char[4*numberOfValues];
  int inputType = VTK_DOUBLE;
  int inputIncrement = 1;
  int outputFormat = VTK_RGBA;

  lut->MapScalarsThroughTable2(input, output, inputType, numberOfValues,
                               inputIncrement, outputFormat);

  delete[] input;
  delete[] output;

  return VTK_THREAD_RETURN_VALUE;
}

} // end anonymous namespace

int TestLookupTableThreaded(int, char* [])
{
  lut = vtkLookupTable::New();
  lut->SetNumberOfTableValues( 1024 );

  vtkNew<vtkMultiThreader> threader;
  threader->SetSingleMethod( ThreadedMethod, NULL );
  threader->SetNumberOfThreads( 4 );
  threader->SingleMethodExecute();

  lut->Delete();

  return EXIT_SUCCESS;
}
