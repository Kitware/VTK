/*=========================================================================

  Program:   Visualization Library
  Module:    vlDataW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlDataW.hh"

// Description:
// Created object with default header and ASCII format.
vlDataWriter::vlDataWriter()
{
  this->Filename = NULL;

  this->Header = new char[256];
  strcpy(this->Header,"vl output");

  this->Type = ASCII;
}

vlDataWriter::~vlDataWriter()
{
  if ( this->Filename ) delete [] this->Filename;
  if ( this->Header ) delete [] this->Header;
}

// Description:
// Open a vl data file. Returns NULL if error.
FILE *vlDataWriter::OpenVLFile(char *filename, int debug)
{
  if (debug) this->DebugOn();
  else this->DebugOff();

  return NULL;
}

// Description:
// Write the header of a vl data file. Returns 0 if error.
int vlDataWriter::WriteHeader(FILE *fp)
{
  vlDebugMacro(<<"Writing header...");

  return 1;
}

// Description:
// Write the point data (e.g., scalars, vectors, ...) of a vl data file. 
// Returns 0 if error.
int vlDataWriter::WritePointData(FILE *fp, vlDataSet *ds)
{
  vlDebugMacro(<<"Writing point data...");

  return 1;
}
