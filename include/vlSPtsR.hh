/*=========================================================================

  Program:   Visualization Library
  Module:    vlSPtsR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPointsReader - read vl structured points data file
// .SECTION Description
// vlStructuredPointsReader is a source object that reads ASCII or binary 
// structured points data files in vl format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vlStructuredPointsReader_h
#define __vlStructuredPointsReader_h

#include "SPtsSrc.hh"
#include "vlDataR.hh"

class vlStructuredPointsReader : public vlStructuredPointsSource, public vlDataReader
{
public:
  vlStructuredPointsReader();
  ~vlStructuredPointsReader();
  char *GetClassName() {return "vlStructuredPointsReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of vl structured points data file to read.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  void Execute();
  char *Filename;
};

#endif


