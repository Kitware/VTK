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
//
// Read ASCII/Binary Stereo Lithography Files
//
#ifndef __vlSTLReader_h
#define __vlSTLReader_h

#include "PolySrc.hh"
#include <stdio.h>

class vlSTLReader : public vlPolySource 
{
public:
  vlSTLReader(char *fname=0);
  char *GetClassName() {return "vlSTLReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  char *Filename;
  void Execute();
  void ReadBinarySTL(FILE *fp);
  void ReadASCIISTL(FILE *fp);
  int GetSTLFileType(FILE *fp);
};

#endif


