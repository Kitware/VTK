/*=========================================================================

  Program:   Visualization Library
  Module:    STLWrite.hh
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
//
// Write Stereo Lithography Files
//
#ifndef __vlSTLWriter_h
#define __vlSTLWriter_h

#include <stdio.h>
#include "Writer.hh"
#include "PolyF.hh"

#define STL_ASCII 0
#define STL_BINARY 1

class vlSTLWriter : public vlWriter, public vlPolyFilter
{
public:
  vlSTLWriter();
  ~vlSTLWriter();
  char *GetClassName() {return "vlSTLWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  vlSetClampMacro(WriteMode,int,STL_ASCII,STL_BINARY);
  vlGetMacro(WriteMode,int);

  void Write();

protected:
  void Execute() {this->Write();};

  char *Filename;
  int WriteMode;

  void WriteBinarySTL(vlPoints *pts, vlCellArray *polys);
  void WriteAsciiSTL(vlPoints *pts, vlCellArray *polys);
};

#endif

