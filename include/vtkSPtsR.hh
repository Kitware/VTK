/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPtsR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPointsReader - read vtk structured points data file
// .SECTION Description
// vtkStructuredPointsReader is a source object that reads ASCII or binary 
// structured points data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkStructuredPointsReader_h
#define __vtkStructuredPointsReader_h

#include "SPtsSrc.hh"
#include "vtkDataR.hh"

class vtkStructuredPointsReader : public vtkStructuredPointsSource
{
public:
  vtkStructuredPointsReader();
  ~vtkStructuredPointsReader();
  char *GetClassName() {return "vtkStructuredPointsReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // overload because of vtkDataReader ivar
  unsigned long int GetMTime();

  void SetFilename(char *name);
  char *GetFilename();

  int GetFileType();

  void SetScalarsName(char *name);
  char *GetScalarsName();

  void SetVectorsName(char *name);
  char *GetVectorsName();

  void SetTensorsName(char *name);
  char *GetTensorsName();

  void SetNormalsName(char *name);
  char *GetNormalsName();

  void SetTCoordsName(char *name);
  char *GetTCoordsName();

  void SetLookupTableName(char *name);
  char *GetLookupTableName();

protected:
  void Execute();
  vtkDataReader Reader;
};

#endif


