/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Indent.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

// .NAME vtkIndent - control print indentation
// .SECTION Description
// vtkIndent is used to control indentation during the chaining print 
// process.

#ifndef __vtkIndent_h
#define __vtkIndent_h

#include <iostream.h>

class vtkIndent
{
public:
  vtkIndent(int ind=0) {this->Indent=ind;};
  vtkIndent GetNextIndent();
  int Indent;

friend ostream& operator<<(ostream& os, vtkIndent& ind);
};

#endif
