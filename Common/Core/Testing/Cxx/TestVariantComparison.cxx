/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVariantComparison.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVariant.h"
#include "vtkObject.h"

#include <vtksys/stl/map>
#include <cstdio>

int
TestVariantComparison(int, char *[])
{
  signed char positiveChar = 100;
  signed char negativeChar = -100;
  short positiveShort = 10000;
  short negativeShort = -10000;
  int positiveInt = 100000;
  int negativeInt = -100000;
  long positiveLong = 1000000;
  long negativeLong = -1000000;

  int shiftAmount64 = 8 * sizeof(vtkTypeInt64) - 2;
  int shiftAmountInt = 8 * sizeof(int) - 2;
  int shiftAmountLong = 8 * sizeof(long) - 2;

  vtkTypeInt64 positive64 = static_cast<vtkTypeInt64>(1) << shiftAmount64;
  vtkTypeInt64 negative64 = -positive64;

  // There is nothing inherently magical about these values.  I just
  // happen to like them and they're outside the range of signed
  // integers.
  unsigned char unsignedChar = 192;
  unsigned short unsignedShort = 49152;
  unsigned int unsignedInt = (static_cast<unsigned int>(1)<<shiftAmountInt) * 3;
  unsigned long unsignedLong = (static_cast<unsigned long>(1)<<shiftAmountLong) * 3;
  vtkTypeUInt64 unsigned64 = 3 * (static_cast<vtkTypeUInt64>(1) << shiftAmount64);

  vtkStdString numberString("100000");
  vtkStdString alphaString("ABCDEFG");

  float positiveFloat = 12345.678;
  float negativeFloat = -12345.678;
  double positiveDouble = 123456789.012345;
  double negativeDouble = -123456789.012345;

  vtkObject *fooObject = vtkObject::New();

  vtkVariant invalidVariant;

  // Now we need variants for all of those
  vtkVariant positiveCharVariant(positiveChar);
  vtkVariant unsignedCharVariant(unsignedChar);
  vtkVariant negativeCharVariant(negativeChar);

  vtkVariant positiveShortVariant(positiveShort);
  vtkVariant unsignedShortVariant(unsignedShort);
  vtkVariant negativeShortVariant(negativeShort);

  vtkVariant positiveIntVariant(positiveInt);
  vtkVariant unsignedIntVariant(unsignedInt);
  vtkVariant negativeIntVariant(negativeInt);

  vtkVariant positiveLongVariant(positiveLong);
  vtkVariant unsignedLongVariant(unsignedLong);
  vtkVariant negativeLongVariant(negativeLong);

  vtkVariant positive64Variant(positive64);
  vtkVariant unsigned64Variant(unsigned64);
  vtkVariant negative64Variant(negative64);

  vtkVariant positiveFloatVariant(positiveFloat);
  vtkVariant negativeFloatVariant(negativeFloat);
  vtkVariant positiveDoubleVariant(positiveDouble);
  vtkVariant negativeDoubleVariant(negativeDouble);

  vtkVariant numberStringVariant(numberString);
  vtkVariant alphaStringVariant(alphaString);

  vtkVariant fooObjectVariant(fooObject);

  int errorCount = 0;
  int overallErrorCount = 0;

#define CHECK_EXPRESSION_FALSE(expr) { if ((expr)) { ++errorCount; cerr << "TEST FAILED: " << #expr << " should have been false\n\n"; } }

#define CHECK_EXPRESSION_TRUE(expr) { if (!(expr)) { ++errorCount; cerr << "TEST FAILED: " << #expr << " should have been true\n\n"; } }

  cerr << "Testing same-type comparisons... ";
  CHECK_EXPRESSION_FALSE(positiveCharVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(unsignedCharVariant < positiveCharVariant);
  CHECK_EXPRESSION_FALSE(unsignedCharVariant < negativeCharVariant);

  CHECK_EXPRESSION_FALSE(positiveShortVariant < negativeShortVariant);
  CHECK_EXPRESSION_FALSE(unsignedShortVariant < positiveShortVariant);
  CHECK_EXPRESSION_FALSE(unsignedShortVariant < negativeShortVariant);

  CHECK_EXPRESSION_FALSE(positiveIntVariant < negativeIntVariant);
  CHECK_EXPRESSION_FALSE(unsignedIntVariant < positiveIntVariant);
  CHECK_EXPRESSION_FALSE(unsignedIntVariant < negativeIntVariant);

  CHECK_EXPRESSION_FALSE(positiveLongVariant < negativeLongVariant);
  CHECK_EXPRESSION_FALSE(unsignedLongVariant < positiveLongVariant);
  CHECK_EXPRESSION_FALSE(unsignedLongVariant < negativeLongVariant);

  CHECK_EXPRESSION_FALSE(positive64Variant < negative64Variant);
  CHECK_EXPRESSION_FALSE(unsigned64Variant < positive64Variant);
  CHECK_EXPRESSION_FALSE(unsigned64Variant < negative64Variant);

  CHECK_EXPRESSION_FALSE(positiveFloat < negativeFloat);
  CHECK_EXPRESSION_FALSE(positiveDouble < negativeDouble);

  CHECK_EXPRESSION_FALSE(alphaString < numberString);

  if (errorCount == 0)
    {
    cerr << "Test succeeded.\n";
    }
  else
    {
    cerr << errorCount << " error(s) found!\n";
    }
  overallErrorCount += errorCount;
  errorCount = 0;

  cerr << "Testing cross-type comparisons... ";

  CHECK_EXPRESSION_FALSE(positiveShortVariant < positiveCharVariant);
  CHECK_EXPRESSION_FALSE(positiveIntVariant < positiveCharVariant);
  CHECK_EXPRESSION_FALSE(positiveLongVariant < positiveCharVariant);
  CHECK_EXPRESSION_FALSE(positive64Variant < positiveCharVariant);

  CHECK_EXPRESSION_FALSE(positiveShortVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(positiveIntVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(positiveLongVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(positive64Variant < negativeCharVariant);

  CHECK_EXPRESSION_FALSE(positiveShortVariant < unsignedCharVariant);
  CHECK_EXPRESSION_FALSE(positiveIntVariant < unsignedCharVariant);
  CHECK_EXPRESSION_FALSE(positiveLongVariant < unsignedCharVariant);
  CHECK_EXPRESSION_FALSE(positive64Variant < unsignedCharVariant);

  CHECK_EXPRESSION_FALSE(negativeCharVariant < negativeShortVariant);
  CHECK_EXPRESSION_FALSE(negativeCharVariant < negativeIntVariant);
  CHECK_EXPRESSION_FALSE(negativeCharVariant < negativeLongVariant);
  CHECK_EXPRESSION_FALSE(negativeCharVariant < negative64Variant);

  CHECK_EXPRESSION_FALSE(unsignedShortVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(unsignedIntVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(unsignedLongVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(unsigned64Variant < negativeCharVariant);

  CHECK_EXPRESSION_FALSE(positiveFloatVariant < positiveCharVariant);
  CHECK_EXPRESSION_FALSE(positiveFloatVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(positiveFloatVariant < unsignedCharVariant);

  CHECK_EXPRESSION_FALSE(positiveDoubleVariant < positiveCharVariant);
  CHECK_EXPRESSION_FALSE(positiveDoubleVariant < negativeCharVariant);
  CHECK_EXPRESSION_FALSE(positiveDoubleVariant < unsignedCharVariant);

  CHECK_EXPRESSION_FALSE(alphaStringVariant < positiveIntVariant);
  CHECK_EXPRESSION_FALSE(numberStringVariant != positiveIntVariant);
  CHECK_EXPRESSION_FALSE(positiveDoubleVariant < fooObjectVariant);
  CHECK_EXPRESSION_FALSE(positiveFloatVariant < invalidVariant);

  if (errorCount == 0)
    {
    cerr << "Test succeeded.\n";
    }
  else
    {
    cerr << errorCount << " error(s) found!\n";
    }
  overallErrorCount += errorCount;
  errorCount = 0;

  cerr << "Testing cross-type equality...";

  char c = 100;
  short s = 100;
  int i = 100;
  long l = 100;
  vtkTypeInt64 i64 = 100;
  float f = 100;
  double d = 100;
  vtkStdString str("100");

  CHECK_EXPRESSION_TRUE(vtkVariant(c) == vtkVariant(s));
  CHECK_EXPRESSION_TRUE(vtkVariant(c) == vtkVariant(i));
  CHECK_EXPRESSION_TRUE(vtkVariant(c) == vtkVariant(l));
  CHECK_EXPRESSION_TRUE(vtkVariant(c) == vtkVariant(i64));
  CHECK_EXPRESSION_TRUE(vtkVariant(c) == vtkVariant(f));
  CHECK_EXPRESSION_TRUE(vtkVariant(c) == vtkVariant(d));

  CHECK_EXPRESSION_TRUE(vtkVariant(s) == vtkVariant(i));
  CHECK_EXPRESSION_TRUE(vtkVariant(s) == vtkVariant(l));
  CHECK_EXPRESSION_TRUE(vtkVariant(s) == vtkVariant(i64));
  CHECK_EXPRESSION_TRUE(vtkVariant(s) == vtkVariant(f));
  CHECK_EXPRESSION_TRUE(vtkVariant(s) == vtkVariant(d));
  CHECK_EXPRESSION_TRUE(vtkVariant(s) == vtkVariant(str));

  CHECK_EXPRESSION_TRUE(vtkVariant(i) == vtkVariant(l));
  CHECK_EXPRESSION_TRUE(vtkVariant(i) == vtkVariant(i64));
  CHECK_EXPRESSION_TRUE(vtkVariant(i) == vtkVariant(f));
  CHECK_EXPRESSION_TRUE(vtkVariant(i) == vtkVariant(d));
  CHECK_EXPRESSION_TRUE(vtkVariant(i) == vtkVariant(str));

  CHECK_EXPRESSION_TRUE(vtkVariant(l) == vtkVariant(i64));
  CHECK_EXPRESSION_TRUE(vtkVariant(l) == vtkVariant(f));
  CHECK_EXPRESSION_TRUE(vtkVariant(l) == vtkVariant(d));
  CHECK_EXPRESSION_TRUE(vtkVariant(l) == vtkVariant(str));

  CHECK_EXPRESSION_TRUE(vtkVariant(i64) == vtkVariant(f));
  CHECK_EXPRESSION_TRUE(vtkVariant(i64) == vtkVariant(d));
  CHECK_EXPRESSION_TRUE(vtkVariant(i64) == vtkVariant(str));

  CHECK_EXPRESSION_TRUE(vtkVariant(f) == vtkVariant(d));
  CHECK_EXPRESSION_TRUE(vtkVariant(f) == vtkVariant(str));

  CHECK_EXPRESSION_TRUE(vtkVariant(d) == vtkVariant(str));

  if (errorCount == 0)
    {
    cerr << " Test succeeded.\n";
    }
  else
    {
    cerr << errorCount << " error(s) found!\n";
    }
  overallErrorCount += errorCount;
  errorCount = 0;

  cerr << "Testing vtkVariant as STL map key... ";

  vtksys_stl::map<vtkVariant, vtkStdString> TestMap;

  TestMap[vtkVariant(s)] = "short";
  TestMap[vtkVariant(i)] = "int";
  TestMap[vtkVariant(l)] = "long";
  TestMap[vtkVariant(i64)] = "int64";
  TestMap[vtkVariant(f)] = "float";
  TestMap[vtkVariant(d)] = "double";
  TestMap[vtkVariant(str)] = "string";

  CHECK_EXPRESSION_TRUE(TestMap.find(vtkVariant(100)) != TestMap.end());
  CHECK_EXPRESSION_TRUE(TestMap[vtkVariant(100)] == "string");
  CHECK_EXPRESSION_TRUE(TestMap.size() == 1);

  if (errorCount == 0)
    {
    cerr << " Test succeeded.\n";
    }
  else
    {
    cerr << errorCount << " error(s) found!\n";
    }
  overallErrorCount += errorCount;
  errorCount = 0;

  cerr << "Testing vtkVariant as STL map key with strict weak ordering (fast comparator)...";

  // This one should treat variants containing different types as
  // unequal.
  vtksys_stl::map<vtkVariant, vtkStdString, vtkVariantStrictWeakOrder> TestMap2;
  TestMap2[vtkVariant()] = "invalid";
  TestMap2[vtkVariant(s)] = "short";
  TestMap2[vtkVariant(i)] = "int";
  TestMap2[vtkVariant(l)] = "long";
  TestMap2[vtkVariant(i64)] = "int64";
  TestMap2[vtkVariant(f)] = "float";
  TestMap2[vtkVariant(d)] = "double";
  TestMap2[vtkVariant(str)] = "string";

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant()) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant()] == "invalid");

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant(s)) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant(s)] == "short");

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant(i)) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant(i)] == "int");

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant(l)) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant(l)] == "long");

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant(i64)) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant(i64)] == "int64");

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant(f)) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant(f)] == "float");

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant(d)) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant(d)] == "double");

  CHECK_EXPRESSION_TRUE(TestMap2.find(vtkVariant(str)) != TestMap2.end());
  CHECK_EXPRESSION_TRUE(TestMap2[vtkVariant("100")] == "string");

  CHECK_EXPRESSION_TRUE(TestMap2.size() == 8);

  if (errorCount == 0)
    {
    cerr << " Test succeeded.\n";
    }
  else
    {
    cerr << errorCount << " error(s) found!\n";
    }
  overallErrorCount += errorCount;

  if (overallErrorCount == 0)
    {
    cerr << "All tests succeeded.\n";
    }
  else
    {
    cerr << "Some tests failed!  Overall error count: " << overallErrorCount
         << "\n";
    cerr << "Debug information:\n";
    cerr << "CHAR(" << sizeof(char) << "): "
         << "positive " << positiveChar << ", "
         << "negative " << negativeChar << ", "
         << "unsigned " << unsignedChar << "\n";
    cerr << "SHORT(" << sizeof(short) << "): "
         << "positive " << positiveShort << ", "
         << "negative " << negativeShort << ", "
         << "unsigned " << unsignedShort << "\n";
    cerr << "INT(" << sizeof(int) << "): "
         << "positive " << positiveInt << ", "
         << "negative " << negativeInt << ", "
         << "unsigned " << unsignedInt << "\n";
    cerr << "LONG(" << sizeof(long) << "): "
         << "positive " << positiveLong << ", "
         << "negative " << negativeLong << ", "
         << "unsigned " << unsignedLong << "\n";
    cerr << "INT64(" << sizeof(vtkTypeInt64) << "): "
         << "positive " << positive64 << ", "
         << "negative " << negative64 << ", "
         << "unsigned " << unsigned64 << "\n";
    }

  fooObject->Delete();
  return (overallErrorCount > 0);
}
