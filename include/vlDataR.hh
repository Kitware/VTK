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
#include "DataSet.hh"

#define ASCII 1
#define BINARY 2

// Special read macros
#define vlReadDebugMacro(x) \
  cerr << "Debug: In " __FILE__ << ", line " << __LINE__ << "\n" << "vlDataReader" << " (" << this << "): " x << "\n\n"

#define vlReadWarningMacro(x) \
  cerr << "Warning: In " __FILE__ << ", line " << __LINE__ << "\n" << "vlDataReader" << " (" << this << "): " x << "\n\n"

#define vlReadErrorMacro(x) \
  cerr << "ERROR In " __FILE__ << ", line " << __LINE__ << "\n" << "vlDataReader" << " (" << this << "): " x << "\n\n"


class vlDataReader
{
public:
  vlDataReader() {};
  ~vlDataReader() {};

  FILE *OpenVLFile(char *filename, int debug);
  int ReadHeader(FILE *fp, int debug);
  int ReadPointData(FILE *fp, vlDataSet *ds, int numPts, int debug);

protected:
  int FileType;
  char *LowerCase(char *);

};

#endif


