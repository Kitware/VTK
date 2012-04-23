/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkFacetReader.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFacetReader - reads a dataset in Facet format
// .SECTION Description
// vtkFacetReader creates a poly data dataset. It reads ASCII files
// stored in Facet format
//
// The facet format looks like this:
// FACET FILE ...
// nparts
// Part 1 name
// 0
// npoints 0 0
// p1x p1y p1z
// p2x p2y p2z
// ...
// 1
// Part 1 name
// ncells npointspercell
// p1c1 p2c1 p3c1 ... pnc1 materialnum partnum
// p1c2 p2c2 p3c2 ... pnc2 materialnum partnum
// ...

#ifndef __vtkFacetReader_h
#define __vtkFacetReader_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSHYBRID_EXPORT vtkFacetReader : public vtkPolyDataAlgorithm
{
public:
  static vtkFacetReader *New();
  vtkTypeMacro(vtkFacetReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of Facet datafile to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  static int CanReadFile(const char *filename);

protected:
  vtkFacetReader();
  ~vtkFacetReader();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  char *FileName;

private:
  vtkFacetReader(const vtkFacetReader&);  // Not implemented.
  void operator=(const vtkFacetReader&);  // Not implemented.
};

#endif

