/*=========================================================================

  Program:   Visualization Library
  Module:    Indent.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

// .NAME vlIndent - control print indentation
// .SECTION Description
// vlIndent is used to control indentation during the chaining print 
// process.

#ifndef __vlIndent_h
#define __vlIndent_h

#include <iostream.h>

class vlIndent
{
public:
  vlIndent(int ind=0) {this->Indent=ind;};
  vlIndent GetNextIndent();
  int Indent;

friend ostream& operator<<(ostream& os, vlIndent& ind);
};

#endif
