/*=========================================================================

  Program:   Visualization Library
  Module:    VoxelR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlVoxelReader
// .SECTION Description
// vlVoxelReader reads a binary 0/1 bit voxel file. File is written by
// vlVoxelModeller.

#ifndef __vlVoxelReader_h
#define __vlVoxelReader_h

#include <stdio.h>
#include "SPtsSrc.hh"
#include "BScalars.hh"

class vlVoxelReader : public vlStructuredPointsSource 
{
public:
  vlVoxelReader():Filename(NULL) {};
  ~vlVoxelReader() {if (this->Filename) delete [] this->Filename;};
  char *GetClassName() {return "vlVoxelReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the name of the file to read.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  char *Filename;
  void Execute();
};

#endif


