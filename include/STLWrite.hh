/*=========================================================================

  Program:   Visualization Library
  Module:    STLWrite.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlSTLWriter - write stereo lithography files
// .SECTION Description
// vlSTLWriter writes stereo lithography (.stl) files in either ASCII or 
// binary form.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// vlSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.

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

  // Description:
  // Specify the name of the file to write.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  // Description:
  // Specify type of file to write (ascii or binary).
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

