/*=========================================================================

  Program:   Visualization Library
  Module:    vlPolyR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyReader - read vl polygonal data file
// .SECTION Description
// vlPolyReader is a source object that reads ASCII or binary 
// polygonal data files in vl format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vlPolyReader_h
#define __vlPolyReader_h

#include "PolySrc.hh"
#include "vlDataR.hh"

class vlPolyReader : public vlPolySource, vlDataReader
{
public:
  vlPolyReader();
  ~vlPolyReader();
  char *GetClassName() {return "vlPolyReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of vl unstructured grid data file to read.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  void Execute();
  char *Filename;
};

#endif


