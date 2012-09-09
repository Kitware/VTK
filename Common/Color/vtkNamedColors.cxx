/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNamedColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNamedColors.h"

#include "vtkObjectFactory.h"

#include <iostream>
#include <map> // STL Header for the underlying color map
#include <sstream> // STL Header for parsing character arrays
#include <algorithm> // STL Header for case conversion of characters
#include <functional> // STL Header for case conversion of characters
#include <locale> // STL Header for case conversion of characters
#include <cstdlib>


class vtkNamedColorsDataStore
{
public:
  vtkNamedColorsDataStore()
  {
    this->Init();
  }
  virtual ~vtkNamedColorsDataStore()
  {
  }

public:
  //-----------------------------------------------------------------------------
  // Return a pointer to the internal color map.
  // Use with caution.
  // Likley uses could be for searching for colors that have the same
  // value but different names e.g. aqua and cyan.
  std::map<vtkStdString, std::vector<unsigned char> > * GetColorMap()
  {
    return &this->colorMap;
  }

  //-----------------------------------------------------------------------------
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as an unsigned char vector:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // The color vector is empty if the color is not found.
  std::vector<unsigned char> GetColorAsUnsignedCharVector(const vtkStdString & name)
  {
    if ( !name.empty() )
      {
      vtkStdString s = this->ToLowercase(name);
      std::map<vtkStdString, std::vector<unsigned char> >::iterator p = this->colorMap.find(s);
      if ( p != this->colorMap.end() )
        {
        return p->second;
        }
      }
    std::vector<unsigned char> v;
    return v;
  }

  //-----------------------------------------------------------------------------
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as a double vector:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // The color vector is empty if the color is not found.
  std::vector<double> GetColorAsDoubleVector(const vtkStdString & name)
  {
    std::vector<double> v;
    if ( !name.empty() )
      {
      std::vector< unsigned char > uv = this->GetColorAsUnsignedCharVector(name);
      for ( std::vector<unsigned char>::iterator p = uv.begin(); p != uv.end(); ++p )
        {
        v.push_back(static_cast<double>(*p) / 255.0);
        }
      }
    return v;
  }

  //-----------------------------------------------------------------------------
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is an unsigned char array:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // No color is set if the name is empty or the color vector is not of size 4.
  void SetColor(const vtkStdString & name, const std::vector<unsigned char> & color)
  {
    if ( !name.empty() && color.size() == 4 )
      {
      vtkStdString s = this->ToLowercase(name);
      this->colorMap[s] = color;
      }
  }

  //-----------------------------------------------------------------------------
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a double array:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // No color is set if the name is empty or the color vector is not of size 4.
  void SetColor(const vtkStdString & name,  const std::vector<double> & color)
  {
    if ( !name.empty() && color.size() == 4 )
      {
      vtkStdString s = this->ToLowercase(name);
      std::vector<unsigned char> v;
      for(std::vector<double>::const_iterator p = color.begin(); p != color.end(); ++p)
        {
        v.push_back(static_cast<unsigned char>(*p * 255.0));
        }
      this->colorMap[s] = v;
      }
  }

  //-----------------------------------------------------------------------------
  // Remove the color by name.
  // The name is treated as being case-insensitive.
  void RemoveColor(const vtkStdString & name)
  {
    if (!name.empty())
      {
      vtkStdString s = this->ToLowercase(name);
      std::map<vtkStdString, std::vector<unsigned char> >::iterator p = this->colorMap.find(s);
      if ( p != this->colorMap.end() )
        {
          this->colorMap.erase(p);
        }
      }
  }

  //-----------------------------------------------------------------------------
  // Return true if the color exists.
  bool ColorExists(const vtkStdString & name)
  {
    vtkStdString s = this->ToLowercase(name);
    return this->colorMap.find(s) != this->colorMap.end();
  }

public:
  //-----------------------------------------------------------------------------
  // Initialise the color map by loading the colors from a list.
  void Init()
  {
    // These are the standard names and corresponding colors.
    // This table is loaded into a map where the key is the name
    // and the vector corresponds to the RGBA values.
    // Warning: If you add more colors to this table ensure that
    //          the format exactly matches those below.
    //          If you do not, the parsing into the map will most
    //          likely fail.
    //          The names you add can have spaces in them however
    //          the names MUST be all lower case.
    // Note: The colors in the last part of this table are from
    //       http://en.wikipedia.org/wiki/Web_colors.
    //       The names and values here will tale precedence over
    //       earlier names.
  static const char *colorTable[] =
    {
    // Name, R, G, B, A
    // These colors should be exactly the same as the ones defined in
    // colors.tcl.
    //  Whites
    "antique_white,250,235,214,255",
    "azure,240,255,255,255",
    "bisque,255,227,195,255",
    "blanched_almond,255,235,204,255",
    "cornsilk,255,247,219,255",
    "eggshell,252,229,201,255",
    "floral_white,255,250,240,255",
    "gainsboro,219,219,219,255",
    "ghost_white,247,247,255,255",
    "honeydew,240,255,240,255",
    "ivory,255,255,240,255",
    "lavender,230,230,250,255",
    "lavender_blush,255,240,245,255",
    "lemon_chiffon,255,250,204,255",
    "linen,250,240,230,255",
    "mint_cream,245,255,250,255",
    "misty_rose,255,227,225,255",
    "moccasin,255,227,180,255",
    "navajo_white,255,222,172,255",
    "old_lace,253,245,230,255",
    "papaya_whip,255,239,213,255",
    "peach_puff,255,217,185,255",
    "seashell,255,245,237,255",
    "snow,255,250,250,255",
    "thistle,216,190,216,255",
    "titanium_white,252,255,239,255",
    "wheat,245,222,179,255",
    "white,255,255,255,255",
    "white_smoke,245,245,245,255",
    "zinc_white,252,247,255,255",
    //  Greys
    "cold_grey,127,137,135,255",
    "dim_grey,105,105,105,255",
    "grey,191,191,191,255",
    "light_grey,211,211,211,255",
    "slate_grey,111,128,143,255",
    "slate_grey_dark,46,78,78,255",
    "slate_grey_light,119,135,153,255",
    "warm_grey,127,127,104,255",
    //  Blacks
    "black,0,0,0,255",
    "ivory_black,40,35,33,255",
    "lamp_black,45,71,58,255",
    //  Reds
    "alizarin_crimson,226,38,53,255",
    "brick,155,102,30,255",
    "cadmium_red_deep,226,22,12,255",
    "coral,255,126,79,255",
    "coral_light,240,128,128,255",
    "deep_pink,255,19,147,255",
    "english_red,211,61,25,255",
    "firebrick,177,33,33,255",
    "geranium_lake,226,17,48,255",
    "hot_pink,255,105,180,255",
    "indian_red,175,22,30,255",
    "light_salmon,255,160,121,255",
    "madder_lake_deep,226,45,48,255",
    "maroon,176,47,96,255",
    "pink,255,191,203,255",
    "pink_light,255,181,193,255",
    "raspberry,135,38,86,255",
    "red,255,0,0,255",
    "rose_madder,226,53,56,255",
    "salmon,250,128,114,255",
    "tomato,255,98,70,255",
    "venetian_red,211,25,30,255",
    //  Browns
    "beige,163,147,127,255",
    "brown,127,41,41,255",
    "brown_madder,219,40,40,255",
    "brown_ochre,135,66,30,255",
    "burlywood,222,184,134,255",
    "burnt_sienna,137,53,15,255",
    "burnt_umber,137,51,35,255",
    "chocolate,209,105,29,255",
    "deep_ochre,114,61,25,255",
    "flesh,255,124,63,255",
    "flesh_ochre,255,86,33,255",
    "gold_ochre,198,119,38,255",
    "greenish_umber,255,61,12,255",
    "khaki,240,230,139,255",
    "khaki_dark,189,182,106,255",
    "light_beige,245,245,219,255",
    "peru,204,133,63,255",
    "rosy_brown,188,143,143,255",
    "raw_sienna,198,96,20,255",
    "raw_umber,114,73,17,255",
    "sepia,94,38,17,255",
    "sienna,160,82,45,255",
    "saddle_brown,139,69,18,255",
    "sandy_brown,244,163,96,255",
    "tan,209,180,139,255",
    "van_dyke_brown,94,38,5,255",
    //  Oranges
    "cadmium_orange,255,96,2,255",
    "cadmium_red_light,255,2,12,255",
    "carrot,237,145,33,255",
    "dark_orange,255,139,0,255",
    "mars_orange,150,68,20,255",
    "mars_yellow,226,112,25,255",
    "orange,255,127,0,255",
    "orange_red,255,69,0,255",
    "yellow_ochre,226,130,22,255",
    //  Yellows
    "aureoline_yellow,255,168,35,255",
    "banana,226,206,86,255",
    "cadmium_lemon,255,226,2,255",
    "cadmium_yellow,255,153,17,255",
    "cadmium_yellow_light,255,175,15,255",
    "gold,255,214,0,255",
    "goldenrod,217,165,32,255",
    "goldenrod_dark,184,134,10,255",
    "goldenrod_light,250,250,209,255",
    "goldenrod_pale,237,231,170,255",
    "light_goldenrod,237,221,129,255",
    "melon,226,168,104,255",
    "naples_yellow_deep,255,168,17,255",
    "yellow,255,255,0,255",
    "yellow_light,255,255,223,255",
    //  Greens
    "chartreuse,126,255,0,255",
    "chrome_oxide_green,102,127,20,255",
    "cinnabar_green,96,178,40,255",
    "cobalt_green,61,145,63,255",
    "emerald_green,0,201,86,255",
    "forest_green,33,139,33,255",
    "green,0,255,0,255",
    "green_dark,0,100,0,255",
    "green_pale,152,250,152,255",
    "green_yellow,172,255,46,255",
    "lawn_green,124,251,0,255",
    "lime_green,50,204,50,255",
    "mint,188,252,201,255",
    "olive,58,94,43,255",
    "olive_drab,106,142,35,255",
    "olive_green_dark,84,106,46,255",
    "permanent_green,10,201,43,255",
    "sap_green,48,127,20,255",
    "sea_green,46,139,87,255",
    "sea_green_dark,143,188,143,255",
    "sea_green_medium,60,179,112,255",
    "sea_green_light,32,177,170,255",
    "spring_green,0,255,126,255",
    "spring_green_medium,0,250,153,255",
    "terre_verte,56,94,15,255",
    "viridian_light,109,255,112,255",
    "yellow_green,153,204,50,255",
    //  Cyans
    "aquamarine,126,255,212,255",
    "aquamarine_medium,102,204,170,255",
    "cyan,0,255,255,255",
    "cyan_white,223,255,255,255",
    "turquoise,64,223,208,255",
    "turquoise_dark,0,205,208,255",
    "turquoise_medium,72,208,204,255",
    "turquoise_pale,175,237,237,255",
    //  Blues
    "alice_blue,240,247,255,255",
    "blue,0,0,255,255",
    "blue_light,172,216,230,255",
    "blue_medium,0,0,204,255",
    "cadet,94,157,160,255",
    "cobalt,61,89,170,255",
    "cornflower,100,148,236,255",
    "cerulean,5,183,204,255",
    "dodger_blue,29,143,255,255",
    "indigo,7,45,84,255",
    "manganese_blue,2,168,158,255",
    "midnight_blue,24,24,111,255",
    "navy,0,0,128,255",
    "peacock,51,160,201,255",
    "powder_blue,176,223,230,255",
    "royal_blue,64,105,225,255",
    "slate_blue,106,89,204,255",
    "slate_blue_dark,72,60,139,255",
    "slate_blue_light,131,111,255,255",
    "slate_blue_medium,123,103,237,255",
    "sky_blue,134,205,235,255",
    "sky_blue_deep,0,190,255,255",
    "sky_blue_light,134,205,250,255",
    "steel_blue,69,129,180,255",
    "steel_blue_light,176,195,222,255",
    "turquoise_blue,0,198,140,255",
    "ultramarine,17,10,142,255",
    //  Magentas
    "blue_violet,138,42,226,255",
    "cobalt_violet_deep,145,33,158,255",
    "magenta,255,0,255,255",
    "orchid,217,111,213,255",
    "orchid_dark,153,50,204,255",
    "orchid_medium,185,84,211,255",
    "permanent_red_violet,219,38,68,255",
    "plum,221,160,221,255",
    "purple,160,32,240,255",
    "purple_medium,147,111,218,255",
    "ultramarine_violet,91,35,109,255",
    "violet,142,94,153,255",
    "violet_dark,148,0,211,255",
    "violet_red,208,32,143,255",
    "violet_red_medium,199,21,133,255",
    "violet_red_pale,218,111,147,255",
    // See: http://en.wikipedia.org/wiki/Web_colors
    // Red colors
    "IndianRed,205,92,92,255",
    "LightCoral,240,128,128,255",
    "Salmon,250,128,114,255",
    "DarkSalmon,233,150,122,255",
    "LightSalmon,255,160,122,255",
    "Red,255,0,0,255",
    "Crimson,220,20,60,255",
    "FireBrick,178,34,34,255",
    "DarkRed,139,0,0,255",
    // Pink colors
    "Pink,255,192,203,255",
    "LightPink,255,182,193,255",
    "HotPink,255,105,180,255",
    "DeepPink,255,20,147,255",
    "MediumVioletRed,199,21,133,255",
    "PaleVioletRed,219,112,147,255",
    // Orange colors
    "LightSalmon,255,160,122,255",
    "Coral,255,127,80,255",
    "Tomato,255,99,71,255",
    "OrangeRed,255,69,0,255",
    "DarkOrange,255,140,0,255",
    "Orange,255,165,0,255",
    // Yellow colors
    "Gold,255,215,0,255",
    "Yellow,255,255,0,255",
    "LightYellow,255,255,224,255",
    "LemonChiffon,255,250,205,255",
    "LightGoldenrodYellow,250,250,210,255",
    "PapayaWhip,255,239,213,255",
    "Moccasin,255,228,181,255",
    "PeachPuff,255,218,185,255",
    "PaleGoldenrod,238,232,170,255",
    "Khaki,240,230,140,255",
    "DarkKhaki,189,183,107,255",
    // Purple colors
    "Lavender,230,230,250,255",
    "Thistle,216,191,216,255",
    "Plum,221,160,221,255",
    "Violet,238,130,238,255",
    "Orchid,218,112,214,255",
    "Fuchsia,255,0,255,255",
    "Magenta,255,0,255,255",
    "MediumOrchid,186,85,211,255",
    "MediumPurple,147,112,219,255",
    "BlueViolet,138,43,226,255",
    "DarkViolet,148,0,211,255",
    "DarkOrchid,153,50,204,255",
    "DarkMagenta,139,0,139,255",
    "Purple,128,0,128,255",
    "Indigo,75,0,130,255",
    "DarkSlateBlue,72,61,139,255",
    "SlateBlue,106,90,205,255",
    "MediumSlateBlue,123,104,238,255",
    // Green colors
    "GreenYellow,173,255,47,255",
    "Chartreuse,127,255,0,255",
    "LawnGreen,124,252,0,255",
    "Lime,0,255,0,255",
    "LimeGreen,50,205,50,255",
    "PaleGreen,152,251,152,255",
    "LightGreen,144,238,144,255",
    "MediumSpringGreen,0,250,154,255",
    "SpringGreen,0,255,127,255",
    "MediumSeaGreen,60,179,113,255",
    "SeaGreen,46,139,87,255",
    "ForestGreen,34,139,34,255",
    "Green,0,128,0,255",
    "DarkGreen,0,100,0,255",
    "YellowGreen,154,205,50,255",
    "OliveDrab,107,142,35,255",
    "Olive,128,128,0,255",
    "DarkOliveGreen,85,107,47,255",
    "MediumAquamarine,102,205,170,255",
    "DarkSeaGreen,143,188,143,255",
    "LightSeaGreen,32,178,170,255",
    "DarkCyan,0,139,139,255",
    "Teal,0,128,128,255",
    // Blue/Cyan colors
    "Aqua,0,255,255,255",
    "Cyan,0,255,255,255",
    "LightCyan,224,255,255,255",
    "PaleTurquoise,175,238,238,255",
    "Aquamarine,127,255,212,255",
    "Turquoise,64,224,208,255",
    "MediumTurquoise,72,209,204,255",
    "DarkTurquoise,0,206,209,255",
    "CadetBlue,95,158,160,255",
    "SteelBlue,70,130,180,255",
    "LightSteelBlue,176,196,222,255",
    "PowderBlue,176,224,230,255",
    "LightBlue,173,216,230,255",
    "SkyBlue,135,206,235,255",
    "LightSkyBlue,135,206,250,255",
    "DeepSkyBlue,0,191,255,255",
    "DodgerBlue,30,144,255,255",
    "CornflowerBlue,100,149,237,255",
    "RoyalBlue,65,105,225,255",
    "Blue,0,0,255,255",
    "MediumBlue,0,0,205,255",
    "DarkBlue,0,0,139,255",
    "Navy,0,0,128,255",
    "MidnightBlue,25,25,112,255",
    // Brown colors
    "Cornsilk,255,248,220,255",
    "BlanchedAlmond,255,235,205,255",
    "Bisque,255,228,196,255",
    "NavajoWhite,255,222,173,255",
    "Wheat,245,222,179,255",
    "BurlyWood,222,184,135,255",
    "Tan,210,180,140,255",
    "RosyBrown,188,143,143,255",
    "SandyBrown,244,164,96,255",
    "Goldenrod,218,165,32,255",
    "DarkGoldenrod,184,134,11,255",
    "Peru,205,133,63,255",
    "Chocolate,210,105,30,255",
    "SaddleBrown,139,69,19,255",
    "Sienna,160,82,45,255",
    "Brown,165,42,42,255",
    "Maroon,128,0,0,255",
    // White colors
    "White,255,255,255,255",
    "Snow,255,250,250,255",
    "Honeydew,240,255,240,255",
    "MintCream,245,255,250,255",
    "Azure,240,255,255,255",
    "AliceBlue,240,248,255,255",
    "GhostWhite,248,248,255,255",
    "WhiteSmoke,245,245,245,255",
    "Seashell,255,245,238,255",
    "Beige,245,245,220,255",
    "OldLace,253,245,230,255",
    "FloralWhite,255,250,240,255",
    "Ivory,255,255,240,255",
    "AntiqueWhite,250,235,215,255",
    "Linen,250,240,230,255",
    "LavenderBlush,255,240,245,255",
    "MistyRose,255,228,225,255",
    // Gray colors
    "Gainsboro,220,220,220,255",
    "LightGrey,211,211,211,255",
    "Silver,192,192,192,255",
    "DarkGray,169,169,169,255",
    "Gray,128,128,128,255",
    "DimGray,105,105,105,255",
    "LightSlateGray,119,136,153,255",
    "SlateGray,112,128,144,255",
    "DarkSlateGray,47,79,79,255",
    "Black,0,0,0,255",
    };

    // Here we fill the colorMap.
    size_t colorTableSz = sizeof(colorTable)/sizeof(colorTable[0]);
    for (size_t i = 0; i < colorTableSz; ++i)
    {
    std::vector<unsigned char> color;
    vtkStdString s = colorTable[i];
    size_t idx = s.find(',');
    vtkStdString name = s.substr(0,idx);
    vtkStdString t = this->ToLowercase(name);
    name = t;
    size_t start = idx + 1;
    idx = s.find(',',start);
    t = s.substr(start,idx - start); // Red
    color.push_back(static_cast<unsigned char>(atoi(t.c_str())));
    start = idx + 1;
    idx = s.find(',',start);
    t = s.substr(start,idx - start); // Green
    color.push_back(static_cast<unsigned char>(atoi(t.c_str())));
    start = idx + 1;
    idx = s.find(',',start);
    t = s.substr(start,idx - start); // Blue
    color.push_back(static_cast<unsigned char>(atoi(t.c_str())));
    start = idx + 1;
    t = s.substr(start,s.size() - start); // Alpha
    color.push_back(static_cast<unsigned char>(atoi(t.c_str())));
    this->colorMap[name] = color;
    }
  }

private:
  //-----------------------------------------------------------------------------
  // Convert a string to lowercase.
  vtkStdString ToLowercase(const vtkStdString & str)
  {
      vtkStdString s = str;
      std::transform(str.begin(),str.end(),s.begin(),
        std::bind2nd(std::ptr_fun(&std::tolower<char>), std::locale("")));
      return s;
  }

  // The names and color values.
  std::map<vtkStdString, std::vector<unsigned char> > colorMap;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkNamedColors);

//----------------------------------------------------------------------------
vtkNamedColors::vtkNamedColors()
{
  this->colors = new vtkNamedColorsDataStore;
}

//----------------------------------------------------------------------------
vtkNamedColors::~vtkNamedColors()
{
  delete this->colors;
}

//-----------------------------------------------------------------------------

void vtkNamedColors::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for ( std::map<vtkStdString, std::vector<unsigned char> >::const_iterator p =
    this->colors->GetColorMap()->begin();
    p != this->colors->GetColorMap()->end(); ++p )
    {
    os << indent << ": " << p->first << "(";
    if ( !p->second.empty() )
      {
      std::vector<unsigned char>::const_iterator lastItr = --p->second.end();
      for (std::vector<unsigned char>::const_iterator q =
        p->second.begin(); q != p->second.end(); ++q )
        {
        if ( q != lastItr )
          {
          os << static_cast<int>(*q) << ",";
          }
        else
          {
          os << static_cast<int>(*q);
          }
        }
      }
      os << ")" << endl;
    }
}

//-----------------------------------------------------------------------------
std::vector<vtkStdString> vtkNamedColors::GetColorNames()
{
  std::vector<vtkStdString> colorNames;
  for(std::map<vtkStdString, std::vector<unsigned char> >::const_iterator p =
    this->colors->GetColorMap()->begin();
    p != this->colors->GetColorMap()->end(); ++p )
    {
    colorNames.push_back(p->first);
    }
  return colorNames;
}

//-----------------------------------------------------------------------------
std::vector<std::vector<vtkStdString> > vtkNamedColors::GetSynonyms()
{
  std::vector<vtkStdString> cn = this->GetColorNames();
  std::map<vtkStdString,std::vector<vtkStdString> > synonyms;
  for ( std::vector<vtkStdString>::const_iterator p =
    cn.begin(); p != cn.end(); ++p )
    {
      bool foundDuplicate = false;
      std::vector<unsigned char> vu =
        this->colors->GetColorAsUnsignedCharVector(*p);
      std::vector<vtkStdString> duplicates;
      for ( std::map<vtkStdString, std::vector<unsigned char> >::const_iterator
        q = this->colors->GetColorMap()->begin();
        q != this->colors->GetColorMap()->end(); ++q )
      {
        if ( *p != q->first && vu == q->second )
          {
          duplicates.push_back(q->first);
          }
      }
      if ( !duplicates.empty() )
        {
          bool alreadyInMap = false;
          for( std::vector<vtkStdString>::const_iterator r =
            duplicates.begin(); r != duplicates.end(); ++r)
          {
          if( synonyms.find(*r) != synonyms.end() )
            {
            alreadyInMap = true;
            }
          }
          if ( !alreadyInMap )
            {
            synonyms[*p] = duplicates;
            }
        }
    }
  std::vector<std::vector<vtkStdString> > retVec;
  for(std::map<vtkStdString,std::vector<vtkStdString> >::const_iterator p =
    synonyms.begin(); p != synonyms.end(); ++p)
    {
      std::vector<vtkStdString> vstr;
      vstr.push_back(p->first);
      for(std::vector<vtkStdString>::const_iterator q =
        p->second.begin(); q!= p->second.end(); ++q)
        {
        vstr.push_back(*q);
        }
      retVec.push_back(vstr);
    }
  return retVec;
}

//-----------------------------------------------------------------------------
int vtkNamedColors::GetNumberOfColors()
{
  return static_cast<int>(this->colors->GetColorMap()->size());
}

//-----------------------------------------------------------------------------
void vtkNamedColors::ResetColors()
{
  this->colors->GetColorMap()->clear();
  this->colors->Init();
}

//-----------------------------------------------------------------------------
bool vtkNamedColors::ColorExists(const vtkStdString & name)
{
  return this->colors->ColorExists(name);
}

//-----------------------------------------------------------------------------
unsigned char * vtkNamedColors::GetColorAsUnsignedChar(const vtkStdString & name)
{
  static unsigned char rgba[4];
  for ( size_t i = 0; i < 3; ++i )
    {
    rgba[i] = 0;
    }
  rgba[3] = 255;
  std::vector<unsigned char> v =
    this->colors->GetColorAsUnsignedCharVector(name);
  if ( v.size() == 4 )
  {
    for ( size_t i = 0; i < v.size(); ++i )
      {
      rgba[i] = v[i];
      }
  }
  return rgba;
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name,
                              unsigned char & r, unsigned char & g,
                              unsigned char & b, unsigned char & a)
{
  std::vector<unsigned char> rgba =
    this->colors->GetColorAsUnsignedCharVector(name);
  if ( !rgba.empty() && rgba.size() == 4 )
    {
    r = rgba[0];
    g = rgba[1];
    b = rgba[2];
    a = rgba[3];
    }
  else
    {
    r = g = b = 0;
    a = 255;
    }
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, unsigned char rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = 0;
  rgba[3] = 255;
  std::vector<unsigned char> vcolor =
    this->colors->GetColorAsUnsignedCharVector(name);
  if ( !vcolor.empty() && vcolor.size() == 4 )
  {
  for ( size_t i = 0; i < vcolor.size(); ++i )
    {
    rgba[i] = vcolor[i];
    }
  }
}

//-----------------------------------------------------------------------------
double * vtkNamedColors::GetColorAsDouble(const vtkStdString & name)
{
  static double rgba[4];
  for ( size_t i = 0; i < 3; ++i )
    {
    rgba[i] = 0;
    }
  rgba[3] = 1.0;
  std::vector<double> v = this->colors->GetColorAsDoubleVector(name);
  if ( v.size() == 4 )
    {
    for ( size_t i = 0; i < v.size(); ++i )
      {
      rgba[i] = v[i];
      }
    }
  return rgba;
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name,
                              double & r, double & g, double & b, double & a)
{
  std::vector<double> rgba = this->colors->GetColorAsDoubleVector(name);
  if ( !rgba.empty() && rgba.size() == 4 )
  {
    r = rgba[0];
    g = rgba[1];
    b = rgba[2];
    a = rgba[3];
  }
  else
  {
    r = g = b = 0;
    a = 1.0;
  }
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, double rgba[4])
{
  rgba[0] = rgba[1] = rgba[2] = 0;
  rgba[3] = 1.0;
  std::vector<double> vcolor = this->colors->GetColorAsDoubleVector(name);
  if ( !vcolor.empty() && vcolor.size() == 4 )
    {
    for ( size_t i = 0; i < vcolor.size(); ++i )
      {
      rgba[i] = vcolor[i];
      }
    }
}

//-----------------------------------------------------------------------------
double * vtkNamedColors::GetColorAsDoubleRGB(const vtkStdString & name)
{
  static double rgb[3];
  for ( size_t i = 0; i < 4; ++i )
    {
    rgb[i] = 0;
    }
  std::vector<double> rgba = this->colors->GetColorAsDoubleVector(name);
  if ( rgba.size() == 4 )
    {
    for ( size_t i = 0; i < 3; ++i )
      {
      rgb[i] = rgba[i];
      }
    }
  return rgb;
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColorRGB(const vtkStdString & name,
                                 double & r, double & g, double & b)
{
  std::vector<double> rgba = this->colors->GetColorAsDoubleVector(name);
  if ( !rgba.empty() && rgba.size() == 4 )
  {
    r = rgba[0];
    g = rgba[1];
    b = rgba[2];
  }
  else
  {
    r = g = b = 0;
  }
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColorRGB(const vtkStdString & name, double rgb[3])
{
  rgb[0] = rgb[1] = rgb[2] = 0;
  std::vector<double> rgba = this->colors->GetColorAsDoubleVector(name);
  if ( !rgba.empty() && rgba.size() == 4 )
    {
    for ( size_t i = 0; i < 3; ++i )
      {
      rgb[i] = rgba[i];
      }
    }
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const unsigned char & r, const unsigned char & g,
                              const unsigned char & b, const unsigned char & a)
{
  std::vector<unsigned char> v;
  v.push_back(r);
  v.push_back(g);
  v.push_back(b);
  v.push_back(a);
  this->colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const unsigned char rgba[4])
{
  std::vector<unsigned char> v;
  for ( size_t i = 0; i < 4; ++i )
    {
    v.push_back(rgba[i]);
    }
  this->colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const double & r, const double & g,
                              const double & b, const double & a)
{
  std::vector<double> v;
  v.push_back(r);
  v.push_back(g);
  v.push_back(b);
  v.push_back(a);
  this->colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name, const double rgba[4])
{
  std::vector<double> v;
  for ( size_t i = 0; i < 4; ++i )
    {
    v.push_back(rgba[i]);
    }
  this->colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::RemoveColor(const vtkStdString & name)
{
  if (!name.empty())
    {
    this->colors->RemoveColor(name);
    }
}
