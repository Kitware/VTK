/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNamedColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNamedColors - A class holding colors and their names.
// .SECTION Description
// vtkNamedColors is class that holds colors and their associated names.
//
// Color names are case insensitive and are stored as lower-case names
// along with a 4-element array whose elements are red, green, blue and alpha,
// in that order, corresponding to the RGBA value of the color.
//
// It is assumed that if the RGBA values are unsigned char then each element
// lies in the range 0...255 and if the RGBA values are double then each
// element lies in the range 0...1.
//
// The colors and names are those in http://en.wikipedia.org/wiki/Web_colors
// that are derived from the CSS3 specification:
// http://www.w3.org/TR/css3-color/#svg-color
// In this table common synonyms such as cyan/aqua and
// magenta/fuchsia are also included.
//
// Also included in this class are names and colors taken from
// Wrapping/Tcl/vtktesting/colors.tcl and Wrapping/Python/vtk/util/colors.py.
//
// Web colors and names in http://en.wikipedia.org/wiki/Web_colors take
// precedence over those in colors.tcl and colors.py. One consequence of this
// is that while colors.py and colors.tcl specify green as equivalent to
// (0,255,0), the web color standard defines it as (0,128,0).
//
// For a web page of VTK Named Colors and their RGB values, see:
// http://www.vtk.org/Wiki/VTK/Examples/Python/Visualization/VTKNamedColorPatches_html .
//
// The code used to generate this table is available from:
// http://www.vtk.org/Wiki/VTK/Examples/Python/Visualization/NamedColorPatches ,
// this is useful if you wish to generate your own table.
//
// The SetColor methods will overwrite existing colors if the name of the
// color being set matches an existing color. Note that ColorExists() can be
// used to test for existence of the color being set.
//
// In the case of the GetColor methods returning doubles, alternative versions,
// identified by the letters RGB in the names, are provided.
// These get functions return just the red, green and blue components of
// a color.

#ifndef __vtkNamedColors_h
#define __vtkNamedColors_h

#include "vtkCommonColorModule.h" // For export macro
#include "vtkObject.h"
#include "vtkColor.h" // Needed for vtkColor[34]ub
#include "vtkStdString.h" // Needed for arguments
#include "vtkStringArray.h" // For returning color names

class vtkNamedColorsDataStore;

class VTKCOMMONCOLOR_EXPORT vtkNamedColors : public vtkObject
{
public:
  vtkTypeMacro(vtkNamedColors, vtkObject);

  // Description:
  // Methods invoked by print to print information about the object
  // including superclasses. Typically not called by the user
  // (use Print() instead) but used in the hierarchical print
  // process to combine the output of several classes.
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Create a new vtkNamedColors object.
  static vtkNamedColors* New();

  // Description:
  // Get the number of colors.
  int GetNumberOfColors();

  // Description:
  // Reset the colors in the color map to the original colors.
  // Any colors inserted by the user will be lost.
  void ResetColors();

  // Description:
  // Return true if the color exists.
  bool ColorExists(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor4ub class.
  // The color black is returned if the color is not found.
  vtkColor4ub GetColor4ub(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as four unsigned char variables:
  // red, green, blue, alpha. The range of each element is 0...255.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name,
                unsigned char & r, unsigned char & g,
                unsigned char & b, unsigned char & a);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as an unsigned char array:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, unsigned char rgba[4]);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor4ub class.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor4ub & rgba);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor4d class.
  // The color black is returned if the color is not found.
  vtkColor4d GetColor4d(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as four double variables:
  // red, green, blue, alpha. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name,
                double & r, double & g, double & b, double & a);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a double array:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, double rgba[4]);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor4d class.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor4d & rgba);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor3ub class.
  // The color black is returned if the color is not found.
  vtkColor3ub GetColor3ub(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor3d class.
  // The color black is returned if the color is not found.
  vtkColor3d GetColor3d(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as three double variables:
  // red, green, blue. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name,
                   double & r, double & g, double & b);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a double array:
  // [red, green, blue]. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColorRGB(const vtkStdString & name, double rgb[3]);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor3ub class.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor3ub & rgb);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as a vtkColor3d class.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, vtkColor3d & rgb);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The range of each color is 0...255.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name,
                const unsigned char & r, const unsigned char & g,
                const unsigned char & b, const unsigned char & a = 255);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The range of each color is 0...1.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name,
                const double & r, const double & g,
                const double & b, const double & a = 1);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is an unsigned char array:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // The user must ensure that the color array size is 4.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name, const unsigned char rgba[4]);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor4ub class.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name, const vtkColor4ub & rgba);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor3ub class.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name, const vtkColor3ub & rgb);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a double array:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name, const double rgba[4]);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor4d class.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name, const vtkColor4d & rgba);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a vtkColor3d class.
  // No color is set if the name is empty.
  virtual void SetColor(const vtkStdString & name, const vtkColor3d & rgb);

  // Description:
  // Remove the color by name.
  // The name is treated as being case-insensitive.
  // Examples for parsing are provided in:
  // TestNamedColors.cxx and TestNamedColorsIntegration.py
  void RemoveColor(const vtkStdString & name);

  // Description:
  // Return a string of color names with each name
  // delimited by a line feed.
  // This is easily parsed by the user into whatever
  // data structure they require.
  // Examples for parsing are provided in:
  // TestNamedColors.cxx and TestNamedColorsIntegration.py
  vtkStdString GetColorNames();

  // Description:
  // Return a string array of color names.
  void GetColorNames(vtkStringArray * colorNames);

  //  Description:
  // Return a string of synonyms such as
  // cyan/aqua and magenta/fuchsia.
  // The string is formatted such that a single line feed delimits
  // each color in the synonym and a double line feed delimits each
  // synonym.
  // Warning this could take a long time for very large color maps.
  // This is easily parsed by the user into whatever
  // data structure they require.
  vtkStdString GetSynonyms();

protected:
  vtkNamedColors();
  virtual ~vtkNamedColors();

private:
  // Description:
  // The implementation of the color map and other required methods.
  vtkNamedColorsDataStore *Colors;

  vtkNamedColors(const vtkNamedColors&);  // Not implemented.
  void operator=(const vtkNamedColors&);  // Not implemented.
};

#endif /* __vtkNamedColors_h */
