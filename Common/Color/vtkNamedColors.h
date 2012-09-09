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
// The colors and names are those in
// http://en.wikipedia.org/wiki/Web_colors
// that are derived from the CSS3 specification:
// http://www.w3.org/TR/css3-color/#svg-color
// In this table common synonyms such as cyan/aqua and
// magenta/fuchsia are also included.
//
// The color names are case insensitive and the colors are treated as
// a 4-element array whose elements are red, green, blue and alpha in that
// order.
//
// If the array is an usnsigned char then each element
// lies in the range 0...255.
// If the array is double then each element lies in the range 0...1.
//
// In the case of the get methods returning doubles, alternative versions,
// identified by the letters RGB in the names, are provided.
// These get functions return just the red, green and blue components of
// a color.

#ifndef __vtkNamedColors_h
#define __vtkNamedColors_h

#include "vtkCommonColorModule.h" // For export macro
#include "vtkObject.h"
#include <vtkStdString.h>
#include <vector> // STL Header for returning/storing color values

class vtkNamedColorsDataStore;

class VTKCOMMONCOLOR_EXPORT vtkNamedColors : public vtkObject
{
public:
  vtkTypeMacro(vtkNamedColors, vtkObject);
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
  // The color is returned as as an unsigned char array:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // The color black is returned if the color is not found.
  unsigned char * GetColorAsUnsignedChar(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as four unsigned char variables:
  // red, green, blue, alpha. The range of each element is 0...255.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name,
                unsigned char & r, unsigned char & g,
                unsigned char & b, unsigned char & a);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as an unsigned char array:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, unsigned char rgba[4]);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as a double array:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  double * GetColorAsDouble(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as four double variables:
  // red, green, blue, alpha. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name,
                double & r, double & g, double & b, double & a);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as a double array:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColor(const vtkStdString & name, double rgba[4]);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as a double array:
  // [red, green, blue]. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  double * GetColorAsDoubleRGB(const vtkStdString & name);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as three double variables:
  // red, green, blue. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColorRGB(const vtkStdString & name,
                   double & r, double & g, double & b);

  // Description:
  // Get the color by name.
  // The name is treated as being case-insensitive.
  // The color is returned as as a double array:
  // [red, green, blue]. The range of each element is 0...1.
  // The color black is returned if the color is not found.
  void GetColorRGB(const vtkStdString & name, double rgb[3]);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The range of each color is 0...255.
  // No color is set if the name is empty.
  void SetColor(const vtkStdString & name,
                const unsigned char & r, const unsigned char & g,
                const unsigned char & b, const unsigned char & a = 255);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is an unsigned char array:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // The user must ensure that the color array size is 4.
  // No color is set if the name is empty.
  void SetColor(const vtkStdString & name, const unsigned char rgba[4]);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The range of each color is 0...1.
  void SetColor(const vtkStdString & name,
                const double & r, const double & g,
                const double & b, const double & a = 1);

  // Description:
  // Set the color by name.
  // The name is treated as being case-insensitive.
  // The color is a double array:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // No color is set if the name is empty.
  void SetColor(const vtkStdString & name, const double rgba[4]);

  // Description:
  // Remove the color by name.
  // The name is treated as being case-insensitive.
  void RemoveColor(const vtkStdString & name);

  // Description:
  // Return a vector of color names.
  std::vector<vtkStdString> GetColorNames();

  //  Description:
  // Return a vector where each element of the vector is a vector of
  // synonyms such as cyan/aqua and magenta/fuchsia
  // Warning this could take a long time for very large color maps.
  std::vector<std::vector<vtkStdString> > GetSynonyms();

protected:
  vtkNamedColors();
  virtual ~vtkNamedColors();

private:
  // Description:
  // The implementation of the color map and other required methods.
  vtkNamedColorsDataStore *colors;

  vtkNamedColors(const vtkNamedColors&);  // Not implemented.
  void operator=(const vtkNamedColors&);  // Not implemented.
};

#endif /* __vtkNamedColors_h */
