/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BYUWrite.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkBYUWriter - write MOVIE.BYU files
// .SECTION Description
// vtkBYUWriter writes MOVIE.BYU polygonal files. These files consist 
// of a geometry file (.g), a scalar file (.s), a displacement or 
// vector file (.d), and a 2D texture coordinate file (.t). These files 
// must be specified to the object, the appropriate boolean 
// variables must be true, and data must be available from the input
// for the files to be written.

#ifndef __vtkBYUWriter_h
#define __vtkBYUWriter_h

#include <stdio.h>
#include "Writer.hh"
#include "PolyData.hh"

class vtkBYUWriter : public vtkWriter
{
public:
  vtkBYUWriter();
  ~vtkBYUWriter();
  char *GetClassName() {return "vtkBYUWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkPolyData *input);
  void SetInput(vtkPolyData &input) {this->SetInput(&input);};
  vtkPolyData *GetInput() {return (vtkPolyData *)this->Input;};
                               
  // Description:
  // Specify the name of the geometry file to write.
  vtkSetStringMacro(GeometryFilename);
  vtkGetStringMacro(GeometryFilename);

  // Description:
  // Specify the name of the displacement file to write.
  vtkSetStringMacro(DisplacementFilename);
  vtkGetStringMacro(DisplacementFilename);

  // Description:
  // Specify the name of the scalar file to write.
  vtkSetStringMacro(ScalarFilename);
  vtkGetStringMacro(ScalarFilename);

  // Description:
  // Specify the name of the texture file to write.
  vtkSetStringMacro(TextureFilename);
  vtkGetStringMacro(TextureFilename);

  // Description:
  // Turn on/off writing the displacement file.
  vtkSetMacro(WriteDisplacement,int);
  vtkGetMacro(WriteDisplacement,int);
  vtkBooleanMacro(WriteDisplacement,int);
  
  // Description:
  // Turn on/off writing the scalar file.
  vtkSetMacro(WriteScalar,int);
  vtkGetMacro(WriteScalar,int);
  vtkBooleanMacro(WriteScalar,int);
  
  // Description:
  // Turn on/off writing the texture file.
  vtkSetMacro(WriteTexture,int);
  vtkGetMacro(WriteTexture,int);
  vtkBooleanMacro(WriteTexture,int);

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

