/*=========================================================================

  Program:   Visualization Library
  Module:    vlPolyR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlPolyR.hh"

vlPolyReader::vlPolyReader()
{
  this->Filename = NULL;
}

vlPolyReader::~vlPolyReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlPolyReader::Execute()
{
}

void vlPolyReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolySource::PrintSelf(os,indent);
  os << indent << "Filename: " << this->Filename << "\n";
}
