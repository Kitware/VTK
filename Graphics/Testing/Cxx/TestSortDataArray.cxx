/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSortDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// -*- c++ -*- *******************************************************

#include "vtkSortDataArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkTimerLog.h"

#define ARRAY_SIZE (2*1024*1024)
//  #define ARRAY_SIZE 128

int TestSortDataArray(int, char *[])
{
  vtkIdType i;
  vtkTimerLog *timer = vtkTimerLog::New();

  cout << "Building array" << endl;
  vtkIntArray *keys = vtkIntArray::New();
  keys->SetNumberOfComponents(1);
  keys->SetNumberOfTuples(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; i++)
    {
    keys->SetComponent(i, 0, (int)vtkMath::Random(0, ARRAY_SIZE*4));
    }

  cout << "Sorting array" << endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys);
  timer->StopTimer();

  cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << endl;

  for (i = 0; i < ARRAY_SIZE-1; i++)
    {
    if (keys->GetComponent(i, 0) > keys->GetComponent(i+1, 0))
      {
      cout << "Array not properly sorted!" << endl;
      break;
      }
    }
  cout << "Array consistency check finished\n" << endl;

  cout << "Sorting sorted array" << endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys);
  timer->StopTimer();

  cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << endl;

  for (i = 0; i < ARRAY_SIZE-1; i++)
    {
    if (keys->GetComponent(i, 0) > keys->GetComponent(i+1, 0))
      {
      cout << "Array not properly sorted!" << endl;
      break;
      }
    }
  cout << "Array consistency check finished\n" << endl;

  cout << "Building key/value arrays\n" << endl;
  vtkIntArray *values = vtkIntArray::New();
  values->SetNumberOfComponents(2);
  values->SetNumberOfTuples(ARRAY_SIZE);
  for (i = 0; i < ARRAY_SIZE; i++)
    {
    keys->SetComponent(i, 0, (int)vtkMath::Random(0, ARRAY_SIZE*4));
    values->SetComponent(i, 0, i);
    values->SetComponent(i, 1, (int)vtkMath::Random(0, ARRAY_SIZE*4));
    }
  vtkIntArray *saveKeys = vtkIntArray::New();
  saveKeys->DeepCopy(keys);
  vtkIntArray *saveValues = vtkIntArray::New();
  saveValues->DeepCopy(values);

  cout << "Sorting arrays" << endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys, values);
  timer->StopTimer();

  cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << endl;

  for (i = 0; i < ARRAY_SIZE-1; i++)
    {
    int lookup = (int)values->GetComponent(i, 0);
    if (keys->GetComponent(i, 0) > keys->GetComponent(i+1, 0))
      {
      cout << "Array not properly sorted!" << endl;
      break;
      }
    if (keys->GetComponent(i, 0) != saveKeys->GetComponent(lookup, 0))
      {
      cout << "Values array not consistent with keys array!" << endl;
      break;
      }
    if (values->GetComponent(i, 1) != saveValues->GetComponent(lookup, 1))
      {
      cout << "Values array not consistent with keys array!" << endl;
      break;
      }
    }
  cout << "Array consistency check finished\n" << endl;

  cout << "Sorting sorted arrays" << endl;
  timer->StartTimer();
  vtkSortDataArray::Sort(keys, values);
  timer->StopTimer();

  cout << "Time to sort array: " << timer->GetElapsedTime() << " sec" << endl;

  for (i = 0; i < ARRAY_SIZE-1; i++)
    {
    int lookup = (int)values->GetComponent(i, 0);
    if (keys->GetComponent(i, 0) > keys->GetComponent(i+1, 0))
      {
      cout << "Array not properly sorted!" << endl;
      break;
      }
    if (keys->GetComponent(i, 0) != saveKeys->GetComponent(lookup, 0))
      {
      cout << "Values array not consistent with keys array!" << endl;
      break;
      }
    if (values->GetComponent(i, 1) != saveValues->GetComponent(lookup, 1))
      {
      cout << "Values array not consistent with keys array!" << endl;
      break;
      }
    }
  cout << "Array consistency check finished\n" << endl;

  timer->Delete();
  keys->Delete();
  values->Delete();
  saveKeys->Delete();
  saveValues->Delete();

  return 0;
}
