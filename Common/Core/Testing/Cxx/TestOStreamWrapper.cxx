/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOStreamWrapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#define VTK_STREAMS_FWD_ONLY // like wrapper-generated sources
#include "vtkSystemIncludes.h"

#include <stdio.h> // test covers NOT including <iostream>
#include <string>

int TestOStreamWrapper(int, char *[])
{
  int failed = 0;
  std::string const expect = "hello, world: 1";
  std::string actual;
  std::string s = "hello, world";
  vtkOStrStreamWrapper vtkmsg;
  vtkmsg << s << ": " << 1;
  actual = vtkmsg.str();
  vtkmsg.rdbuf()->freeze(0);
  if(actual != expect)
    {
    failed = 1;
    fprintf(stderr, "Expected '%s' but got '%s'\n",
            expect.c_str(), actual.c_str());
    }
  return failed;
}
