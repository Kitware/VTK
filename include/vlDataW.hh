/*=========================================================================

  Program:   Visualization Library
  Module:    vlDataW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataWriter - helper class for objects that write vl data files
// .SECTION Description
// vlDataWriter is a helper class that opens and writes the vl header and 
// point data (e.g., scalars, vectors, normals, etc) from a vl data file. 
// See text for various formats.

#ifndef __vlDataWriter_hh
#define __vlDataWriter_hh

#include <stdio.h>
#include "Writer.hh"
#include "DataSet.hh"

#define ASCII 1
#define BINARY 2

class vlDataWriter : public vlWriter
{
public:
  vlDataWriter();
  ~vlDataWriter();
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of vl polygon data file to write.
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  // Description:
  // Specify the header for the vl data file.
  vlSetStringMacro(Header);
  vlGetStringMacro(Header);

  // Description:
  // Specify file type (ASCII or BINARY) for vl data file.
  vlSetClampMacro(FileType,int,ASCII,BINARY);
  vlGetMacro(FileType,int);

  // Description:
  // Give a name to the scalar data. If not specified, uses default
  // name "scalars".
  vlSetStringMacro(ScalarsName);
  vlGetStringMacro(ScalarsName);

  // Description:
  // Give a name to the vector data. If not specified, uses default
  // name "vectors".
  vlSetStringMacro(VectorsName);
  vlGetStringMacro(VectorsName);

  // Description:
  // Give a name to the tensors data. If not specified, uses default
  // name "tensors".
  vlSetStringMacro(TensorsName);
  vlGetStringMacro(TensorsName);

  // Description:
  // Give a name to the normals data. If not specified, uses default
  // name "normals".
  vlSetStringMacro(NormalsName);
  vlGetStringMacro(NormalsName);

  // Description:
  // Give a name to the texture coordinates data. If not specified, uses 
  // default name "textureCoords".
  vlSetStringMacro(TCoordsName);
  vlGetStringMacro(TCoordsName);

  // Description:
  // Give a name to the lookup table. If not specified, uses default
  // name "lookupTable".
  vlSetStringMacro(LookupTableName);
  vlGetStringMacro(LookupTableName);

  FILE *OpenVLFile();
  int WriteHeader(FILE *fp);
  int WritePoints(FILE *fp, vlPoints *p);
  int WriteCells(FILE *fp, vlCellArray *cells, char *label);
  int WritePointData(FILE *fp, vlDataSet *ds);
  void CloseVLFile(FILE *fp);

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

  int WriteScalarData(FILE *fp, vlScalars *s, int numPts);
  int WriteVectorData(FILE *fp, vlVectors *v, int numPts);
  int WriteNormalData(FILE *fp, vlNormals *n, int numPts);
  int WriteTCoordData(FILE *fp, vlTCoords *tc, int numPts);
  int WriteTensorData(FILE *fp, vlTensors *t, int numPts);

};

#endif


