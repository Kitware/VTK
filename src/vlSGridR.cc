/*=========================================================================

  Program:   Visualization Library
  Module:    vlSGridR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlSGridR.hh"

vlStructuredGridReader::vlStructuredGridReader()
{
  this->Filename = NULL;
}

vlStructuredGridReader::~vlStructuredGridReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlStructuredGridReader::Execute()
{

}

void vlStructuredGridReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredGridSource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}
