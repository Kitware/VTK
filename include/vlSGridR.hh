/*=========================================================================

  Program:   Visualization Library
  Module:    vlSGridR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGridReader - read vl structured grid data file
// .SECTION Description
// vlStructuredGridReader is a source object that reads ASCII or binary 
// structured grid data files in vl format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vlStructuredGridReader_h
#define __vlStructuredGridReader_h

#include "SGridSrc.hh"
#include "vlDataR.hh"

class vlStructuredGridReader : public vlStructuredGridSource, public vlDataReader
{
public:
  vlStructuredGridReader();
  ~vlStructuredGridReader();
  char *GetClassName() {return "vlStructuredGridReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of vl structured grid data file to read.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  void Execute();
  char *Filename;

};

#endif


