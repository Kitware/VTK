/*=========================================================================

  Program:   Visualization Library
  Module:    vlDataR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlDataR.hh"

#define ASCII 1
#define BINARY 2

// Description:
// Open a vl data file. Returns NULL if error.
FILE *vlDataReader::OpenVLFile(char *filename, int debug)
{
  return NULL;
}

// Description:
// Read the header of a vl data file. Returns 0 if error.
int vlDataReader::ReadHeader(FILE *fp, int debug)
{
  return 1;
}

// Description:
// Read the point data of a vl data file. The number of points (from the 
// dataset) must match the number of points defined in point attributes unless
// numPts<=0, which means no points were defined in the dataset. Returns 0 if 
// error.
int vlDataReader::ReadPointData(FILE *fp, vlDataSet *ds, int numPts, int debug)
{
  return 1;
}
