/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUGrdR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkUnstructuredGridReader - read vtk unstructured grid data file
// .SECTION Description
// vtkUnstructuredGridReader is a source object that reads ASCII or binary 
// unstructured grid data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkUnstructuredGridReader_h
#define __vtkUnstructuredGridReader_h

#include "UGridSrc.hh"
#include "vtkDataR.hh"

class vtkUnstructuredGridReader : public vtkUnstructuredGridSource
{
public:
  vtkUnstructuredGridReader();
  ~vtkUnstructuredGridReader();
  char *GetClassName() {return "vtkUnstructuredGridReader";};
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


