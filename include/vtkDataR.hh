/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataReader - helper class for objects that read vtk data files
// .SECTION Description
// vtkDataReader is a helper class that reads the vtk data file header and 
// point data (e.g., scalars, vectors, normals, etc) from a vtk data file. 
// See text for format.

#ifndef __vtkDataReader_hh
#define __vtkDataReader_hh

#include <stdio.h>
#include "Object.hh"
#include "PointSet.hh"

#define ASCII 1
#define BINARY 2


class vtkDataReader : public vtkObject
{
public:
  vtkDataReader();
  ~vtkDataReader();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of vtk data file to read.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // Get the type of file (ASCII or BINARY)
  vtkGetMacro(FileType,int);

  // Description:
  // Set the name of the scalar data to extract. If not specified, first 
  // scalar data encountered is extracted.
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);

  // Description:
  // Set the name of the vector data to extract. If not specified, first 
  // vector data encountered is extracted.
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);

  // Description:
  // Set the name of the tensor data to extract. If not specified, first 
  // tensor data encountered is extracted.
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);

  // Description:
  // Set the name of the normal data to extract. If not specified, first 
  // normal data encountered is extracted.
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);

  // Description:
  // Set the name of the texture coordinate data to extract. If not specified,
  // first texture coordinate data encountered is extracted.
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);

  // Description:
  // Set the name of the lookup table data to extract. If not specified, uses 
  // lookup table named by scalar. Otherwise, this specification supersedes.
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);

  // Special methods
  char *LowerCase(char *);
  FILE *OpenVLFile();
  int ReadHeader(FILE *fp);
  int ReadPointData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadPoints(FILE *fp, vtkPointSet *ps, int numPts);
  int ReadCells(FILE *fp, int size, int *data);
  void CloseVLFile(FILE *fp);

protected:
  char *Filename;
  int FileType;

  char *ScalarsName;
  char *VectorsName;
  char *TensorsName;
  char *TCoordsName;
  char *NormalsName;
  char *LookupTableName;
  char *ScalarLut;

  vtkSetStringMacro(ScalarLut);
  vtkGetStringMacro(ScalarLut);

  int ReadScalarData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadVectorData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadNormalData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadTensorData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadCoScalarData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadLutData(FILE *fp, vtkDataSet *ds, int numPts);
  int ReadTCoordsData(FILE *fp, vtkDataSet *ds, int numPts);
};

#endif


