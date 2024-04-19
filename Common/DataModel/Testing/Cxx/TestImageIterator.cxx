// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of image iterators
// .SECTION Description
// this program tests the image iterators
// At this point it only creates an object of every supported type.

#include "vtkDebugLeaks.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageProgressIterator.h"

template <class T>
inline int DoTest(T*)
{
  int ext[6] = { 0, 0, 0, 0, 0, 0 };
  vtkImageData* id = vtkImageData::New();
  id->SetExtent(ext);
  id->AllocateScalars(VTK_DOUBLE, 1);
  vtkImageIterator<T>* it = new vtkImageIterator<T>(id, ext);
  vtkImageProgressIterator<T>* ipt = new vtkImageProgressIterator<T>(id, ext, nullptr, 0);
  delete it;
  delete ipt;
  id->Delete();
  return 0;
}

int TestImageIterator(int, char*[])
{
  DoTest(static_cast<char*>(nullptr));
  DoTest(static_cast<int*>(nullptr));
  DoTest(static_cast<long*>(nullptr));
  DoTest(static_cast<short*>(nullptr));
  DoTest(static_cast<float*>(nullptr));
  DoTest(static_cast<double*>(nullptr));
  DoTest(static_cast<unsigned long*>(nullptr));
  DoTest(static_cast<unsigned short*>(nullptr));
  DoTest(static_cast<unsigned char*>(nullptr));
  DoTest(static_cast<unsigned int*>(nullptr));

  return 0;
}
