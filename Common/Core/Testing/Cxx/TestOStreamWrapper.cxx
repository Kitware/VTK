// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#define VTK_STREAMS_FWD_ONLY // like wrapper-generated sources
#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include "vtkSystemIncludes.h"

#include <cstdio> // test covers NOT including <iostream>
#include <sstream>
#include <string>

int TestOStreamWrapper(int, char*[])
{
  std::string const expect = "hello, world: 1";
  std::string actual;
  std::string s = "hello, world";
  vtkOStrStreamWrapper vtkmsg;
  vtkmsg << s << ": " << 1;
  actual = vtkmsg.str();
  vtkmsg.rdbuf()->freeze(0);
  if (actual != expect)
  {
    fprintf(stderr, "Expected '%s' but got '%s'\n", expect.c_str(), actual.c_str());
    return 1;
  }

  // Verify that vtkSmartPointer can be printed as address.
  vtkSmartPointer<vtkObject> smartPointedObject = vtkSmartPointer<vtkObject>::New();
  vtkOStrStreamWrapper oStrStreamWrapper;
  oStrStreamWrapper << smartPointedObject;
  std::string smartPointerStr = oStrStreamWrapper.str();

  // Verify that the address retrieved by oStrStreamWrapper is the same as the real address
  std::stringstream strStream;
  strStream << smartPointedObject;

  if (smartPointerStr != strStream.str())
  {
    fprintf(stderr,
      "Output of oStrStreamWrapper for vtkSmartPointer (%s) differs from its address (%s)\n",
      smartPointerStr.c_str(), strStream.str().c_str());
    return 1;
  }

  return 0;
}
