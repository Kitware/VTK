/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkByteSwap.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkByteSwap - perform machine dependent byte swapping
// .SECTION Description
// vtkByteSwap is used by other classes to perform machine dependent byte
// swapping. Byte swapping is often used when reading or writing binary 
// files.
// .EXAMPLE STLRead.cc

#ifndef __vtkByteSwap_h
#define __vtkByteSwap_h
#include <stdio.h>

#include "vtkWin32Header.h"

class VTK_EXPORT vtkByteSwap
{
public:
  void Delete() {delete this;};

  static void Swap4LE(char *c);
  static void Swap4LE(float *p) { vtkByteSwap::Swap4LE((char *)p);};
  static void Swap4LE(int *i)   { vtkByteSwap::Swap4LE((char *)i);};
  static void Swap4LE(unsigned long *i) { Swap4LE((char *)i);};

  static void Swap4LERange(char *c,int num);
  static void Swap4LERange(float *p,int num) 
  { vtkByteSwap::Swap4LERange((char *)p,num);};
  static void Swap4LERange(int *i,int num) 
  { vtkByteSwap::Swap4LERange((char *)i,num);};
  static void Swap4LERange(unsigned long *i,int num) 
  { vtkByteSwap::Swap4LERange((char *)i,num);};

  static void Swap4BE(char *c);
  static void Swap4BE(float *p) { vtkByteSwap::Swap4BE((char *)p);};
  static void Swap4BE(int *i)   { vtkByteSwap::Swap4BE((char *)i);};
  static void Swap4BE(unsigned long *i) { vtkByteSwap::Swap4BE((char *)i);};

  static void Swap4BERange(char *c,int num);
  static void Swap4BERange(float *p,int num) 
  { vtkByteSwap::Swap4BERange((char *)p,num); };
  static void Swap4BERange(int *i,int num) 
  { vtkByteSwap::Swap4BERange((char *)i,num); };
  static void Swap4BERange(unsigned long *i,int num) 
  { vtkByteSwap::Swap4BERange((char *)i,num); };

  static void SwapWrite4BERange(char *c,int num,FILE *fp);
  static void SwapWrite4BERange(float *p,int num, FILE *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)p,num,fp);};
  static void SwapWrite4BERange(int *i,int num,FILE *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};
  static void SwapWrite4BERange(unsigned long *i,int num, FILE *fp) 
  { vtkByteSwap::SwapWrite4BERange((char *)i,num,fp);};

  static void Swap2BERange(char *c,int num);
  static void Swap2LERange(char *c,int num);
  static void Swap2BERange(short *i,int num) 
  { vtkByteSwap::Swap2BERange((char *)i,num);};
  static void Swap2LERange(short *i,int num) 
  { vtkByteSwap::Swap2LERange((char *)i,num);};

  static void SwapWrite2BERange(char *c,int num,FILE *fp);
  static void SwapWrite2BERange(short *i,int num, FILE *fp) 
  {vtkByteSwap::SwapWrite2BERange((char *)i,num,fp);};
};

#endif
