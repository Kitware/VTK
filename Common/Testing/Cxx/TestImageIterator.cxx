/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageIterator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of image iterators
// .SECTION Description
// this program tests the image iterators
// At this point it only creates an object of every supported type.

#include "vtkDebugLeaks.h"
#include "vtkImageIterator.h"
#include "vtkImageProgressIterator.h"
#include "vtkImageData.h"

template<typename T>
int DoTest(T* = 0)
{
  int ext[6] = { 0, 0, 0, 0, 0, 0 };
  vtkImageData *id = vtkImageData::New();
  vtkImageIterator<T> *it = new vtkImageIterator<T>(id,ext);
  vtkImageProgressIterator<T> *ipt 
    = new vtkImageProgressIterator<T>(id,ext,0,0);
  delete it;
  delete ipt;
  return 0;
  id->Delete();
}

int main()
{
  vtkDebugLeaks::PromptUserOff();

  DoTest<char>();
  DoTest<int>();
  DoTest<long>();
  DoTest<short>();
  DoTest<float>();
  DoTest<double>();
  DoTest<unsigned long>();
  DoTest<unsigned short>();
  DoTest<unsigned char>();
  DoTest<unsigned int>();  

  return 0;
} 
