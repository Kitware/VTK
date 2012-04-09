/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

template<class T>
inline int DoTest(T*)
{
  int ext[6] = { 0, 0, 0, 0, 0, 0 };
  vtkImageData *id = vtkImageData::New();
  id->SetExtent(ext);
  id->AllocateScalars(VTK_DOUBLE, 1);
  vtkImageIterator<T> *it = new vtkImageIterator<T>(id,ext);
  vtkImageProgressIterator<T> *ipt
    = new vtkImageProgressIterator<T>(id,ext,0,0);
  delete it;
  delete ipt;
  id->Delete();
  return 0;
}


int TestImageIterator(int,char *[])
{
  DoTest( static_cast<char*>( 0 ) );
  DoTest( static_cast<int*>( 0 ) );
  DoTest( static_cast<long*>( 0 ) );
  DoTest( static_cast<short*>( 0 ) );
  DoTest( static_cast<float*>( 0 ) );
  DoTest( static_cast<double*>( 0 ) );
  DoTest( static_cast<unsigned long*>( 0 ) );
  DoTest( static_cast<unsigned short*>( 0 ) );
  DoTest( static_cast<unsigned char*>( 0 ) );
  DoTest( static_cast<unsigned int*>( 0 ) );

  return 0;
}
