/*=========================================================================

  Program:   Visualization Library
  Module:    ByteSwap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Class for performing common math operations (e.g., dot, cross products)
//
#ifndef __vlByteSwap_hh
#define __vlByteSwap_hh

class vlByteSwap
{
public:
  Swap4(char *c);
  Swap4(float *p) {Swap4((char *)p);};
  Swap4(int *i) {Swap4((char *)i);};
  Swap4(unsigned long *i) {Swap4((char *)i);};
};

#endif
