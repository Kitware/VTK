/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPowerWeighting.cxx

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

#include <vtkSmartPointer.h>
#include <vtkPowerWeighting.h>
#include <vtkDenseArray.h>

int TestPowerWeighting(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  try
    {
    vtkSmartPointer<vtkDenseArray<double> > array =
      vtkSmartPointer<vtkDenseArray<double> >::New();
    array->Resize(2);
    array->SetValue(0, 3);
    array->SetValue(1, 4);

    vtkSmartPointer<vtkArrayData> arrayData =
      vtkSmartPointer<vtkArrayData>::New();
    arrayData->AddArray(array);

    vtkSmartPointer<vtkPowerWeighting> powerWeighting =
      vtkSmartPointer<vtkPowerWeighting>::New();
    powerWeighting->SetPower(2);
    powerWeighting->SetInputConnection(arrayData->GetProducerPort());
    powerWeighting->Update();

    vtkSmartPointer<vtkArrayData> outputArrayData = powerWeighting->GetOutput();
    vtkSmartPointer<vtkDenseArray<double> > outputArray =
      vtkDenseArray<double>::SafeDownCast(outputArrayData->GetArray(0));

    if((outputArray->GetValue(0) != 9) || (outputArray->GetValue(1) != 16))
      {
      return EXIT_FAILURE;
      }

    return EXIT_SUCCESS;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    }

  return EXIT_FAILURE;
}
