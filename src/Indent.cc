/*=========================================================================

  Program:   Visualization Library
  Module:    Indent.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Indent.hh"
#define vlStdIndent 2
#define NumberOfBlanks 40

static char blanks[NumberOfBlanks+1]="                                        ";

vlIndent vlIndent::GetNextIndent()
{
  int indent = this->Indent + vlStdIndent;
  if ( indent > NumberOfBlanks ) indent = NumberOfBlanks;
  return indent;
}

ostream& operator<<(ostream& os, vlIndent& ind)
{
  os << blanks + (NumberOfBlanks-ind.Indent) ;
  return os;
}
