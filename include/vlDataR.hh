/*=========================================================================

  Program:   Visualization Library
  Module:    vlDataR.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataReader - helper class for objects that read vl data files
// .SECTION Description
// vlDataReader is a helper class that reads the vl data file header and 
// point data (e.g., scalars, vectors, normals, etc) from a vl data file. 
// See text for format.

#ifndef __vlDataReader_hh
#define __vlDataReader_hh

#include <stdio.h>
#include "Object.hh"
#include "PointSet.hh"

#define ASCII 1
#define BINARY 2


class vlDataReader : public vlObject
{
public:
  vlDataReader();
  ~vlDataReader();
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of vl data file to read.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  // Description:
  // Get the type of file (ASCII or BINARY)
  vlGetMacro(FileType,int);

  // Description:
  // Set the name of the scalar data to extract. If not specified, first 
  // scalar data encountered is extracted.
  vlSetStringMacro(ScalarsName);
  vlGetStringMacro(ScalarsName);

  // Description:
  // Set the name of the vector data to extract. If not specified, first 
  // vector data encountered is extracted.
  vlSetStringMacro(VectorsName);
  vlGetStringMacro(VectorsName);

  // Description:
  // Set the name of the tensor data to extract. If not specified, first 
  // tensor data encountered is extracted.
  vlSetStringMacro(TensorsName);
  vlGetStringMacro(TensorsName);

  // Description:
  // Set the name of the normal data to extract. If not specified, first 
  // normal data encountered is extracted.
  vlSetStringMacro(NormalsName);
  vlGetStringMacro(NormalsName);

  // Description:
  // Set the name of the texture coordinate data to extract. If not specified,
  // first texture coordinate data encountered is extracted.
  vlSetStringMacro(TCoordsName);
  vlGetStringMacro(TCoordsName);

  // Description:
  // Set the name of the lookup table data to extract. If not specified, uses 
  // lookup table named by scalar. Otherwise, this specification supersedes.
  vlSetStringMacro(LookupTableName);
  vlGetStringMacro(LookupTableName);

  // Special methods
  char *LowerCase(char *);
  FILE *OpenVLFile(int debug);
  int ReadHeader(FILE *fp, int debug);
  int ReadPointData(FILE *fp, vlDataSet *ds, int numPts, int debug);
  int ReadPoints(FILE *fp, vlPointSet *ps, int numPts);
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

  vlSetStringMacro(ScalarLut);
  vlGetStringMacro(ScalarLut);

  int ReadScalarData(FILE *fp, vlDataSet *ds, int numPts);
  int ReadVectorData(FILE *fp, vlDataSet *ds, int numPts);
  int ReadNormalData(FILE *fp, vlDataSet *ds, int numPts);
  int ReadTensorData(FILE *fp, vlDataSet *ds, int numPts);
  int ReadCoScalarData(FILE *fp, vlDataSet *ds, int numPts);
  int ReadLutData(FILE *fp, vlDataSet *ds, int numPts);
  int ReadTCoordsData(FILE *fp, vlDataSet *ds, int numPts);
};

#endif


