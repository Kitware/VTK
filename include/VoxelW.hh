/*=========================================================================

  Program:   Visualization Library
  Module:    VoxelW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlVoxelWriter_h
#define __vlVoxelWriter_h

#include <stdio.h>
#include "Writer.hh"
#include "StrPtsF.hh"

class vlVoxelWriter : public vlWriter, public vlStructuredPointsFilter
{
public:
  vlVoxelWriter();
  ~vlVoxelWriter();
  char *GetClassName() {return "vlVoxelWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  void Write();

protected:
  void Execute() {this->Write();};

  char *Filename;
};

#endif

