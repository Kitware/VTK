/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BYURead.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkBYUReader - read MOVIE.BYU polygon files
// .SECTION Description
// vtkBYUReader is a source object that reads MOVIE.BYU polygon files.
// These files consist of a geometry file (.g), a scalar file (.s), a 
// displacement or vector file (.d), and a 2D texture coordinate file
// (.t).

#ifndef __vtkBYUReader_h
#define __vtkBYUReader_h

#include <stdio.h>
#include "PolySrc.hh"

class vtkBYUReader : public vtkPolySource 
{
public:
  vtkBYUReader();
  ~vtkBYUReader();
  char *GetClassName() {return "vtkBYUReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify name of geometry filename.
  vtkSetStringMacro(GeometryFilename);
  vtkGetStringMacro(GeometryFilename);

  // Description:
  // Specify name of displacement filename.
  vtkSetStringMacro(DisplacementFilename);
  vtkGetStringMacro(DisplacementFilename);

  // Description:
  // Specify name of scalar filename.
  vtkSetStringMacro(ScalarFilename);
  vtkGetStringMacro(ScalarFilename);

  // Description:
  // Specify name of texture coordinates filename.
  vtkSetStringMacro(TextureFilename);
  vtkGetStringMacro(TextureFilename);

  // Description:
  // Turn on/off the reading of the displacement file.
  vtkSetMacro(ReadDisplacement,int)
  vtkGetMacro(ReadDisplacement,int)
  vtkBooleanMacro(ReadDisplacement,int)
  
  // Description:
  // Turn on/off the reading of the scalar file.
  vtkSetMacro(ReadScalar,int)
  vtkGetMacro(ReadScalar,int)
  vtkBooleanMacro(ReadScalar,int)
  
  // Description:
  // Turn on/off the reading of the texture coordinate file.
  // Specify name of geometry filename.
  vtkSetMacro(ReadTexture,int)
  vtkGetMacro(ReadTexture,int)
  vtkBooleanMacro(ReadTexture,int)

  vtkSetClampMacro(PartNumber,int,1,LARGE_INTEGER);
  vtkGetMacro(PartNumber,int);

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


