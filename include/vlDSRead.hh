/*=========================================================================

  Program:   Visualization Library
  Module:    vlDSRead.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSetReader - class to read any type of vl dataset
// .SECTION Description
// vlDataSetReader is a class that provides instance variables 
// and methods to read any type of dataset in visualization library format. 
// The output type of this class will vary depending upon the type of data
// file. Note: these formats are not standard. Use other more standard 
// formats when you can.

#ifndef __vlDataSetReader_h
#define __vlDataSetReader_h

#include "DSSrc.hh"
#include "vlDataR.hh"

class vlDataSetReader : public vlDataSetSource
{
public:
  vlDataSetReader();
  ~vlDataSetReader();
  char *GetClassName() {return "vlDataSetReader";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetFilename(char *name);
  char *GetFilename();

  void SetFileType(int type);
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
  vlDataReader Reader;
};

#endif


