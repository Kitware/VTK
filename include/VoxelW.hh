/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VoxelW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkVoxelWriter - write out 0/1 voxel data from vtkVoxelModeller
// .SECTION Description
// vtkVoxelWriter writes a binary 0/1 voxel file. vtkVoxelWriter writes only
// structured points data.

#ifndef __vtkVoxelWriter_h
#define __vtkVoxelWriter_h

#include <stdio.h>
#include "Writer.hh"
#include "StrPts.hh"

class vtkVoxelWriter : public vtkWriter
{
public:
  vtkVoxelWriter();
  ~vtkVoxelWriter();
  char *GetClassName() {return "vtkVoxelWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkStructuredPoints *input);
  void SetInput(vtkStructuredPoints &input) {this->SetInput(&input);};
  vtkStructuredPoints *GetInput() {return (vtkStructuredPoints *)this->Input;};

  // Description:
  // Specify name of file to write.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

protected:
  void WriteData();

  char *Filename;
};

#endif

