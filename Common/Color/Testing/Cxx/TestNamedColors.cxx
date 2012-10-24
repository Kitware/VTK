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
#include <string>
#include <vector>

const int NUMBER_OF_SYNONYMS = 81;
const int NUMBER_OF_COLORS = 283;
const int PRINT_SELF_STRING_SIZE = 9242;
// For comparing unsigned char converted to double precision.
const double EPS1 = 0.004; // 1/255 = 0.0039
// For comparing two double precision numbers.
const double EPS2 = 1.0e-9;

// Forward declaration of the test functions.
//  A test to see if black is returned if the color name is empty.
bool TestEmptyColorName();

// A test to see if empty vectors are returned when the color name
// does not match a known one.
bool TestNoSuchColor(vtkStdString const & name);

// A test to see if returning an array matches the individual values.
bool TestUnsignedChar(vtkStdString const & name);

// A test to see if returning an array matches the individual values.
bool TestDouble(vtkStdString const & name);

// A test to see if returning an array matches the individual values.
// Alpha is ignored.
bool TestDoubleRGB(vtkStdString const & name);

// A test to see if the unsigned char conversion to double
// matches the double vector.
bool TestUCharToDouble(vtkStdString const & name);

//  A test to see if adding a color works.
bool TestAddingAColor(vtkStdString name, const double dcolor[4],
                      const unsigned char ucolor[4]);

// Parse the color names returning a std::vector<std::string>
// colorNames is a string formatted with each name separated
// with a linefeed.
std::vector<vtkStdString> ParseColorNames(const vtkStdString & colorNames);

// Parse the synonyms returning a std::vector<std::vector<std::string> >
// synonyms is a string of synonyms separated by a double linefeed where
// each synonym is two or more color names separated by a linefeed
std::vector<std::vector<vtkStdString> > ParseSynonyms(
  const vtkStdString & synonyms);

//  A test to see if searching for synonyms works.
bool TestSearchForSynonyms();

//-----------------------------------------------------------------------------
bool TestEmptyColorName()
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  vtkStdString name;
  // Reference color
  unsigned char rr, rg, rb;
  rr = rb = rg = 0;
  unsigned char ra = 255;
  vtkColor4ub v = nc->GetColor4ub(name);
  if ( v[0] != rr || v[1] != rg || v[2] != rb || v[3] != ra )
    {
    vtkGenericWarningMacro(
      << "Fail: an empty color name "
      << "returned an unsigned char color other than black."
      );
    return false;
  }
  vtkColor3ub v3 = nc->GetColor3ub(name);
  if ( v3[0] != rr || v3[1] != rg || v3[2] != rb )
    {
    vtkGenericWarningMacro(
      << "Fail: an empty color name "
      << "returned an unsigned char color other than black."
      );
    return false;
  }
  unsigned char ur, ug, ub;
  ur = ug = ub = 0;
  unsigned char ua = 0;
  nc->GetColor(name,ur,ug,ub,ua);
  if ( ur != rr || ug != rg || ub != rb || ua != ra )
    {
    vtkGenericWarningMacro(
      << "Fail: an empty color name "
      << "returned an unsigned char color other than black."
      );
    return false;
  }

  // Reference color
  double rrd, rgd, rbd;
  rrd = rgd = rbd = 0;
  double rad = 1;
  vtkColor4d vd = nc->GetColor4d(name);
  if ( vd[0] != rrd || vd[1] != rgd || vd[2] != rbd || vd[3] != rad )
    {
    vtkGenericWarningMacro(
      << "Fail: an empty color name "
      << "returned a double color other than black."
      );
    return false;
  }
  vtkColor3d vd3 = nc->GetColor3d(name);
  if ( vd3[0] != rrd || vd3[1] != rgd || vd3[2] != rbd )
    {
    vtkGenericWarningMacro(
      << "Fail: an empty color name "
      << "returned a double color other than black."
      );
    return false;
  }
  double dr, dg, db;
  dr = dg = db = 1;
  double da = 0;
  nc->GetColor(name,dr,dg,db,da);
  if ( dr != rrd || dg != rgd || db != rbd || da != rad )
    {
    vtkGenericWarningMacro(
      << "Fail: an empty color name "
      << "returned an double color other than black."
      );
    return false;
  }
  nc->GetColor(name,dr,dg,db);
  if ( ur != rrd || dg != rgd || db != rbd )
    {
    vtkGenericWarningMacro(
      << "Fail: an empty color name "
      << "returned an double color other than black."
      );
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool TestNoSuchColor(vtkStdString const & name)
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
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
bool TestUnsignedChar(vtkStdString const & name)
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  vtkColor4ub v = nc->GetColor4ub(name);
  unsigned char cv[4];
  nc->GetColor(name,cv);
  bool sameElements = true;
  for ( int i = 0; i < 4; ++i )
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
  sameElements = true;
  nc->GetColor(name,v);
  for ( int i = 0; i < 4; ++i )
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
  sameElements = true;
  vtkColor3ub vv;
  nc->GetColor(name,vv);
  for ( int i = 0; i < 3; ++i )
    {
      if ( vv[i] != cv[i] )
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
    return false;
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
bool TestDouble(vtkStdString const & name)
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  vtkColor4d v = nc->GetColor4d(name);
  double cv[4];
  nc->GetColor(name,cv);
  bool sameElements = true;
  for ( int i = 0; i < 4; ++i )
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
  sameElements = true;
  nc->GetColor(name,v);
  for ( int i = 0; i < 4; ++i )
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
  sameElements = true;
  vtkColor3d vv;
  nc->GetColor(name,vv);
  for ( int i = 0; i < 3; ++i )
    {
      if ( vv[i] != cv[i] )
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
    return false;
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
bool TestDoubleRGB(vtkStdString const & name)
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  vtkColor3d v = nc->GetColor3d(name);
  double cv[3];
  nc->GetColorRGB(name,cv);
  bool sameElements = true;
  for ( int i = 0; i < 3; ++i )
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
  nc->GetColor(name, red, green, blue);
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
bool TestUCharToDouble(vtkStdString const & name)
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  vtkColor4ub vu = nc->GetColor4ub(name);
  vtkColor4d  vd = nc->GetColor4d(name);
  double vdu[4];
  for ( int i = 0; i < 4; ++i )
    {
    vdu[i] = static_cast<double>(vu[i]) / 255.0;
    }
  bool sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
      if ( std::abs(vd[i] -  vdu[i]) > EPS2 )
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
bool TestAddingAColor(vtkStdString name, const double dcolor[4],
                      const unsigned char ucolor[4])
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  int sz = nc->GetNumberOfColors();

  vtkColor4ub ub4(ucolor);
  vtkColor4d d4(dcolor);
  vtkColor3ub ub3;
  vtkColor3d d3;
  for(int i = 0; i < 3; ++i)
    {
      ub3[i] = ub4[i];
      d3[i] = d4[i];
    }

  // Test for adding empty names.
  nc->SetColor("",dcolor);
  nc->SetColor("",dcolor[0],dcolor[1],dcolor[2],dcolor[3]);
  if(sz != nc->GetNumberOfColors())
    {
    vtkGenericWarningMacro(
      << "Fail: Setting a double color with an empty name."
      );
    nc->ResetColors();
    return false;
    }
  nc->SetColor("",ucolor);
  nc->SetColor("",ucolor[0],ucolor[1],ucolor[2],ucolor[3]);
  if(sz != nc->GetNumberOfColors())
    {
    vtkGenericWarningMacro(
      << "Fail: Setting an unsigned char color with an empty name."
      );
    nc->ResetColors();
    return false;
    }

  nc->SetColor(name,dcolor);
  vtkColor4ub vu = nc->GetColor4ub(name);
  bool sameElements = true;
  for ( int i = 0; i < 4; ++i )
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
    nc->ResetColors();
    return false;
    }

  nc->SetColor(name,ucolor);
  vtkColor4d vd = nc->GetColor4d(name);
  sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
    if ( std::abs(vd[i] - dcolor[i]) > EPS1 )
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
    nc->ResetColors();
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
  vu = nc->GetColor4ub(name);
  sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
    if ( vu[i] != uc[i] )
      {
      sameElements &= false;
      }
    }
  if ( !sameElements )
    {
    vtkGenericWarningMacro(
      << "Fail: Set as unsigned char array get as vtkColor4ub, "
      << "colors do not match "
      << "for color: " << name
      );
    nc->ResetColors();
    return false;
    }

  nc->SetColor(name,ub4);
  vu = nc->GetColor4ub(name);
  sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
    if ( vu[i] != uc[i] )
      {
      sameElements &= false;
      }
    }
  if ( !sameElements )
    {
    vtkGenericWarningMacro(
      << "Fail: Set as vtkColor4ub array get as vtkColor4ub, "
      << "colors do not match "
      << "for color: " << name
      );
    nc->ResetColors();
    return false;
    }

  nc->SetColor(name,ub3);
  vu = nc->GetColor4ub(name);
  sameElements = true;
  for ( int i = 0; i < 3; ++i )
    {
    if ( vu[i] != uc[i] )
      {
      sameElements &= false;
      }
    }
  if ( !sameElements )
    {
    vtkGenericWarningMacro(
      << "Fail: Set as vtkColor3ub array get as vtkColor4ub, "
      << "colors do not match "
      << "for color: " << name
      );
    nc->ResetColors();
    return false;
    }

  nc->SetColor(name,ur,ug,ub,ua);
  vu = nc->GetColor4ub(name);
  sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
    if ( vu[i] != uc[i] )
      {
      sameElements &= false;
      }
    }
  if ( !sameElements )
    {
    vtkGenericWarningMacro(
      << "Fail: Set as unsigned char values get as vtkColor4ub, "
      << "colors do not match "
      << "for color: " << name
      );
    nc->ResetColors();
    return false;
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
  vd = nc->GetColor4d(name);
  sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
    if ( std::abs(vd[i] - d[i]) > EPS2 )
      {
      sameElements &= false;
      break;
      }
    }
  if(!sameElements)
  {
  vtkGenericWarningMacro(
    << "Fail: Set as double array get as vtkColor4d, colors do not match "
    << "for color: " << name
    );
  nc->ResetColors();
  return false;
  }

  nc->SetColor(name,d4);
  vd = nc->GetColor4d(name);
  sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
    if ( std::abs(vd[i] - d[i]) > EPS2 )
      {
      sameElements &= false;
      break;
      }
    }
  if(!sameElements)
  {
  vtkGenericWarningMacro(
    << "Fail: Set as vtkColor4d get as vtkColor4d, colors do not match "
    << "for color: " << name
    );
  nc->ResetColors();
  return false;
  }

  nc->SetColor(name,dr,dg,db,da);
  vd = nc->GetColor4d(name);
  sameElements = true;
  for ( int i = 0; i < 4; ++i )
    {
    if ( std::abs(vd[i] - d[i]) > EPS2 )
      {
      sameElements &= false;
      break;
      }
    }
  if(!sameElements)
  {
  vtkGenericWarningMacro(
    << "Fail: Set as double values get as vtkColor4d, colors do not match "
    << "for color: " << name
    );
  nc->ResetColors();
  return false;
  }

  nc->SetColor(name,d3);
  vd = nc->GetColor4d(name);
  sameElements = true;
  for ( int i = 0; i < 3; ++i )
    {
    if ( std::abs(vd[i] - d[i]) > EPS2 )
      {
      sameElements &= false;
      break;
      }
    }
  if(!sameElements)
  {
  vtkGenericWarningMacro(
    << "Fail: Set as vtkColor3d get as vtkColor4d, colors do not match "
    << "for color: " << name
    );
  nc->ResetColors();
  return false;
  }

  nc->RemoveColor(name);
  sz = nc->GetNumberOfColors();
  if (sz != NUMBER_OF_COLORS)
    {
    vtkGenericWarningMacro(
      << "Fail: Incorrect number of colors found, expected "
      << nc->GetNumberOfColors() << ", got "
      << NUMBER_OF_COLORS << " instead after inserting/deleting the color "
      << name
      );
    nc->ResetColors();
    return false;
    }

  return true;
}

//-----------------------------------------------------------------------------
std::vector<vtkStdString> ParseColorNames(const vtkStdString & colorNames)
{
  // The delimiter for a color.
  const std::string colorDelimiter = "\n";
  std::vector<vtkStdString> cn;
  size_t start = 0;
  size_t end = colorNames.find(colorDelimiter);
  while(end != std::string::npos)
    {
    cn.push_back(colorNames.substr(start,end - start));
    start = end + 1;
    end = colorNames.find(colorDelimiter,start);
    }
  // Get the last color.
  if (!colorNames.empty())
    {
    cn.push_back(colorNames.substr(start,colorNames.size() - start));
    }
  return cn;
}

//-----------------------------------------------------------------------------
std::vector<std::vector<vtkStdString> > ParseSynonyms(
  const vtkStdString & synonyms)
{
  // The delimiter for a string of synonyms.
  const std::string synonymDelimiter = "\n\n";
  size_t start = 0;
  size_t end = synonyms.find("\n\n"); // The delimiter for a string of synonyms.
  std::vector<vtkStdString> cn;
  std::vector<std::vector<vtkStdString> > syn;
  vtkStdString str;
  while(end != std::string::npos)
    {
    str = synonyms.substr(start,end - start);
    cn = ParseColorNames(str);
    syn.push_back(cn);
    start = end + 2;
    end = synonyms.find(synonymDelimiter,start);
  }
  // Get the last set of synonyms.
  if(!synonyms.empty())
    {
    str = synonyms.substr(start,end - start);
    cn = ParseColorNames(str);
    syn.push_back(cn);
    }
  // Sanity check!
  //for(std::vector<std::vector<vtkStdString> >::const_iterator p =
  //  syn.begin(); p != syn.end(); ++p)
  //  {
  //    for(std::vector<vtkStdString>::const_iterator q =
  //      p->begin(); q != p->end(); ++q)
  //    {
  //      std::cout << *q << " ";
  //    }
  //    std::cout << std::endl;
  //  }
  return syn;
}

//-----------------------------------------------------------------------------
bool TestSearchForSynonyms()
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  std::vector<std::vector<vtkStdString> > synonyms =
    ParseSynonyms(nc->GetSynonyms());
  return static_cast<int>(synonyms.size()) == NUMBER_OF_SYNONYMS;
}

int TestNamedColors(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
  bool testResult = TestEmptyColorName();
  if ( !testResult )
    {
    vtkGenericWarningMacro(
      << "Fail: TestNoSuchColor()"
      );
    }

  testResult &= TestNoSuchColor("no_such_color"); // This color does not exist.
  if ( !testResult )
    {
    vtkGenericWarningMacro(
      << "Fail: TestNoSuchColor()"
      );
    }

  int counter = -1;
  const int colorsToSkip = 20;
  std::vector<vtkStdString> cn = ParseColorNames(nc->GetColorNames());
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

  if ( !TestSearchForSynonyms() )
    {
    vtkGenericWarningMacro(
      << "Fail: TestSearchForSynonyms() - incorrect number of synonyms found, "
      << "expected "
      << NUMBER_OF_SYNONYMS << " instead."
      );
    testResult &= false;
    }

  if ( static_cast<int>(cn.size()) != NUMBER_OF_COLORS )
    {
    vtkGenericWarningMacro(
      << "Fail: Incorrect number of colors"
      << "found " <<
      cn.size() << ", expected "
      << NUMBER_OF_COLORS << " instead."
      );
    testResult &= false;
    }

  nc->ResetColors();
  if ( nc->GetNumberOfColors() != NUMBER_OF_COLORS )
    {
    vtkGenericWarningMacro(
      << "Fail: GetNumberOfColors(), incorrect number of colors"
      << "found " <<
      cn.size() << ", expected "
      << NUMBER_OF_COLORS << " instead."
      );
    testResult &= false;
    }

  vtkSmartPointer<vtkStringArray> vs =
    vtkSmartPointer<vtkStringArray>::New();
  nc->GetColorNames(vs);
  if ( vs->GetNumberOfValues() != NUMBER_OF_COLORS )
  {
    vtkGenericWarningMacro(
      << "Fail: GetColorNames(), incorrect number of colors"
      << "found " <<
      vs->GetNumberOfValues() << ", expected "
      << NUMBER_OF_COLORS << " instead."
      );
    testResult &= false;
  }

  std::ostringstream os;
  nc->PrintSelf(os,vtkIndent(2));
  //std::cout << os.str() << std::endl;
  if ( static_cast<int>(os.str().size()) != PRINT_SELF_STRING_SIZE )
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
