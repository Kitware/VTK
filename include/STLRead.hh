/*=========================================================================

  Program:   Visualization Library
  Module:    STLRead.hh
  Language:  C++
  Date:      7/20/94
  Version:   1.9

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlSTLReader - read ASCII or binary stereo lithography files
// .SECTION Description
// vlSTLReader is a source object that reads ASCII or binary stereo 
// lithography files (.stl files). The filename must be specified to
// vlSTLReader. The object automatically senses whether the file is
// ASCII or binary.
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
  vlSTLReader():Filename(NULL) {};
  ~vlSTLReader() {if (this->Filename) delete [] this->Filename;};
  char *GetClassName() {return "vlSTLReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  char *Filename;
  void Execute();
  int ReadBinarySTL(FILE *fp, vlFloatPoints*, vlCellArray*);
  int ReadASCIISTL(FILE *fp, vlFloatPoints*, vlCellArray*);
  int GetSTLFileType(FILE *fp);
};

#endif


