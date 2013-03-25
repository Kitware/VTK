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
#include "vtkSmartPointer.h"

#include <map> // STL Header for the underlying color map
#include <vector> // STL Header for returning/storing color values
#include <sstream> // STL Header for parsing character arrays
#include <algorithm> // STL Header for transform
#include <iterator> // STL Header for back inserter
#include <cctype> // Header to ensure std::tolower is present
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
  // Likely uses could be for searching for colors that have the same
  // value but different names e.g. aqua and cyan.
  std::map<vtkStdString, vtkColor4ub> * GetColorMap()
  {
    return &this->colorMap;
  }

  //-----------------------------------------------------------------------------
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as vtkColor4ub class.
  // Black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor4ub & color)
  {
    for(int i = 0; i < 3; ++i)
      {
      color[i] = 0;
      }
    color[3] = 255;
    if ( !name.empty() )
      {
      vtkStdString s = this->ToLowercase(name);
      std::map<vtkStdString, vtkColor4ub>::iterator p = this->colorMap.find(s);
      if ( p != this->colorMap.end() )
        {
        color = p->second;
        }
      }
  }

  //-----------------------------------------------------------------------------
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as vtkColor4ub class.
  // Black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor4d & color)
  {
    vtkColor4ub c;
    this->GetColor(name,c);
    for(int i = 0; i < c.GetSize(); ++i)
      {
      color[i] = static_cast<double>(c[i] / 255.0);
      }
  }

  //-----------------------------------------------------------------------------
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as vtkColor3ub class.
  // Black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor3ub & color)
  {
    vtkColor4ub c;
    this->GetColor(name,c);
    for(int i = 0; i < color.GetSize(); ++i)
      {
      color[i] = c[i];
      }
  }

  //-----------------------------------------------------------------------------
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as vtkColor3d class.
  // Black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor3d & color)
  {
    vtkColor4d c;
    this->GetColor(name,c);
    for(int i = 0; i < color.GetSize(); ++i)
      {
      color[i] = c[i];
      }
  }

  //-----------------------------------------------------------------------------
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor4ub class.
  // No color is set if the name is empty or the color vector is not of size 4.
  void SetColor(const vtkStdString & name, const vtkColor4ub & color)
  {
    if ( !name.empty() && color.GetSize() == 4 )
      {
      vtkStdString s = this->ToLowercase(name);
      this->colorMap[s] = color;
      }
  }

  //-----------------------------------------------------------------------------
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor4d class.
  // No color is set if the name is empty or the color vector is not of size 4.
  void SetColor(const vtkStdString & name, const vtkColor4d & color)
  {
    if ( !name.empty() && color.GetSize() == 4 )
      {
      vtkStdString s = this->ToLowercase(name);
      std::vector<unsigned char> uv;
      unsigned char r, g, b, a;
      r = static_cast<unsigned char>(color.GetRed() * 255.0);
      g = static_cast<unsigned char>(color.GetGreen() * 255.0);
      b = static_cast<unsigned char>(color.GetBlue() * 255.0);
      a = static_cast<unsigned char>(color.GetAlpha() * 255.0);
      this->colorMap[s] = vtkColor4ub(r,g,b,a);
      }
  }

  //-----------------------------------------------------------------------------
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor3ub class.
  // No color is set if the name is empty or the color vector is not of size 4.
  void SetColor(const vtkStdString & name, const vtkColor3ub & color)
  {
    vtkColor4ub c;
    for(int i = 0; i < 3; ++i)
      {
      c[i] = color[i];
      }
    c[3] = 255;
    this->SetColor(name,c);
  }

  //-----------------------------------------------------------------------------
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor3d class.
  // No color is set if the name is empty or the color vector is not of size 4.
  void SetColor(const vtkStdString & name, const vtkColor3d & color)
  {
    vtkColor4d c;
    for(int i = 0; i < 3; ++i)
      {
        c[i] = color[i];
      }
    c[3] = 1.0;
    this->SetColor(name,c);
  }

  //-----------------------------------------------------------------------------
  // Remove the color by name.
  // The name is treated as being case-insensitive.
  void RemoveColor(const vtkStdString & name)
  {
    if (!name.empty())
      {
      vtkStdString s = this->ToLowercase(name);
      std::map<vtkStdString, vtkColor4ub>::iterator p = this->colorMap.find(s);
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

//-----------------------------------------------------------------------------
// Return a vector of color names.
std::vector<vtkStdString> GetColorNames()
{
  std::vector<vtkStdString> colorNames;
  for(std::map<vtkStdString, vtkColor4ub>::const_iterator p =
    this->GetColorMap()->begin();
    p != this->GetColorMap()->end(); ++p )
    {
    colorNames.push_back(p->first);
    }
  return colorNames;
}

//-----------------------------------------------------------------------------
// Return a vector where each element of the vector is a vector of
// synonyms such as cyan/aqua and magenta/fuchsia
// Warning this could take a long time for very large color maps.
std::vector<std::vector<vtkStdString> > GetSynonyms()
{
  std::vector<vtkStdString> cn = this->GetColorNames();
  std::map<vtkStdString, std::vector<vtkStdString> > synonyms;
  for ( std::vector<vtkStdString>::const_iterator p =
    cn.begin(); p != cn.end(); ++p )
    {
    vtkColor4ub vu;
    this->GetColor(*p,vu);
    std::vector<vtkStdString> duplicates;
    for ( std::map<vtkStdString, vtkColor4ub>::const_iterator
      q = this->GetColorMap()->begin();
      q != this->GetColorMap()->end(); ++q )
      {
      if( *p != q->first && vu.Compare( q->second, 1 ) )
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
          break;
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
    "antique_white,250,235,215,255",
    "azure,240,255,255,255",
    "bisque,255,228,196,255",
    "blanched_almond,255,235,205,255",
    "cornsilk,255,248,220,255",
    "eggshell,252,230,201,255",
    "floral_white,255,250,240,255",
    "gainsboro,220,220,220,255",
    "ghost_white,248,248,255,255",
    "honeydew,240,255,240,255",
    "ivory,255,255,240,255",
    "lavender,230,230,250,255",
    "lavender_blush,255,240,245,255",
    "lemon_chiffon,255,250,205,255",
    "linen,250,240,230,255",
    "mint_cream,245,255,250,255",
    "misty_rose,255,228,225,255",
    "moccasin,255,228,181,255",
    "navajo_white,255,222,173,255",
    "old_lace,253,245,230,255",
    "papaya_whip,255,239,213,255",
    "peach_puff,255,218,185,255",
    "seashell,255,245,238,255",
    "snow,255,250,250,255",
    "thistle,216,191,216,255",
    "titanium_white,252,255,240,255",
    "wheat,245,222,179,255",
    "white,255,255,255,255",
    "white_smoke,245,245,245,255",
    "zinc_white,252,247,255,255",
    //  Greys
    "cold_grey,128,138,135,255",
    "dim_grey,105,105,105,255",
    "grey,192,192,192,255",
    "light_grey,211,211,211,255",
    "slate_grey,112,128,144,255",
    "slate_grey_dark,47,79,79,255",
    "slate_grey_light,119,136,153,255",
    "warm_grey,128,128,105,255",
    //  Blacks
    "black,0,0,0,255",
    "ivory_black,41,36,33,255",
    "lamp_black,46,71,59,255",
    //  Reds
    "alizarin_crimson,227,38,54,255",
    "brick,156,102,31,255",
    "cadmium_red_deep,227,23,13,255",
    "coral,255,127,80,255",
    "coral_light,240,128,128,255",
    "deep_pink,255,20,147,255",
    "english_red,212,61,26,255",
    "firebrick,178,34,34,255",
    "geranium_lake,227,18,48,255",
    "hot_pink,255,105,180,255",
    "indian_red,176,23,31,255",
    "light_salmon,255,160,122,255",
    "madder_lake_deep,227,46,48,255",
    "maroon,176,48,96,255",
    "pink,255,192,203,255",
    "pink_light,255,182,193,255",
    "raspberry,135,38,87,255",
    "red,255,0,0,255",
    "rose_madder,227,54,56,255",
    "salmon,250,128,114,255",
    "tomato,255,99,71,255",
    "venetian_red,212,26,31,255",
    //  Browns
    "beige,163,148,128,255",
    "brown,128,42,42,255",
    "brown_madder,219,41,41,255",
    "brown_ochre,135,66,31,255",
    "burlywood,222,184,135,255",
    "burnt_sienna,138,54,15,255",
    "burnt_umber,138,51,36,255",
    "chocolate,210,105,30,255",
    "deep_ochre,115,61,26,255",
    "flesh,255,125,64,255",
    "flesh_ochre,255,87,33,255",
    "gold_ochre,199,120,38,255",
    "greenish_umber,255,61,13,255",
    "khaki,240,230,140,255",
    "khaki_dark,189,183,107,255",
    "light_beige,245,245,220,255",
    "peru,205,133,63,255",
    "rosy_brown,188,143,143,255",
    "raw_sienna,199,97,20,255",
    "raw_umber,115,74,18,255",
    "sepia,94,38,18,255",
    "sienna,160,82,45,255",
    "saddle_brown,139,69,19,255",
    "sandy_brown,244,164,96,255",
    "tan,210,180,140,255",
    "van_dyke_brown,94,38,5,255",
    //  Oranges
    "cadmium_orange,255,97,3,255",
    "cadmium_red_light,255,3,13,255",
    "carrot,237,145,33,255",
    "dark_orange,255,140,0,255",
    "mars_orange,150,69,20,255",
    "mars_yellow,227,112,26,255",
    "orange,255,128,0,255",
    "orange_red,255,69,0,255",
    "yellow_ochre,227,130,23,255",
    //  Yellows
    "aureoline_yellow,255,168,36,255",
    "banana,227,207,87,255",
    "cadmium_lemon,255,227,3,255",
    "cadmium_yellow,255,153,18,255",
    "cadmium_yellow_light,255,176,15,255",
    "gold,255,215,0,255",
    "goldenrod,218,165,32,255",
    "goldenrod_dark,184,134,11,255",
    "goldenrod_light,250,250,210,255",
    "goldenrod_pale,238,232,170,255",
    "light_goldenrod,238,221,130,255",
    "melon,227,168,105,255",
    "naples_yellow_deep,255,168,18,255",
    "yellow,255,255,0,255",
    "yellow_light,255,255,224,255",
    //  Greens
    "chartreuse,127,255,0,255",
    "chrome_oxide_green,102,128,20,255",
    "cinnabar_green,97,179,41,255",
    "cobalt_green,61,145,64,255",
    "emerald_green,0,201,87,255",
    "forest_green,34,139,34,255",
    "green,0,255,0,255",
    "green_dark,0,100,0,255",
    "green_pale,152,251,152,255",
    "green_yellow,173,255,47,255",
    "lawn_green,124,252,0,255",
    "lime_green,50,205,50,255",
    "mint,189,252,201,255",
    "olive,59,94,43,255",
    "olive_drab,107,142,35,255",
    "olive_green_dark,85,107,47,255",
    "permanent_green,10,201,43,255",
    "sap_green,48,128,20,255",
    "sea_green,46,139,87,255",
    "sea_green_dark,143,188,143,255",
    "sea_green_medium,60,179,113,255",
    "sea_green_light,32,178,170,255",
    "spring_green,0,255,127,255",
    "spring_green_medium,0,250,154,255",
    "terre_verte,56,94,15,255",
    "viridian_light,110,255,112,255",
    "yellow_green,154,205,50,255",
    //  Cyans
    "aquamarine,127,255,212,255",
    "aquamarine_medium,102,205,170,255",
    "cyan,0,255,255,255",
    "cyan_white,224,255,255,255",
    "turquoise,64,224,208,255",
    "turquoise_dark,0,206,209,255",
    "turquoise_medium,72,209,204,255",
    "turquoise_pale,175,238,238,255",
    //  Blues
    "alice_blue,240,248,255,255",
    "blue,0,0,255,255",
    "blue_light,173,216,230,255",
    "blue_medium,0,0,205,255",
    "cadet,95,158,160,255",
    "cobalt,61,89,171,255",
    "cornflower,100,149,237,255",
    "cerulean,5,184,204,255",
    "dodger_blue,30,144,255,255",
    "indigo,8,46,84,255",
    "manganese_blue,3,168,158,255",
    "midnight_blue,25,25,112,255",
    "navy,0,0,128,255",
    "peacock,51,161,201,255",
    "powder_blue,176,224,230,255",
    "royal_blue,65,105,225,255",
    "slate_blue,106,90,205,255",
    "slate_blue_dark,72,61,139,255",
    "slate_blue_light,132,112,255,255",
    "slate_blue_medium,123,104,238,255",
    "sky_blue,135,206,235,255",
    "sky_blue_deep,0,191,255,255",
    "sky_blue_light,135,206,250,255",
    "steel_blue,70,130,180,255",
    "steel_blue_light,176,196,222,255",
    "turquoise_blue,0,199,140,255",
    "ultramarine,18,10,143,255",
    //  Magentas
    "blue_violet,138,43,226,255",
    "cobalt_violet_deep,145,33,158,255",
    "magenta,255,0,255,255",
    "orchid,218,112,214,255",
    "orchid_dark,153,50,204,255",
    "orchid_medium,186,85,211,255",
    "permanent_red_violet,219,38,69,255",
    "plum,221,160,221,255",
    "purple,160,32,240,255",
    "purple_medium,147,112,219,255",
    "ultramarine_violet,92,36,110,255",
    "violet,143,94,153,255",
    "violet_dark,148,0,211,255",
    "violet_red,208,32,144,255",
    "violet_red_medium,199,21,133,255",
    "violet_red_pale,219,112,147,255",
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

    // Here we fill the color map.
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
      this->colorMap[name] = vtkColor4ub(color[0],color[1],color[2],color[3]);
      }
  }

private:
  //-----------------------------------------------------------------------------
  // Convert a string to lowercase.
  vtkStdString ToLowercase(const vtkStdString & str)
  {
    vtkStdString s;
    std::transform(str.begin(),str.end(),std::back_inserter(s),
                   (int (*)(int))std::tolower);
    return s;
  }

  // The names and color values.
  std::map<vtkStdString, vtkColor4ub> colorMap;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkNamedColors);

//----------------------------------------------------------------------------
vtkNamedColors::vtkNamedColors()
{
  this->Colors = new vtkNamedColorsDataStore;
}

//----------------------------------------------------------------------------
vtkNamedColors::~vtkNamedColors()
{
  delete this->Colors;
}

//-----------------------------------------------------------------------------

void vtkNamedColors::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for ( std::map<vtkStdString, vtkColor4ub >::const_iterator p =
    this->Colors->GetColorMap()->begin();
    p != this->Colors->GetColorMap()->end(); ++p )
    {
    os << indent << ": " << p->first << "(";
    if (p->second.GetSize() == 4)
      {
      int r = p->second.GetRed();
      int g = p->second.GetGreen();
      int b = p->second.GetBlue();
      int a = p->second.GetAlpha();
      os << setw(3) << r << "," << setw(3) << g
        << ","<< setw(3) << b <<"," << setw(3) << a;
      }
      os << ")" << endl;
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkNamedColors::GetColorNames()
{
  std::vector<vtkStdString> cnv = this->Colors->GetColorNames();
  vtkStdString colorNames;
  // Get the last element in the vector.
  std::vector<vtkStdString>::iterator lastItr = cnv.end();
  --lastItr;
  // Now just iterate through the map getting the color names.
  for(std::vector<vtkStdString>::const_iterator p =
    cnv.begin(); p != cnv.end(); ++p )
    {
    colorNames += *p;
    if (p != lastItr)
      {
      colorNames += "\n";
      }
    }
  return colorNames;
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColorNames(vtkStringArray * colorNames)
{
  std::vector<vtkStdString> cnv = this->Colors->GetColorNames();
  // Now just iterate through the map getting the color names.
  for(std::vector<vtkStdString>::const_iterator p =
    cnv.begin(); p != cnv.end(); ++p )
    {
    colorNames->InsertNextValue(*p);
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkNamedColors::GetSynonyms()
{
  vtkStdString synonyms;
  std::vector<std::vector<vtkStdString> > syn = this->Colors->GetSynonyms();
  std::vector<std::vector<vtkStdString> >::const_iterator synLast = syn.end();
  --synLast;
  for(std::vector<std::vector<vtkStdString> >::const_iterator p =
    syn.begin(); p != syn.end(); ++p)
    {
    // Get the last element in the vector.
    std::vector<vtkStdString>::const_iterator strLast =
      p->end();
    --strLast;
    for(std::vector<vtkStdString>::const_iterator q =
      p->begin(); q!= p->end(); ++q)
      {
      synonyms += *q;
      if(q != strLast)
        {
        synonyms += "\n";
        }
      }
    if (p != synLast)
      {
      synonyms += "\n\n";
      }
    }
  return synonyms;
}

//-----------------------------------------------------------------------------
int vtkNamedColors::GetNumberOfColors()
{
  return static_cast<int>(this->Colors->GetColorMap()->size());
}

//-----------------------------------------------------------------------------
void vtkNamedColors::ResetColors()
{
  this->Colors->GetColorMap()->clear();
  this->Colors->Init();
}

//-----------------------------------------------------------------------------
bool vtkNamedColors::ColorExists(const vtkStdString & name)
{
  return this->Colors->ColorExists(name);
}

//-----------------------------------------------------------------------------
vtkColor4ub vtkNamedColors::GetColor4ub(const vtkStdString & name)
{
  vtkColor4ub color;
  this->Colors->GetColor(name,color);
  return color;
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name,
                              unsigned char & r, unsigned char & g,
                              unsigned char & b, unsigned char & a)
{
  vtkColor4ub rgba;
  this->Colors->GetColor(name,rgba);
  r = rgba[0];
  g = rgba[1];
  b = rgba[2];
  a = rgba[3];
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, unsigned char rgba[4])
{
  vtkColor4ub color;
  this->Colors->GetColor(name,color);
  for(int i = 0; i < color.GetSize(); ++i)
    {
    rgba[i] = color[i];
    }
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, vtkColor4ub & rgba)
{
  this->Colors->GetColor(name,rgba);
}

//-----------------------------------------------------------------------------
vtkColor4d vtkNamedColors::GetColor4d(const vtkStdString & name)
{
  vtkColor4d color;
  this->Colors->GetColor(name,color);
  return color;
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name,
                              double & r, double & g, double & b, double & a)
{
  vtkColor4d rgba;
  this->Colors->GetColor(name,rgba);
  r = rgba[0];
  g = rgba[1];
  b = rgba[2];
  a = rgba[3];
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, double rgba[4])
{
  vtkColor4d color;
  this->Colors->GetColor(name,color);
  for(int i = 0; i < color.GetSize(); ++i)
    {
    rgba[i] = color[i];
    }
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, vtkColor4d & rgba)
{
  this->Colors->GetColor(name,rgba);
}

//-----------------------------------------------------------------------------
vtkColor3ub vtkNamedColors::GetColor3ub(const vtkStdString & name)
{
  vtkColor3ub rgb;
  this->Colors->GetColor(name,rgb);
  return rgb;
}

//-----------------------------------------------------------------------------
vtkColor3d vtkNamedColors::GetColor3d(const vtkStdString & name)
{
  vtkColor3d rgb;
  this->Colors->GetColor(name,rgb);
  return rgb;
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name,
                                 double & r, double & g, double & b)
{
  vtkColor3d rgb;
  this->Colors->GetColor(name,rgb);
  r = rgb[0];
  g = rgb[1];
  b = rgb[2];
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColorRGB(const vtkStdString & name, double rgb[3])
{
  vtkColor3d color;
  this->Colors->GetColor(name,color);
  for(int i = 0; i < color.GetSize(); ++i)
    {
    rgb[i] = color[i];
    }
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, vtkColor3ub & rgb)
{
  this->Colors->GetColor(name,rgb);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::GetColor(const vtkStdString & name, vtkColor3d & rgb)
{
  this->Colors->GetColor(name,rgb);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const unsigned char & r, const unsigned char & g,
                              const unsigned char & b, const unsigned char & a)
{
  vtkColor4ub v;
  v.SetRed(r);
  v.SetGreen(g);
  v.SetBlue(b);
  v.SetAlpha(a);
  this->Colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const unsigned char rgba[4])
{
  vtkColor4ub v;
  for ( int i = 0; i < v.GetSize(); ++i )
    {
    v[i] = rgba[i];
    }
  this->Colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const vtkColor4ub & rgba)
{
  this->Colors->SetColor(name,rgba);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const vtkColor3ub & rgb)
{
  this->Colors->SetColor(name,rgb);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
                              const double & r, const double & g,
                              const double & b, const double & a)
{
  vtkColor4d v;
  v.SetRed(r);
  v.SetGreen(g);
  v.SetBlue(b);
  v.SetAlpha(a);
  this->Colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name, const double rgba[4])
{
  vtkColor4d v;
  for ( int i = 0; i < v.GetSize(); ++i )
    {
    v[i] = rgba[i];
    }
  this->Colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
  const vtkColor4d & rgba)
{
  vtkColor4d v;
  for ( int i = 0; i < v.GetSize(); ++i )
    {
    v[i] = rgba[i];
    }
  this->Colors->SetColor(name,v);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::SetColor(const vtkStdString & name,
  const vtkColor3d & rgb)
{
  this->Colors->SetColor(name,rgb);
}

//-----------------------------------------------------------------------------
void vtkNamedColors::RemoveColor(const vtkStdString & name)
{
  if (!name.empty())
    {
    this->Colors->RemoveColor(name);
    }
}
