/*=========================================================================

  Program:   Visualization Library
  Module:    CyReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CyReader.hh"

vlCyberReader::vlCyberReader()
{
  this->Filename = NULL;
}

vlCyberReader::~vlCyberReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlCyberReader::Execute()
{

}
