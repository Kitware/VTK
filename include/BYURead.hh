/*=========================================================================

  Program:   Visualization Library
  Module:    BYURead.hh
  Language:  C++
  Date:      7/20/94
  Version:   1.2

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlBYUReader - read MOVIE.BYU polygon files
// .SECTION Description
// vlBYUReader is a source object that reads MOVIE.BYU polygon files.
// These files consist of a geometry file (.g), a scalar file (.s), a 
// displacement or vector file (.d), and a 2D texture coordinate file
// (.t).

#ifndef __vlBYUReader_h
#define __vlBYUReader_h

#include <stdio.h>
#include "PolySrc.hh"

class vlBYUReader : public vlPolySource 
{
public:
  vlBYUReader();
  ~vlBYUReader();
  char *GetClassName() {return "vlBYUReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(GeometryFilename);
  vlGetStringMacro(GeometryFilename);

  vlSetStringMacro(DisplacementFilename);
  vlGetStringMacro(DisplacementFilename);

  vlSetStringMacro(ScalarFilename);
  vlGetStringMacro(ScalarFilename);

  vlSetStringMacro(TextureFilename);
  vlGetStringMacro(TextureFilename);

  vlSetMacro(ReadDisplacement,int)
  vlGetMacro(ReadDisplacement,int)
  vlBooleanMacro(ReadDisplacement,int)
  
  vlSetMacro(ReadScalar,int)
  vlGetMacro(ReadScalar,int)
  vlBooleanMacro(ReadScalar,int)
  
  vlSetMacro(ReadTexture,int)
  vlGetMacro(ReadTexture,int)
  vlBooleanMacro(ReadTexture,int)

  vlSetClampMacro(PartNumber,int,1,LARGE_INTEGER);
  vlGetMacro(PartNumber,int);

protected:
  void Execute();

  char *GeometryFilename;
  char *DisplacementFilename;
  char *ScalarFilename;
  char *TextureFilename;
  int ReadDisplacement;
  int ReadScalar;
  int ReadTexture;
  int PartNumber;

  void ReadGeometryFile(FILE *fp, int &numPts);
  void ReadDisplacementFile(int numPts);
  void ReadScalarFile(int numPts);
  void ReadTextureFile(int numPts);
};

#endif


