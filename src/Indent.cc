/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Indent.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Indent.hh"
#define vtkStdIndent 2
#define NumberOfBlanks 40

static char blanks[NumberOfBlanks+1]="                                        ";

// Description:
// Determine the next indentation level.
vtkIndent vtkIndent::GetNextIndent()
{
  int indent = this->Indent + vtkStdIndent;
  if ( indent > NumberOfBlanks ) indent = NumberOfBlanks;
  return indent;
}

// Description:
// Print out the indentation.
ostream& operator<<(ostream& os, vtkIndent& ind)
{
  os << blanks + (NumberOfBlanks-ind.Indent) ;
  return os;
}
