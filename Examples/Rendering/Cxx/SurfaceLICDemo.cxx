/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SurfaceLICDemo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestSurfaceLIC.h"

extern int SurfaceLIC( int argc, char * argv[] );

int main( int argc, char * argv[] )
{
  RenderingMode = SURFACE_LIC_DEMO;
  return SurfaceLIC( argc, argv );
}
