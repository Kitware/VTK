/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUnicodeStringAPI.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkUnicodeString.h>

#include <vtksys/stl/iterator>
#include <vtksys/ios/iostream>
#include <vtksys/ios/sstream>
#include <vtksys/stl/stdexcept>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    vtksys_ios::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw vtkstd::runtime_error(buffer.str()); \
    } \
}

// Sample strings - nothing risque, I hope ...
static const vtkstd::string sample_utf8_ascii = "abcde123";
static const vtkstd::string sample_utf8_greek = "\xce\xb1\xce\xb2\xce\xb3"; // Greek lower-case alpha, beta, gamma.
static const vtkstd::string sample_utf8_thai = "\xe0\xb8\x81\xe0\xb8\x82\xe0\xb8\x83"; // Thai ko kai, kho khai, kho khuat.
static const vtkstd::string sample_utf8_linear_b = "\xf0\x90\x80\x80\xf0\x90\x80\x81\xf0\x90\x80\x82\xf0\x90\x80\x83\xf0\x90\x80\x84"; // Linear-B syllables a, e, i, o, u.
static const vtkstd::string sample_utf8_mixed = "a\xce\xb1\xe0\xb8\x81\xf0\x90\x80\x80"; // a, alpha, ko kai, syllable-a.
static const vtkTypeUInt16 sample_utf16[] = 
{
  0x0041,       // 'a'
  0x0020,       // ' '
  0xD800,       // high-half zone part
  0xDC00,       // low-half zone part
  0xD800,       // etc.
  0xDC01,
  0x0000
};


int TestUnicodeStringAPI(int, char*[])
{
  try
    {
    vtkUnicodeString a;
    test_expression(a.empty());
    test_expression(a.character_count() == 0);

    a = vtkUnicodeString::from_utf8(sample_utf8_ascii);
    test_expression(!a.empty());
    test_expression(a.character_count() == 8);
    test_expression(a.utf8_str() == sample_utf8_ascii);
    test_expression(a.at(1) == 0x00000062);
    test_expression(a[1] == 0x00000062);
    
    a.clear();
    test_expression(a.empty());
   
    a = vtkUnicodeString::from_utf8(sample_utf8_greek);
    test_expression(a.character_count() == 3);
    test_expression(a.utf8_str() == sample_utf8_greek);
    test_expression(a.at(2) == 0x000003b3);
    test_expression(a[2] == 0x000003b3);
   
    a = vtkUnicodeString::from_utf8(sample_utf8_thai);
    test_expression(a.character_count() == 3);
    test_expression(a.utf8_str() == sample_utf8_thai);
    test_expression(a.at(1) == 0x00000e02);
    test_expression(a[1] == 0x00000e02);

    a = vtkUnicodeString::from_utf8(sample_utf8_linear_b);
    test_expression(a.character_count() == 5);
    test_expression(a.utf8_str() == sample_utf8_linear_b);
    test_expression(a.at(4) == 0x00010004);
    test_expression(a[4] == 0x00010004);
   
    a = vtkUnicodeString::from_utf8(sample_utf8_mixed);
    test_expression(a.character_count() == 4);
    test_expression(a.utf8_str() == sample_utf8_mixed);
    test_expression(a.at(2) == 0x00000e01);
    test_expression(a[2] == 0x00000e01);
  
    vtkUnicodeString b = vtkUnicodeString::from_utf8(sample_utf8_ascii);
    test_expression(b.utf8_str() == sample_utf8_ascii);
    test_expression(a.utf8_str() == sample_utf8_mixed);
    a.swap(b);
    test_expression(a.utf8_str() == sample_utf8_ascii);
    test_expression(b.utf8_str() == sample_utf8_mixed);

    a = vtkUnicodeString::from_utf16(sample_utf16);
    test_expression(a.character_count() == 4);
    test_expression(a[0] == 0x00000041);
    test_expression(a[1] == 0x00000020);
    test_expression(a[2] == 0x00010000);
    test_expression(a[3] == 0x00010001);

    a = vtkUnicodeString::from_utf8("Hello, World!");
    test_expression(a.substr(7) == vtkUnicodeString::from_utf8("World!"));
    test_expression(a.substr(1, 4) == vtkUnicodeString::from_utf8("ello"));
  
    return 0;
    }
  catch(vtkstd::exception& e)
    {
    cerr << e.what() << endl;
    return 1;
    }
}

