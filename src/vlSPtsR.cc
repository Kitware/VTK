/*=========================================================================

  Program:   Visualization Library
  Module:    vlSPtsR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlSPtsR.hh"

vlStructuredPointsReader::vlStructuredPointsReader()
{
  this->Filename = NULL;
}

vlStructuredPointsReader::~vlStructuredPointsReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlStructuredPointsReader::Execute()
{

}

void vlStructuredPointsReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}
