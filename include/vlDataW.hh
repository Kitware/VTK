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
// See text for various format.

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
  vlSetClampMacro(Type,int,ASCII,BINARY);
  vlGetMacro(Type,int);

  FILE *OpenVLFile(char *filename, int debug=0);
  int WriteHeader(FILE *fp);
  int WritePointData(FILE *fp, vlDataSet *ds);

protected:
  char *Filename;
  char *Header;
  int Type;

};

#endif


