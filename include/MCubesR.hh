/*=========================================================================

  Program:   Visualization Library
  Module:    MCubesR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlMCubesReader - read binary marching cubes file
// .SECTION Description
// vlMCubesReader is a source object that reads binary marching cubes
// files. (Marching cubes is an iso-surfacing technique that generates 
// many triangles). The binary format is supported by B. Lorensen's
// marching cubes program. The format repeats point coordinates, so
// this object will merge the points with a vlLocator object. You can 
// choose to supply the vlLocator or use the default.
// .SECTION Caveats
// Binary files assumed written in sun/hp/sgi form.

#ifndef __vlMCubesReader_h
#define __vlMCubesReader_h

#include <stdio.h>
#include "PolySrc.hh"
#include "FPoints.hh"
#include "CellArr.hh"

class vlMCubesReader : public vlPolySource 
{
public:
  vlMCubesReader();
  ~vlMCubesReader();
  char *GetClassName() {return "vlMCubesReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of marching cubes file.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  // Description:
  // Specify file name of marching cubes limits file.
  vlSetStringMacro(LimitsFilename);
  vlGetStringMacro(LimitsFilename);

  // Description:
  // Specify whether to flip normals in opposite direction.
  vlSetMacro(FlipNormals,int);
  vlGetMacro(FlipNormals,int);
  vlBooleanMacro(FlipNormals,int);

  // Description:
  // Specify whether to read normals.
  vlSetMacro(Normals,int);
  vlGetMacro(Normals,int);
  vlBooleanMacro(Normals,int);

  void SetLocator(vlLocator *locator);
  void SetLocator(vlLocator& locator) {this->SetLocator(&locator);};
  vlGetObjectMacro(Locator,vlLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  char *Filename;
  char *LimitsFilename;

  vlLocator *Locator;
  int SelfCreatedLocator;

  int FlipNormals;
  int Normals;

  void Execute();
};

#endif


