/*=========================================================================

  Program:   Visualization Library
  Module:    BYUWrite.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "PolyData.hh"

class vlBYUWriter : public vlWriter
{
public:
  vlBYUWriter();
  ~vlBYUWriter();
  char *GetClassName() {return "vlBYUWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlPolyData *input);
  void SetInput(vlPolyData &input) {this->SetInput(&input);};
  vlPolyData *GetInput() {return (vlPolyData *)this->Input;};
                               
  // Description:
  // Specify the name of the geometry file to write.
  vlSetStringMacro(GeometryFilename);
  vlGetStringMacro(GeometryFilename);

  // Description:
  // Specify the name of the displacement file to write.
  vlSetStringMacro(DisplacementFilename);
  vlGetStringMacro(DisplacementFilename);

  // Description:
  // Specify the name of the scalar file to write.
  vlSetStringMacro(ScalarFilename);
  vlGetStringMacro(ScalarFilename);

  // Description:
  // Specify the name of the texture file to write.
  vlSetStringMacro(TextureFilename);
  vlGetStringMacro(TextureFilename);

  // Description:
  // Turn on/off writing the displacement file.
  vlSetMacro(WriteDisplacement,int);
  vlGetMacro(WriteDisplacement,int);
  vlBooleanMacro(WriteDisplacement,int);
  
  // Description:
  // Turn on/off writing the scalar file.
  vlSetMacro(WriteScalar,int);
  vlGetMacro(WriteScalar,int);
  vlBooleanMacro(WriteScalar,int);
  
  // Description:
  // Turn on/off writing the texture file.
  vlSetMacro(WriteTexture,int);
  vlGetMacro(WriteTexture,int);
  vlBooleanMacro(WriteTexture,int);

protected:
  void WriteData();

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

