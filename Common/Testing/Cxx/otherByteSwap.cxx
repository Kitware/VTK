/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherByteSwap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the byte swapper

#include "vtkByteSwap.h"
#include "vtkDebugLeaks.h"

int TestByteSwap(ostream& strm)
{
  // actual test
  strm << "Test vtkByteSwap Start" << endl;
  
  char check[1024];
  short sword[2];
  char cword[1024];
  unsigned short usword[2];

  memcpy ((char *) sword, "abcd", 2);
  vtkByteSwap::Swap2BE(sword);
  memcpy ((char *) check, sword, 2);
  strm << "Swap2BE(short \"ab\") -> " << (char) check[0] << (char) check[1] << endl;

  memcpy ((char *) usword, "abcd", 2);
  vtkByteSwap::Swap2BE(usword);
  memcpy ((char *) check, usword, 2);
  strm << "Swap2BE(unsigned short \"ab\") -> " << (char) check[0] << (char) check[1] << endl;

  memcpy ((char *) cword, "abcd", 4);
  vtkByteSwap::Swap4BE(cword);
  memcpy ((char *) check, cword, 4);
  strm << "Swap4BE(char *\"abcd\") -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap8BE(cword);
  memcpy ((char *) check, cword, 8);
  strm << "Swap8BE(char *\"abcdefgh\") -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;


  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap2BERange(cword,8);
  memcpy ((char *) check, cword, 8);
  strm << "Swap2BERange(char *\"abcdefgh\",8) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap4BERange(cword,8);
  memcpy ((char *) check, cword, 8);
  strm << "Swap4BERange(char *\"abcdefgh\",8) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap8BERange(cword,8);
  memcpy ((char *) check, cword, 8);
  strm << "Swap8BERange(char *\"abcdefgh\",8) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefghijklmnop", 16);
  strm << "SwapWrite2BERange(char *\"abcdefghijklmnop\",8,stdout) -> ";
  vtkByteSwap::SwapWrite2BERange(cword,8,stdout);
  strm << endl;

  memcpy ((char *) cword, "abcdefghijklmnop", 16);
  strm << "SwapWrite4BERange(char *\"abcdefghijklmnop\",4,stdout) -> ";
  vtkByteSwap::SwapWrite4BERange(cword,4,stdout);
  strm << endl;

  memcpy ((char *) cword, "abcdefghijklmnop", 16);
  strm << "SwapWrite8BERange(char *\"abcdefghijklmnop\",2,stdout) -> ";
  vtkByteSwap::SwapWrite8BERange(cword,2,stdout);
  strm << endl;

  memcpy ((char *) cword, "abcdefghijklmnop", 16);
  strm << "SwapWrite2BERange(char *\"abcdefghijklmnop\",8,&strm) -> ";
  vtkByteSwap::SwapWrite2BERange(cword,8,&strm);
  strm << endl;

  memcpy ((char *) cword, "abcdefghijklmnop", 16);
  strm << "SwapWrite4BERange(char *\"abcdefghijklmnop\",4,&strm) -> ";
  vtkByteSwap::SwapWrite4BERange(cword,4,&strm);
  strm << endl;

  memcpy ((char *) cword, "abcdefghijklmnop", 16);
  strm << "SwapWrite8BERange(char *\"abcdefghijklmnop\",2,&strm) -> ";
  vtkByteSwap::SwapWrite8BERange(cword,2,&strm);
  strm << endl;

  memcpy ((char *) sword, "abcd", 2);
  vtkByteSwap::Swap2LE(sword);
  memcpy ((char *) check, sword, 2);
  strm << "Swap2LE(short \"ab\") -> " << (char) check[0] << (char) check[1] << endl;

  memcpy ((char *) usword, "abcd", 2);
  vtkByteSwap::Swap2LE(usword);
  memcpy ((char *) check, usword, 2);
  strm << "Swap2LE(unsigned short \"ab\") -> " << (char) check[0] << (char) check[1] << endl;

  memcpy ((char *) cword, "abcd", 4);
  vtkByteSwap::Swap4LE(cword);
  memcpy ((char *) check, cword, 4);
  strm << "Swap4LE(char *\"abcd\") -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap8LE(cword);
  memcpy ((char *) check, cword, 8);
  strm << "Swap8LE(char *\"abcdefgh\") -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;


  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap2LERange(cword,8);
  memcpy ((char *) check, cword, 8);
  strm << "Swap2LERange(char *\"abcdefgh\",8) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap4LERange(cword,8);
  memcpy ((char *) check, cword, 8);
  strm << "Swap4LERange(char *\"abcdefgh\",8) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::Swap8LERange(cword,8);
  memcpy ((char *) check, cword, 8);
  strm << "Swap8LERange(char *\"abcdefgh\",8) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::SwapVoidRange(cword,4,2);
  memcpy ((char *) check, cword, 8);
  strm << "SwapVoidRange(char *\"abcdefgh\",4,2) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::SwapVoidRange(cword,2,4);
  memcpy ((char *) check, cword, 8);
  strm << "SwapVoidRange(char *\"abcdefgh\",2,4) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  memcpy ((char *) cword, "abcdefgh", 8);
  vtkByteSwap::SwapVoidRange(cword,1,8);
  memcpy ((char *) check, cword, 8);
  strm << "SwapVoidRange(char *\"abcdefgh\",1,8) -> " << (char) check[0] << (char) check[1]  << (char) check[2] << (char) check[3] << (char) check[4] << (char) check[5]  << (char) check[6] << (char) check[7] << endl;

  strm << "Test vtkByteSwap End" << endl;
  return 0;
}


int otherByteSwap(int,char *[])
{
#ifndef VTK_LEGACY_REMOVE
  vtkDebugLeaks::PromptUserOff();
#endif

  ostrstream vtkmsg_with_warning_C4701; 
  return TestByteSwap(vtkmsg_with_warning_C4701);
} 
