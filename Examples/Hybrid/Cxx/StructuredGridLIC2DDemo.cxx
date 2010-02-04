/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StructuredGridLIC2DDemo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestStructuredGridLIC2DSlice.h"

extern int StructuredGridLIC2DSlice(int argc, char* argv[]);

int main( int argc, char * argv[] )
{
  RenderingMode = STRUCTURED_GRID_LIC2D_SLICE_DEMO;
  return StructuredGridLIC2DSlice( argc, argv );
}
