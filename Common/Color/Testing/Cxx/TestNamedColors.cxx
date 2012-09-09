/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNamedColors.h"
#include "vtkSmartPointer.h"
#include "vtkTestDriver.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cmath>
#include <sstream>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#define NUMBER_OF_SYNONYMS 20
#define NUMBER_OF_COLORS 283
#define PRINT_SELF_STRING_SIZE 8936

//-----------------------------------------------------------------------------
//  A test to see if empty vectors are returned when the color name
//  does not match a known one.
bool TestNoSuchColor(vtkStdString const & name)
{
  VTK_CREATE(vtkNamedColors, nc);
  if ( nc->ColorExists(name) )
    {
    vtkGenericWarningMacro(
      << "Fail: the color "
      << name << " exists when it shouldn't."
      );
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
//  A test to see if returning an array matches the individual values.
bool TestUnsignedChar(vtkStdString const & name)
{
  VTK_CREATE(vtkNamedColors, nc);
  unsigned char *v;
  v = nc->GetColorAsUnsignedChar(name);
  unsigned char cv[4];
  nc->GetColor(name,cv);
  bool sameElements = true;
  for ( size_t i = 0; i < 4; ++i )
    {
      if ( v[i] != cv[i] )
        {
        sameElements &= false;
        break;
        }
    }
  if (!sameElements)
    {
    vtkGenericWarningMacro(
      << "Fail: arrays are not the same "
      << "for color: " << name
      );
    }
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
  nc->GetColor(name, red, green, blue, alpha);
  if ( red != v[0] && blue != v[1] && green != v[2] && alpha != v[3] )
  {
    vtkGenericWarningMacro(
      << "Fail: One of red, green blue or alpha do not match the array "
      << "for color: " << name
      );
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
//  A test to see if returning an array matches the individual values.
bool TestDouble(vtkStdString const & name)
{
  VTK_CREATE(vtkNamedColors, nc);
  double *v;
  v = nc->GetColorAsDouble(name);
  double cv[4];
  nc->GetColor(name,cv);
  bool sameElements = true;
  for ( size_t i = 0; i < 4; ++i )
    {
      if ( v[i] != cv[i] )
        {
        sameElements &= false;
        break;
        }
    }
  if (!sameElements)
    {
    vtkGenericWarningMacro(
      << "Fail: arrays are not the same "
      << "for color: " << name
      );
    }
  double red;
  double green;
  double blue;
  double alpha;
  nc->GetColor(name, red, green, blue, alpha);
  if ( red != v[0] && blue != v[1] && green != v[2] && alpha != v[3] )
  {
    vtkGenericWarningMacro(
      << "Fail: One of red, green blue or alpha do not match the array "
      << "for color: " << name
      );
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
//  A test to see if returning an array matches the individual values.
// Alpha is ignored.
bool TestDoubleRGB(vtkStdString const & name)
{
  VTK_CREATE(vtkNamedColors, nc);
  double *v;
  v = nc->GetColorAsDoubleRGB(name);
  double cv[3];
  nc->GetColorRGB(name,cv);
  bool sameElements = true;
  for ( size_t i = 0; i < 3; ++i )
    {
      if ( v[i] != cv[i] )
        {
        sameElements &= false;
        break;
        }
    }
  if (!sameElements)
    {
    vtkGenericWarningMacro(
      << "Fail: arrays are not the same "
      << "for color: " << name
      );
    }
  double red;
  double green;
  double blue;
  nc->GetColorRGB(name, red, green, blue);
  if ( red != v[0] && blue != v[1] && green != v[2] )
  {
    vtkGenericWarningMacro(
      << "Fail: One of red, green or blue do not match the array "
      << "for color: " << name
      );
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
//  A test to see if the unsigned char conversion to double
//  matches the double vector.
bool TestUCharToDouble(vtkStdString const & name)
{
  VTK_CREATE(vtkNamedColors, nc);
  unsigned char *vu = nc->GetColorAsUnsignedChar(name);
  double *vd = nc->GetColorAsDouble(name);
  double vdu[4];
  for ( size_t i = 0; i < 4; ++i )
    {
    vdu[i] = static_cast<double>(vu[i]) / 255.0;
    }
  bool sameElements = true;
  for ( size_t i = 0; i < 4; ++i )
    {
      if ( vd[i] != vdu[i] )
        {
        sameElements &= false;
        break;
        }
    }
  if (!sameElements)
    {
    vtkGenericWarningMacro(
      << "Fail: arrays are not the same "
      << "for color: " << name
      );
    }
  return sameElements;
}

//-----------------------------------------------------------------------------
//  A test to see if adding a color works.
bool TestAddingAColor(vtkStdString name, const double dcolor[4],
                      const unsigned char ucolor[4])
{
  VTK_CREATE(vtkNamedColors, nc);
  int num1 = nc->GetNumberOfColors();

  nc->SetColor(name,dcolor);
  unsigned char *vu = nc->GetColorAsUnsignedChar(name);
  bool sameElements = true;
  for ( size_t i = 0; i < 4; ++i )
    {
      if ( vu[i] != ucolor[i] )
        {
        sameElements &= false;
        break;
        }
    }
  if (!sameElements)
    {
    vtkGenericWarningMacro(
      << "Fail: Set as double get as unsigned char, colors do not match "
      << "for color: " << name
      );
    return false;
    }

  nc->SetColor(name,ucolor);
  double *vd = nc->GetColorAsDouble(name);
  const double eps1 = 0.004; // 1/255 = 0.0039
  sameElements = true;
  for ( size_t i = 0; i < 4; ++i )
    {
    if ( std::abs(vd[i] - dcolor[i]) > eps1 )
      {
      sameElements &= false;
      }
    }
  if ( !sameElements )
    {
    vtkGenericWarningMacro(
      << "Fail: Set as unsigned char get as double, colors do not match "
      << "for color: " << name
      );
    return false;
    }

  // Set/Get as unsigned char.
  unsigned char uc[4];
  for( size_t i = 0; i < 4; ++i )
    {
    uc[i] = ucolor[i];
    }
  unsigned char ur, ug, ub, ua;
  ur = ucolor[0];
  ug = ucolor[1];
  ub = ucolor[2];
  ua = ucolor[3];
  nc->SetColor(name,uc);
  vu = nc->GetColorAsUnsignedChar(name);
  for ( size_t i = 0; i < 4; ++i )
    {
    if ( vu[i] != uc[i] )
      {
        vtkGenericWarningMacro(
        << "Fail: Set as unsigned char array get as unsigned vector, "
        << "colors do not match "
        << "for color: " << name
        );
        return false;
      }
    }

  nc->SetColor(name,ur,ug,ub,ua);
  vu = nc->GetColorAsUnsignedChar(name);
  for ( size_t i = 0; i < 4; ++i )
    {
    if ( vu[i] != uc[i] )
      {
      vtkGenericWarningMacro(
        << "Fail: Set as unsigned char values get as unsigned vector, "
        << "colors do not match "
        << "for color: " << name
        );
      return false;
      }
    }

  // Set/Get as double.
  double d[4];
  for( size_t i = 0; i < 4; ++i )
    {
    d[i] = dcolor[i];
    }
  double dr, dg, db, da;
  dr = dcolor[0];
  dg = dcolor[1];
  db = dcolor[2];
  da = dcolor[3];
  nc->SetColor(name,d);
  vd = nc->GetColorAsDouble(name);
  sameElements = true;
  const double eps2 = 1.0e-9;
  for ( size_t i = 0; i < 4; ++i )
    {
    if ( std::abs(vd[i] - d[i]) > eps2 )
      {
      sameElements &= false;
      break;
      }
    }
  if(!sameElements)
  {
  vtkGenericWarningMacro(
    << "Fail: Set as double array get as double vector, colors do not match "
    << "for color: " << name
    );
  return false;
  }
  nc->SetColor(name,dr,dg,db,da);
  vd = nc->GetColorAsDouble(name);
  sameElements = true;
  for ( size_t i = 0; i < 4; ++i )
    {
    if ( std::abs(vd[i] - d[i]) > eps2 )
      {
      sameElements &= false;
      break;
      }
    }
  if(!sameElements)
  {
  vtkGenericWarningMacro(
    << "Fail: Set as double values get as double vector, colors do not match "
    << "for color: " << name
    );
  return false;
  }

  nc->RemoveColor(name);
  int sz = nc->GetNumberOfColors();
  if (sz != NUMBER_OF_COLORS)
    {
    vtkGenericWarningMacro(
      << "Fail: Incorrect number of colors found, expected "
      << nc->GetNumberOfColors() << ", got "
      << NUMBER_OF_COLORS << " instead after inserting/deleting the color "
      << name
      );
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
//  A test to see if searching for duplicates works.
bool SearchForDuplicateColors()
{
  VTK_CREATE(vtkNamedColors, nc);
  std::vector<std::vector<vtkStdString> > synonyms = nc->GetSynonyms();
  return synonyms.size() == NUMBER_OF_SYNONYMS;
}

int TestNamedColors(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  VTK_CREATE(vtkNamedColors, nc);
  bool testResult = TestNoSuchColor("AliceGreen"); // This color does not exist.
  if ( !testResult )
    {
    vtkGenericWarningMacro(
      << "Fail: TestNoSuchColor()"
      );
    }
  int counter = -1;
  const int colorsToSkip = 20;
  std::vector<vtkStdString> cn = nc->GetColorNames();
  for ( std::vector<vtkStdString>::const_iterator
        p = cn.begin(); p != cn.end(); ++p )
    {
    counter++;
    // Skip some colors to make testing faster.
    if ( counter % colorsToSkip != 0 )
    {
      continue;
    }
    if ( !TestUnsignedChar(*p) )
      {
      vtkGenericWarningMacro(
        << "Fail: TestUnsignedChar(), with color "
        << *p
        );
      testResult &= false;
      }
    if ( !TestDouble(*p) )
      {
      vtkGenericWarningMacro(
        << "Fail: TestDouble(), with color "
        << *p
        );
      testResult &= false;
      }
    if ( !TestDoubleRGB(*p) )
      {
      vtkGenericWarningMacro(
        << "Fail: TestDoubleRGB(), with color "
        << *p
        );
      testResult &= false;
      }
    if ( !TestUCharToDouble(*p) )
      {
      vtkGenericWarningMacro(
        << "Fail: TestUCharToDouble(), with color "
        << *p
        );
      testResult &= false;
      }
    }
  unsigned char ucolor[4];
  double dcolor[4];
  vtkStdString name("Weird Color"); // Choose a name with spaces.
  unsigned char ur = 51;
  double r = 0.2;
  for ( size_t i = 0; i < 3; ++i )
    {
    ucolor[i] = static_cast<unsigned char>(i+1) * ur;
    dcolor[i] = (i+1) * r;
    }
  ucolor[3] = 0;
  dcolor[3] = 0;
  if ( !TestAddingAColor(name,dcolor,ucolor) )
    {
    vtkGenericWarningMacro(
      << "Fail: TestAddingAColor(), with color "
      << name
      );
    testResult &= false;
    }
  if ( !SearchForDuplicateColors() )
    {
    vtkGenericWarningMacro(
      << "Fail: SearchForSynonyms() - incorrect number of synonyms found, "
      << "expected "
      << NUMBER_OF_SYNONYMS << " instead."
      );
    testResult &= false;
    }
  if ( nc->GetNumberOfColors() != NUMBER_OF_COLORS )
    {
    vtkGenericWarningMacro(
      << "Fail: SearchForDuplicateColors() - incorrect number of colors"
      << "found " <<
      nc->GetNumberOfColors() << ", expected "
      << NUMBER_OF_COLORS << " instead."
      );
    testResult &= false;
    }
  std::ostringstream os;
  nc->PrintSelf(os,vtkIndent(2));
  //std::cout << os.str() << std::endl;
  if ( os.str().size() != PRINT_SELF_STRING_SIZE )
  {
    vtkGenericWarningMacro(
      << "Fail: PrintSelf() - a string of size " <<
      PRINT_SELF_STRING_SIZE << " was expected, got "
      << os.str().size() << " instead."
      );
    testResult &= false;
  }
  if ( !testResult )
    {
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
