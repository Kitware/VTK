/*=========================================================================

  Program:   Visualization Toolkit
  Module:    MCubesR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkMCubesReader - read binary marching cubes file
// .SECTION Description
// vtkMCubesReader is a source object that reads binary marching cubes
// files. (Marching cubes is an iso-surfacing technique that generates 
// many triangles). The binary format is supported by B. Lorensen's
// marching cubes program. The format repeats point coordinates, so
// this object will merge the points with a vtkLocator object. You can 
// choose to supply the vtkLocator or use the default.
// .SECTION Caveats
// Binary files assumed written in sun/hp/sgi form.

#ifndef __vtkMCubesReader_h
#define __vtkMCubesReader_h

#include <stdio.h>
#include "PolySrc.hh"
#include "FPoints.hh"
#include "CellArr.hh"

class vtkMCubesReader : public vtkPolySource 
{
public:
  vtkMCubesReader();
  ~vtkMCubesReader();
  char *GetClassName() {return "vtkMCubesReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of marching cubes file.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // Specify file name of marching cubes limits file.
  vtkSetStringMacro(LimitsFilename);
  vtkGetStringMacro(LimitsFilename);

  // Description:
  // Specify whether to flip normals in opposite direction.
  vtkSetMacro(FlipNormals,int);
  vtkGetMacro(FlipNormals,int);
  vtkBooleanMacro(FlipNormals,int);

  // Description:
  // Specify whether to read normals.
  vtkSetMacro(Normals,int);
  vtkGetMacro(Normals,int);
  vtkBooleanMacro(Normals,int);

  void SetLocator(vtkLocator *locator);
  void SetLocator(vtkLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  char *Filename;
  char *LimitsFilename;

  vtkLocator *Locator;
  int SelfCreatedLocator;

  int FlipNormals;
  int Normals;

  void Execute();
};

#endif


