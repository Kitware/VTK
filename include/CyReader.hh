/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CyReader.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCyberReader - read Cyberware laser digitizer files
// .SECTION Description
// vtkCyberReader is a source object that reads a Cyberware laser digitizer
// file. (Original source code provided coutesy of Cyberware, Inc.)

#ifndef __vtkCyberReader_h
#define __vtkCyberReader_h

#include <stdio.h>
#include "PolySrc.hh"
#include "FPoints.hh"
#include "CellArr.hh"

class vtkCyberReader : public vtkPolySource 
{
public:
  vtkCyberReader();
  ~vtkCyberReader();
  char *GetClassName() {return "vtkCyberReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify Cyberware file name.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

protected:
  char *Filename;

  void Execute();
};

#endif


