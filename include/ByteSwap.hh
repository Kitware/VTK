/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ByteSwap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkByteSwap - perform machine dependent byte swapping
// .SECTION Description
// vtkByteSwap is used by other classes to perform machine dependent byte
// swapping. Byte swapping is often used when reading or writing binary 
// files.
// .EXAMPLE STLRead.cc

#ifndef __vtkByteSwap_hh
#define __vtkByteSwap_hh

class vtkByteSwap
{
public:
  void Delete() {delete this;};
  void Swap4(char *c);
  void Swap4(float *p) {Swap4((char *)p);};
  void Swap4(int *i) {Swap4((char *)i);};
  void Swap4(unsigned long *i) {Swap4((char *)i);};
};

#endif
