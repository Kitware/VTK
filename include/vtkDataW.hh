/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataWriter - helper class for objects that write vtk data files
// .SECTION Description
// vtkDataWriter is a helper class that opens and writes the vtk header and 
// point data (e.g., scalars, vectors, normals, etc) from a vtk data file. 
// See text for various formats.

#ifndef __vtkDataWriter_hh
#define __vtkDataWriter_hh

#include <stdio.h>
#include "Writer.hh"
#include "DataSet.hh"

#define ASCII 1
#define BINARY 2

class vtkDataWriter : public vtkWriter
{
public:
  vtkDataWriter();
  ~vtkDataWriter();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of vtk polygon data file to write.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // Specify the header for the vtk data file.
  vtkSetStringMacro(Header);
  vtkGetStringMacro(Header);

  // Description:
  // Specify file type (ASCII or BINARY) for vtk data file.
  vtkSetClampMacro(FileType,int,ASCII,BINARY);
  vtkGetMacro(FileType,int);

  // Description:
  // Give a name to the scalar data. If not specified, uses default
  // name "scalars".
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);

  // Description:
  // Give a name to the vector data. If not specified, uses default
  // name "vectors".
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);

  // Description:
  // Give a name to the tensors data. If not specified, uses default
  // name "tensors".
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);

  // Description:
  // Give a name to the normals data. If not specified, uses default
  // name "normals".
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);

  // Description:
  // Give a name to the texture coordinates data. If not specified, uses 
  // default name "textureCoords".
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);

  // Description:
  // Give a name to the lookup table. If not specified, uses default
  // name "lookupTable".
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);

  FILE *OpenVTKFile();
  int WriteHeader(FILE *fp);
  int WritePoints(FILE *fp, vtkPoints *p);
  int WriteCells(FILE *fp, vtkCellArray *cells, char *label);
  int WritePointData(FILE *fp, vtkDataSet *ds);
  void CloseVTKFile(FILE *fp);

protected:
  char *Filename;
  char *Header;
  int FileType;

  char *ScalarsName;
  char *VectorsName;
  char *TensorsName;
  char *TCoordsName;
  char *NormalsName;
  char *LookupTableName;

  int WriteScalarData(FILE *fp, vtkScalars *s, int numPts);
  int WriteVectorData(FILE *fp, vtkVectors *v, int numPts);
  int WriteNormalData(FILE *fp, vtkNormals *n, int numPts);
  int WriteTCoordData(FILE *fp, vtkTCoords *tc, int numPts);
  int WriteTensorData(FILE *fp, vtkTensors *t, int numPts);

};

#endif


