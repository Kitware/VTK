/*=========================================================================

  Program:   Visualization Library
  Module:    STLRead.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlSTLReader - read ASCII or binary stereo lithography files
// .SECTION Description
// vlSTLReader is a source object that reads ASCII or binary stereo 
// lithography files (.stl files). The filename must be specified to
// vlSTLReader. The object automatically detects whether the file is
// ASCII or binary.
//     .stl files are quite inefficient and duplicate vertex definitions. 
// By setting the Merging boolean you can control wether the point data
// is merged after reading. Merging is performed by default, however,
// merging requires a large amount of temporary storage since a 3D hash
// table must be constructed.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// vlSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.

#ifndef __vlSTLReader_h
#define __vlSTLReader_h

#include <stdio.h>
#include "PolySrc.hh"
#include "FPoints.hh"
#include "CellArr.hh"

class vlSTLReader : public vlPolySource 
{
public:
  vlSTLReader();
  ~vlSTLReader();
  char *GetClassName() {return "vlSTLReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of stereo lithography file.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  // Description:
  // Turn on/off merging of points/triangles.
  vlSetMacro(Merging,int);
  vlGetMacro(Merging,int);
  vlBooleanMacro(Merging,int);

  void SetLocator(vlLocator *locator);
  void SetLocator(vlLocator& locator) {this->SetLocator(&locator);};
  vlGetObjectMacro(Locator,vlLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  char *Filename;
  int Merging;
  vlLocator *Locator;
  int SelfCreatedLocator;

  void Execute();
  int ReadBinarySTL(FILE *fp, vlFloatPoints*, vlCellArray*);
  int ReadASCIISTL(FILE *fp, vlFloatPoints*, vlCellArray*);
  int GetSTLFileType(FILE *fp);
};

#endif


