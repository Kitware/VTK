/*=========================================================================

  Program:   Visualization Library
  Module:    ByteSwap.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ByteSwap.hh"

// Description:
// Swap four byte word.
void vlByteSwap::Swap4(char *mem_ptr1)
{
  char one_byte;

  one_byte    = mem_ptr1[0];
  mem_ptr1[0] = mem_ptr1[3];
  mem_ptr1[3] = one_byte;

  one_byte    = mem_ptr1[1];
  mem_ptr1[1] = mem_ptr1[2];
  mem_ptr1[2] = one_byte;
}
