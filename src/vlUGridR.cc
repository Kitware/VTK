/*=========================================================================

  Program:   Visualization Library
  Module:    vlUGridR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlUGridR.hh"

vlUnstructuredGridReader::vlUnstructuredGridReader()
{
  this->Filename = NULL;
}

vlUnstructuredGridReader::~vlUnstructuredGridReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlUnstructuredGridReader::Execute()
{

}

void vlUnstructuredGridReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlUnstructuredGridSource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}
