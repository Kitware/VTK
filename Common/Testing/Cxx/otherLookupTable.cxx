/*==========================================================================

  Program: 
  Module:    otherLookupTable.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the LookupTable

#include "vtkLookupTable.h"
#include "vtkLogLookupTable.h"
#include "vtkDebugLeaks.h"

void Test(ostream& strm, vtkLookupTable *lut1)
{
  // actual test
 
  float range[2] = {0, 1023};
  lut1->SetRange(0,1023);
  lut1->SetScaleToLinear();
  

  lut1->Allocate (1024);
  lut1->SetRampToLinear();
  lut1->Build();

  float rgb[4], *rgb2;
  lut1->GetColor(0, rgb);
  rgb2 = lut1->GetColor(0);

  float opacity;
  opacity = lut1->GetOpacity(0);

  lut1->GetTableValue(10,rgb);
  rgb2 = lut1->GetTableValue(10);

  unsigned char output[4*1024];

  int bitA = 1;
  lut1->MapScalarsThroughTable2((void *) &bitA, output, VTK_BIT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) &bitA, output, VTK_CHAR, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) &bitA, output, VTK_CHAR, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) &bitA, output, VTK_CHAR, 2, 1, VTK_LUMINANCE);


  char charA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) charA, output, VTK_CHAR, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) charA, output, VTK_CHAR, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) charA, output, VTK_CHAR, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) charA, output, VTK_CHAR, 2, 1, VTK_LUMINANCE);

  unsigned char ucharA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) ucharA, output, VTK_UNSIGNED_CHAR, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) ucharA, output, VTK_UNSIGNED_CHAR, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) ucharA, output, VTK_UNSIGNED_CHAR, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) ucharA, output, VTK_UNSIGNED_CHAR, 2, 1, VTK_LUMINANCE);

  int intA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) intA, output, VTK_INT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) intA, output, VTK_INT, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) intA, output, VTK_INT, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) intA, output, VTK_INT, 2, 1, VTK_LUMINANCE);

  unsigned int uintA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) uintA, output, VTK_UNSIGNED_INT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) uintA, output, VTK_UNSIGNED_INT, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) uintA, output, VTK_UNSIGNED_INT, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) uintA, output, VTK_UNSIGNED_INT, 2, 1, VTK_LUMINANCE);

  long longA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) longA, output, VTK_LONG, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) longA, output, VTK_LONG, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) longA, output, VTK_LONG, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) longA, output, VTK_LONG, 2, 1, VTK_LUMINANCE);

  unsigned long ulongA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) ulongA, output, VTK_UNSIGNED_LONG, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) ulongA, output, VTK_UNSIGNED_LONG, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) ulongA, output, VTK_UNSIGNED_LONG, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) ulongA, output, VTK_UNSIGNED_LONG, 2, 1, VTK_LUMINANCE);

  short shortA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) shortA, output, VTK_SHORT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) shortA, output, VTK_SHORT, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) shortA, output, VTK_SHORT, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) shortA, output, VTK_SHORT, 2, 1, VTK_LUMINANCE);

  unsigned short ushortA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) ushortA, output, VTK_UNSIGNED_SHORT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) ushortA, output, VTK_UNSIGNED_SHORT, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) ushortA, output, VTK_UNSIGNED_SHORT, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) ushortA, output, VTK_UNSIGNED_SHORT, 2, 1, VTK_LUMINANCE);

  float floatA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) floatA, output, VTK_FLOAT, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) floatA, output, VTK_FLOAT, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) floatA, output, VTK_FLOAT, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) floatA, output, VTK_FLOAT, 2, 1, VTK_LUMINANCE);

  double doubleA[2] = {1, 10};
  lut1->MapScalarsThroughTable2((void *) doubleA, output, VTK_DOUBLE, 2, 1, VTK_RGBA);
  lut1->MapScalarsThroughTable2((void *) doubleA, output, VTK_DOUBLE, 2, 1, VTK_RGB);
  lut1->MapScalarsThroughTable2((void *) doubleA, output, VTK_DOUBLE, 2, 1, VTK_LUMINANCE_ALPHA);
  lut1->MapScalarsThroughTable2((void *) doubleA, output, VTK_DOUBLE, 2, 1, VTK_LUMINANCE);


}


int main(int argc, char* argv[])
{
  vtkDebugLeaks::PromptUserOff();

  vtkLookupTable *lut1 = vtkLookupTable::New();
  cout << "Test vtkLookupTable Start" << endl;
  lut1->SetAlpha(1.0);
  Test(cout, lut1);
  lut1->SetAlpha(.5);
  Test(cout, lut1);
  lut1->Delete();
  cout << "Test vtkLookupTable End" << endl;

  vtkLogLookupTable *lut2 = vtkLogLookupTable::New();
  cout << "Test vtkLogLookupTable Start" << endl;
  lut2->SetAlpha(1.0);
  Test(cout, lut2);
  lut2->SetAlpha(.5);
  Test(cout, lut2);
  lut2->Delete();
  cout << "Test vtkLogLookupTable End" << endl;

  return 0;
} 
