/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VoxelR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkVoxelReader
// .SECTION Description
// vtkVoxelReader reads a binary 0/1 bit voxel file. File is written by
// vtkVoxelModeller.

#ifndef __vtkVoxelReader_h
#define __vtkVoxelReader_h

#include <stdio.h>
#include "SPtsSrc.hh"
#include "BScalars.hh"

class vtkVoxelReader : public vtkStructuredPointsSource 
{
public:
  vtkVoxelReader():Filename(NULL) {};
  ~vtkVoxelReader() {if (this->Filename) delete [] this->Filename;};
  char *GetClassName() {return "vtkVoxelReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of the file to read.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

protected:
  char *Filename;
  void Execute();
};

#endif


