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
// .NAME vlVoxelWriter - write out 0/1 voxel data from vlVoxelModeller
// .SECTION Description
// vlVoxelWriter writes a binary 0/1 voxel file. vlVoxelWriter writes only
// structured points data.

#ifndef __vlVoxelWriter_h
#define __vlVoxelWriter_h

#include <stdio.h>
#include "Writer.hh"
#include "StrPts.hh"

class vlVoxelWriter : public vlWriter
{
public:
  vlVoxelWriter();
  ~vlVoxelWriter();
  char *GetClassName() {return "vlVoxelWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetInput(vlStructuredPoints *input);
  void SetInput(vlStructuredPoints &input) {this->SetInput(&input);};
  vlStructuredPoints *GetInput() {return (vlStructuredPoints *)this->Input;};

  // Description:
  // Specify name of file to write.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  void WriteData();

  char *Filename;
};

#endif

