/*=========================================================================

  Program:   Visualization Library
  Module:    BYUWrite.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlBYUWriter - write MOVIE.BYU files
// .SECTION Description
// vlBYUWriter writes MOVIE.BYU polygonal files. These files consist 
// of a geometry file (.g), a scalar file (.s), a displacement or 
// vector file (.d), and a 2D texture coordinate file (.t). These files 
// must be specified to the object, the appropriate boolean 
// variables must be true, and data must be available from the input
// for the files to be written.

#ifndef __vlBYUWriter_h
#define __vlBYUWriter_h

#include <stdio.h>
#include "Writer.hh"
#include "PolyF.hh"

class vlBYUWriter : public vlWriter, public vlPolyFilter
{
public:
  vlBYUWriter();
  ~vlBYUWriter();
  char *GetClassName() {return "vlBYUWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(GeometryFilename);
  vlGetStringMacro(GeometryFilename);

  vlSetStringMacro(DisplacementFilename);
  vlGetStringMacro(DisplacementFilename);

  vlSetStringMacro(ScalarFilename);
  vlGetStringMacro(ScalarFilename);

  vlSetStringMacro(TextureFilename);
  vlGetStringMacro(TextureFilename);

  vlSetMacro(WriteDisplacement,int);
  vlGetMacro(WriteDisplacement,int);
  vlBooleanMacro(WriteDisplacement,int);
  
  vlSetMacro(WriteScalar,int);
  vlGetMacro(WriteScalar,int);
  vlBooleanMacro(WriteScalar,int);
  
  vlSetMacro(WriteTexture,int);
  vlGetMacro(WriteTexture,int);
  vlBooleanMacro(WriteTexture,int);

  void Write();

protected:
  void Execute() {this->Write();};

  char *GeometryFilename;
  char *DisplacementFilename;
  char *ScalarFilename;
  char *TextureFilename;
  int WriteDisplacement;
  int WriteScalar;
  int WriteTexture;

  void WriteGeometryFile(FILE *fp, int numPts);
  void WriteDisplacementFile(int numPts);
  void WriteScalarFile(int numPts);
  void WriteTextureFile(int numPts);
};

#endif

