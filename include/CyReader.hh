/*=========================================================================

  Program:   Visualization Library
  Module:    CyReader.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCyberReader - read Cyberware laser digitizer files
// .SECTION Description
// vlCyberReader is a source object that reads a Cyberware laser digitizer
// file. (Original source code provided coutesy of Cyberware, Inc.)

#ifndef __vlCyberReader_h
#define __vlCyberReader_h

#include <stdio.h>
#include "PolySrc.hh"
#include "FPoints.hh"
#include "CellArr.hh"

class vlCyberReader : public vlPolySource 
{
public:
  vlCyberReader();
  ~vlCyberReader();
  char *GetClassName() {return "vlCyberReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify Cyberware file name.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  char *Filename;

  void Execute();
};

#endif


