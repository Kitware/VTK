/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorString.cxx

  Copyright (c) Marco Cecchetti
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkColorString.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkNamedColors.h"

#include <string.h>
#include <climits>
#include <iostream>


//------------------------------------------------------------------------------
//#define DEBUG_PARSING
#ifdef DEBUG_PARSING
  #define DBGLOG( expr ) std::cout expr << "\n";
#else
  #define DBGLOG( expr )
#endif


//------------------------------------------------------------------------------
// Helper class for parsing a string which defines a RGB or RGBA color.
class ColorStringParser
{
public:
  ColorStringParser()
  {
    this->CStr = NULL;
    this->Color.Set(0, 0, 0, 255);
    this->StateGood = true;
    this->NamedColors = vtkSmartPointer<vtkNamedColors>::New();
  }

  bool Parse(const char* str);

  const vtkColor4ub& GetColor() const
  {
    return this->Color;
  }

private:
  void SkipSpaces()
  {
    while (*CStr == ' ' || *CStr == '\t') { ++CStr; }
  }

  unsigned char ParseByte();
  unsigned char ParseHexByte();

  void RGBAFuncStringToRGBA(bool withAlpha);
  void HexStringToRGBA();

private:
  const char* CStr;
  vtkColor4ub Color;
  bool StateGood;
  vtkSmartPointer<vtkNamedColors> NamedColors;
};


//------------------------------------------------------------------------------
// Parse a color string in any of the following format:
// - #RRGGBB
// - #RRGGBBAA
// - rgb(r, g, b)
// - rgba(r, g, b, a)
// - a CSS3 color name, e.g. "steelblue"
// If the passed string defines a color using one of the above formats returns
// true else returns false.
bool ColorStringParser::Parse(const char* color)
{
  if (!color) return false;

  this->StateGood = true;
  this->CStr = color;
  this->SkipSpaces();
  if (*CStr == '#')
    {
    ++CStr;
    this->HexStringToRGBA();
    }
  else if (color == strstr(color, "rgba"))
    {
    CStr += 4;
    this->RGBAFuncStringToRGBA(true);
    }
  else if (color == strstr(color, "rgb"))
    {
    CStr += 3;
    this->RGBAFuncStringToRGBA(false);
    }
  else
    {
    this->StateGood = this->NamedColors->ColorExists(color);
    if (this->StateGood)
        {
        this->NamedColors->GetColor(color, this->Color);
        }
    }

  if (!this->StateGood)
    {
    this->Color.Set(0, 0, 0, 0);
    }
  return this->StateGood;
}


//------------------------------------------------------------------------------
// Parse a positive integer with value in the range [0, 255].
// If the parsed string is not valid set `StateGood` to false.
unsigned char ColorStringParser::ParseByte()
{

  DBGLOG( << "parseByte: '" << str << "'" )
  static const unsigned char BASE = 10;

  // The current digit character.
  // Useful in order to avoid to dereference the same pointer many times.
  char digit;

  // Parsed value is assigned to n.
  unsigned char n = 0;

  // Skip leading zeros.
  const char* p = CStr; // used to check if we have parsed at least one zero
  while (*CStr == '0') { ++CStr; }

  // Parse first non-zero digit (d1).
  digit = *CStr;
  if (digit > '0' && digit <= '9') // n = d1
    {
    n = digit - '0';
    ++CStr;
    }
  else // not a digit
    {
    // if (str == p) --> no zero parsed --> no digit found --> not a valid string
    // else we have parsed at least one zero --> string is valid and 0 is returned.
    this->StateGood = !(CStr == p);
    return 0;
    }
  // Parse second digit (d2).
  digit = *CStr;
  if (digit >= '0' && digit <= '9') // n = d1*10 + d2
    {
    n *= BASE;
    n += (digit - '0');
    ++CStr;
    }
  else // not a digit --> single digit string, return d1
    {
    return n;
    }
  // Parse third digit (d3).
  digit = *CStr;
  if (digit >= '0' && digit <= '9') // n = d1*100 + d2*10 + d3
    {
    unsigned char d = digit - '0';
    if ( n  <= ((UCHAR_MAX - d) / BASE) ) // <==> (d1*100 + d2*10 + d3 <= 255)
      {
      n *= BASE;
      n += d;
      ++CStr;
      }
    else // parsed string gives a value greater than 255 --> string is not valid
      {
      this->StateGood = false;
      return 0;
      }
    }
  // We have parsed a valid 2 or 3 digits string.
  // We do not mind if there are left digits,
  // the caller method takes care of such a case.
  return n;
}


//------------------------------------------------------------------------------
// Parse a string of type "rgb(r, g, b)" or a "rgba(r, g, b, a)" and return
// the parsed value in `Color`.
// The function supposes that the `CStr` pointer points to the next character
// of the "rgb" or the "rgba" heading substrings.
// If the parsed string is not valid set `StateGood` to false.
void ColorStringParser::RGBAFuncStringToRGBA(bool withAlpha)
{
  // Parse '('.
  this->SkipSpaces();
  this->StateGood = (*CStr++ == '(');
  if (!this->StateGood) return;

  // Parse first argument (red).
  this->SkipSpaces();
  this->Color[0] = this->ParseByte();
  if (!this->StateGood) return;

  // Parse delimiter.
  this->SkipSpaces();
  this->StateGood = (*CStr++ == ',');
  if (!this->StateGood) return;

  // Parse second argument (green)
  this->SkipSpaces();
  this->Color[1] = this->ParseByte();
  if (!this->StateGood) return;

  // Parse delimiter.
  this->SkipSpaces();
  this->StateGood = (*CStr++ == ',');
  if (!this->StateGood) return;

  // Parse third argument (blue)
  this->SkipSpaces();
  this->Color[2] = this->ParseByte();
  if (!this->StateGood) return;

  if (withAlpha) // rgba function
    {
    // Parse delimiter.
    this->SkipSpaces();
    this->StateGood = (*CStr++ == ',');
    if (!this->StateGood) return;

    // Parse 4th argument (alpha)
    this->SkipSpaces();
    this->Color[3] = this->ParseByte();
    if (!this->StateGood) return;
    }
  else // rgb function
    {
    this->Color[3] = 255;
    }

  // Parse ')'.
  this->SkipSpaces();
  this->StateGood = (*CStr++ == ')');
  if (!this->StateGood) return;

  // Left characters must be only trailing spaces or the string is not valid.
  this->SkipSpaces();
  this->StateGood = (*CStr++ == '\0');
}


//------------------------------------------------------------------------------
// Parse a single hexadecimal digit to a base 10 integer.
// If the passed character is not a valid hexadecimal digit UCHAR_MAX is returned.
static unsigned char toB10(char d)
{
  switch (d) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return d - '0';
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
      return (d - 'a') + 10;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      return (d - 'A') + 10;
  }
  return UCHAR_MAX;
}


//------------------------------------------------------------------------------
// Parse a two digit hexadecimal positive integer.
// If the parsed string is not valid set `StateGood` to false.
unsigned char ColorStringParser::ParseHexByte()
{
  DBGLOG( << "parseHexByte: '" << str << "'" )
  unsigned int n = toB10(*CStr++);
  n <<= 4;
  n += toB10(*CStr++);
  this->StateGood = !(n > UCHAR_MAX);
  return (this->StateGood) ? static_cast<unsigned char>(n) : 0;
}


//------------------------------------------------------------------------------
// Parse a string of type "#RRGGBB" or "#RRGGBBAA".
// Only heading and trailing spaces are allowed.
// If the parsed string is not valid set `StateGood` to false.
void ColorStringParser::HexStringToRGBA()
{
  // Parse red.
  this->Color[0] = this->ParseHexByte();
  if (!this->StateGood) return;

  // Parse green.
  this->Color[1] = this->ParseHexByte();
  if (!this->StateGood) return;

  // Parse blue.
  this->Color[2] = this->ParseHexByte();
  if (!this->StateGood) return;

  // Do we need to parse alpha ?
  const char* p = CStr;
  this->SkipSpaces();
  if (*CStr == '\0') // we parsed #RRGGBB
    {
    this->Color[3] = 255;
    }
  else if (CStr == p) // #RRGGBBAA ? (note that "#RRGGBB AA" is not valid)
    {
    // Parse alpha.
    this->Color[3] = this->ParseHexByte();
    if (!this->StateGood) return;

    // Left characters must be only trailing spaces or the string is not valid.
    this->SkipSpaces();
    this->StateGood = (*CStr == '\0');
    }
  else // e.g. "#80FACC 0F"
    {
    this->StateGood = false;
    }
}




//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorString);


//------------------------------------------------------------------------------
void vtkColorString::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//------------------------------------------------------------------------------
vtkColorString::vtkColorString()
{
  this->Color.Set(0, 0, 0, 255);
  this->Parser = new ColorStringParser();
}


//------------------------------------------------------------------------------
vtkColorString::~vtkColorString()
{
  if (this->Parser)
    {
    delete this->Parser;
    }
}


//------------------------------------------------------------------------------
bool vtkColorString::SetColor(const char* color)
{
  bool parseGood = this->Parser->Parse(color);
  const vtkColor4ub& c4ub = this->Parser->GetColor();
  if (this->Color != c4ub)
    {
    this->Color = c4ub;
    this->Modified();
    }
  return parseGood;
}


//------------------------------------------------------------------------------
bool vtkColorString::SetColor(const vtkStdString& color)
{
  return this->SetColor(color.c_str());
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(vtkColor4ub& color) const
{
  color = this->Color;
}


//------------------------------------------------------------------------------
vtkColor4ub vtkColorString::GetColor4ub() const
{
  return this->Color;
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(vtkColor4d& color) const
{
  for (unsigned int i = 0; i < 4; ++i)
    {
    color[i] = (this->Color[i] / 255.0);
    }
}


//------------------------------------------------------------------------------
vtkColor4d vtkColorString::GetColor4d() const
{
  vtkColor4d color;
  this->GetColor(color);
  return color;
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(vtkColor3ub& color) const
{
  for (unsigned int i = 0; i < 3; ++i)
    {
    color[i] = this->Color[i];
    }

}


//------------------------------------------------------------------------------
vtkColor3ub vtkColorString::GetColor3ub() const
{
  vtkColor3ub color;
  this->GetColor(color);
  return color;
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(vtkColor3d& color) const
{
  for (unsigned int i = 0; i < 3; ++i)
    {
    color[i] = (this->Color[i] / 255.0);
    }

}


//------------------------------------------------------------------------------
vtkColor3d vtkColorString::GetColor3d() const
{
  vtkColor3d color;
  this->GetColor(color);
  return color;
}


//------------------------------------------------------------------------------
void vtkColorString::GetColorRGBA(unsigned char color[4]) const
{
  for (unsigned int i = 0; i < 4; ++i)
    {
    color[i] = this->Color[i];
    }
}


//------------------------------------------------------------------------------
void vtkColorString::GetColorRGBA(double color[4]) const
{
  for (unsigned int i = 0; i < 4; ++i)
    {
    color[i] = (this->Color[i] / 255.0);
    }
}


//------------------------------------------------------------------------------
void vtkColorString::GetColorRGB(unsigned char color[3]) const
{
  for (unsigned int i = 0; i < 3; ++i)
    {
    color[i] = this->Color[i];
    }
}


//------------------------------------------------------------------------------
void vtkColorString::GetColorRGB(double color[3]) const
{
  for (unsigned int i = 0; i < 3; ++i)
    {
    color[i] = (this->Color[i] / 255.0);
    }
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(unsigned char& r, unsigned char& g,
                              unsigned char& b, unsigned char& a) const
{
  r = this->Color[0];
  g = this->Color[1];
  b = this->Color[2];
  a = this->Color[3];
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(double& r, double& g, double& b, double& a) const
{
  r = this->Color[0] / 255.0;
  g = this->Color[1] / 255.0;
  b = this->Color[2] / 255.0;
  a = this->Color[3] / 255.0;
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(unsigned char& r,
                              unsigned char& g,
                              unsigned char& b) const
{
  r = this->Color[0];
  g = this->Color[1];
  b = this->Color[2];
}


//------------------------------------------------------------------------------
void vtkColorString::GetColor(double& r, double& g, double& b) const
{
  r = this->Color[0] / 255.0;
  g = this->Color[1] / 255.0;
  b = this->Color[2] / 255.0;
}
