/*=========================================================================

  Program:   Visualization Library
  Module:    vlDSRead.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlDSRead.hh"

vlDataSetReader::vlDataSetReader()
{
  this->Filename = NULL;
}

vlDataSetReader::~vlDataSetReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlDataSetReader::Execute()
{

}

void vlDataSetReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetSource::PrintSelf(os,indent);
  os << indent << "Filename: " << this->Filename << "\n";
}
