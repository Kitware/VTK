/*=========================================================================

  Program:   Visualization Library
  Module:    VoxelR.hh
  Language:  C++
  Date:      6/8/94
  Version:   1.1

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
//
// Read Voxel Data
//
#ifndef __vlVoxelReader_h
#define __vlVoxelReader_h

#include <stdio.h>
#include "SPtSrc.hh"
#include "BScalars.hh"

class vlVoxelReader : public vlStructuredPointsSource 
{
public:
  vlVoxelReader():Filename(NULL) {};
  ~vlVoxelReader() {if (this->Filename) delete [] this->Filename;};
  char *GetClassName() {return "vlVoxelReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  char *Filename;
  void Execute();
};

#endif


