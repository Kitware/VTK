/*=========================================================================

  Program:   Visualization Library
  Module:    STLRead.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "STLRead.hh"

void vlSTLReader::Execute()
{

}

void vlSTLReader::ReadBinarySTL(FILE *fp)
{

}

void vlSTLReader::ReadASCIISTL(FILE *fp)
{

}

int vlSTLReader::GetSTLFileType(FILE *fp)
{
  return 1;
}

void vlSTLReader::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlSTLReader::GetClassName()))
    {
    vlPolySource::PrintSelf(os,indent);

    os << indent << "Filename: " << this->Filename << "\n";
    }
}
