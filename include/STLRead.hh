/*=========================================================================

  Program:   Visualization Toolkit
  Module:    STLRead.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkSTLReader - read ASCII or binary stereo lithography files
// .SECTION Description
// vtkSTLReader is a source object that reads ASCII or binary stereo 
// lithography files (.stl files). The filename must be specified to
// vtkSTLReader. The object automatically detects whether the file is
// ASCII or binary.
//     .stl files are quite inefficient and duplicate vertex definitions. 
// By setting the Merging boolean you can control wether the point data
// is merged after reading. Merging is performed by default, however,
// merging requires a large amount of temporary storage since a 3D hash
// table must be constructed.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// vtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.

#ifndef __vtkSTLReader_h
#define __vtkSTLReader_h

#include <stdio.h>
#include "PolySrc.hh"
#include "FPoints.hh"
#include "CellArr.hh"

class vtkSTLReader : public vtkPolySource 
{
public:
  vtkSTLReader();
  ~vtkSTLReader();
  char *GetClassName() {return "vtkSTLReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of stereo lithography file.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // Turn on/off merging of points/triangles.
  vtkSetMacro(Merging,int);
  vtkGetMacro(Merging,int);
  vtkBooleanMacro(Merging,int);

  void SetLocator(vtkLocator *locator);
  void SetLocator(vtkLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  char *Filename;
  int Merging;
  vtkLocator *Locator;
  int SelfCreatedLocator;

  void Execute();
  int ReadBinarySTL(FILE *fp, vtkFloatPoints*, vtkCellArray*);
  int ReadASCIISTL(FILE *fp, vtkFloatPoints*, vtkCellArray*);
  int GetSTLFileType(FILE *fp);
};

#endif


