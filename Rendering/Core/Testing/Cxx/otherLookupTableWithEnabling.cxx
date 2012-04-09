/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherLookupTableWithEnabling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the LookupTableWithEnabling

#include "vtkLookupTableWithEnabling.h"
#include "vtkDebugLeaks.h"

void TestOLT(vtkLookupTableWithEnabling *lut1)
{
  // actual test

  lut1->SetRange(1,1024);

  lut1->Allocate (1024);
  lut1->SetRampToLinear();
  lut1->Build();

  double rgb[4];
  lut1->GetColor(0, rgb);

  lut1->GetOpacity(0);

  lut1->GetTableValue(10,rgb);
  lut1->GetTableValue(10);

  unsigned char output[4*1024];

  int bitA = 0;
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_BIT,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_CHAR,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_CHAR,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_CHAR,
                                2, 1, VTK_LUMINANCE);

  bitA = 1;
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_BIT,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_CHAR,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_CHAR,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(&bitA, output, VTK_CHAR,
                                2, 1, VTK_LUMINANCE);


  char charA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(charA, output, VTK_CHAR,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(charA, output, VTK_CHAR,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(charA, output, VTK_CHAR,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(charA, output, VTK_CHAR,
                                2, 1, VTK_LUMINANCE);

  unsigned char ucharA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(ucharA, output, VTK_UNSIGNED_CHAR,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(ucharA, output, VTK_UNSIGNED_CHAR,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(ucharA, output, VTK_UNSIGNED_CHAR,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(ucharA, output, VTK_UNSIGNED_CHAR,
                                2, 1, VTK_LUMINANCE);

  int intA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(intA, output, VTK_INT,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(intA, output, VTK_INT,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(intA, output, VTK_INT,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(intA, output, VTK_INT,
                                2, 1, VTK_LUMINANCE);

  unsigned int uintA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(uintA, output, VTK_UNSIGNED_INT,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(uintA, output, VTK_UNSIGNED_INT,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(uintA, output, VTK_UNSIGNED_INT,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(uintA, output, VTK_UNSIGNED_INT,
                                2, 1, VTK_LUMINANCE);

  long longA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(longA, output, VTK_LONG,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(longA, output, VTK_LONG,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(longA, output, VTK_LONG,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(longA, output, VTK_LONG,
                                2, 1, VTK_LUMINANCE);

  unsigned long ulongA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(ulongA, output, VTK_UNSIGNED_LONG,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(ulongA, output, VTK_UNSIGNED_LONG,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(ulongA, output, VTK_UNSIGNED_LONG,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(ulongA, output, VTK_UNSIGNED_LONG, 2, 1, VTK_LUMINANCE);

  short shortA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(shortA, output, VTK_SHORT,
                                2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(shortA, output, VTK_SHORT,
                                2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(shortA, output, VTK_SHORT,
                                2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(shortA, output, VTK_SHORT,
                                2, 1, VTK_LUMINANCE);

  unsigned short ushortA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(ushortA, output,
                                VTK_UNSIGNED_SHORT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(ushortA, output,
                                VTK_UNSIGNED_SHORT, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(ushortA, output,
                                VTK_UNSIGNED_SHORT, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(ushortA, output,
                                VTK_UNSIGNED_SHORT, 2, 1, VTK_LUMINANCE);

  float floatA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(floatA, output,
                                VTK_FLOAT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(floatA, output,
                                VTK_FLOAT, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(floatA, output,
                                VTK_FLOAT, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(floatA, output,
                                VTK_FLOAT, 2, 1, VTK_LUMINANCE);

  double doubleA[2] = {0, 10};
  lut1->MapScalarsThroughTable2(doubleA, output,
                                VTK_DOUBLE, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2(doubleA, output,
                                VTK_DOUBLE, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2(doubleA, output,
                                VTK_DOUBLE, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2(doubleA, output,
                                VTK_DOUBLE, 2, 1, VTK_LUMINANCE);


}


int otherLookupTableWithEnabling(int,char *[])
{
  vtkLookupTableWithEnabling *lut1 = vtkLookupTableWithEnabling::New();
  lut1->SetAlpha(1.0);
  lut1->SetScaleToLinear();
  TestOLT(lut1);
  lut1->SetAlpha(.5);
  TestOLT(lut1);
  lut1->Delete();

  vtkLookupTableWithEnabling *lut2 = vtkLookupTableWithEnabling::New();
  lut2->SetAlpha(1.0);
  lut2->SetScaleToLog10();
  TestOLT(lut2);
  lut2->SetAlpha(.5);
  TestOLT(lut2);
  lut2->Delete();

  return 0;
}
