/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorString.h

  Copyright (c) Marco Cecchetti
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkColorString - Helper class for defining a color through a string
// representation.
// .SECTION Description
// vtkColorString is an helper class for defining a color through one of
// the following format:
// - #RRGGBB               (6-digit hexadecimal number)
// - #RRGGBBAA             (8-digit hexadecimal number)
// - rgb(r, g, b)          (where r, g, b are in 0..255)
// - rgba(r, g, b, a)      (where r, g, b, a are in 0..255)
// - a CSS3 color name     (e.g. "steelblue")


#ifndef __vtkColorString_h
#define __vtkColorString_h

#include "vtkCommonColorModule.h" // For export macro
#include "vtkObject.h"
#include "vtkColor.h" // Needed for vtkColor[34]ub
#include "vtkStdString.h" // Needed for arguments




class ColorStringParser;

class VTKCOMMONCOLOR_EXPORT vtkColorString : public vtkObject
{
public:
  static vtkColorString *New();
  vtkTypeMacro(vtkColorString, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set a color by a string in any of the following format:
  // - #RRGGBB
  // - #RRGGBBAA
  // - rgb(r, g, b)
  // - rgba(r, g, b, a)
  // - a CSS3 color name, e.g. "steelblue"
  // If the string argument defines a color using one of the above formats
  // returns true else returns false and the color is set to `rgba(0, 0, 0, 0)`.
  bool SetColor(const char* color);
  bool SetColor(const vtkStdString& color);

  // Description:
  // Return the last set color.
  // The color is returned as an instance of `vtkColor4ub`.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns `rgba(0, 0, 0, 0)`.
  void GetColor(vtkColor4ub& color) const;
  vtkColor4ub GetColor4ub() const;

  // Description:
  // Return the last set color.
  // The color is returned as an instance of `vtkColor4d`.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns `rgba(0, 0, 0, 0)`.
  void GetColor(vtkColor4d& color) const;
  vtkColor4d GetColor4d() const;

  // Description:
  // Return the last set color.
  // The color is returned as an instance of `vtkColor3ub`.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns the `black` color.
  void GetColor(vtkColor3ub& color) const;
  vtkColor3ub GetColor3ub() const;

  // Description:
  // Return the last set color.
  // The color is returned as an instance of `vtkColor3d`.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns the `black` color.
  void GetColor(vtkColor3d& color) const;
  vtkColor3d GetColor3d() const;

  // Description:
  // Return the last set color.
  // The color is returned as a 4 element unsigned char array:
  // [red, green, blue, alpha]. The range of each element is 0...255.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns [0, 0, 0, 0].
  void GetColorRGBA(unsigned char color[4]) const;

  // Description:
  // Return the last set color.
  // The color is returned as a 4 element double array:
  // [red, green, blue, alpha]. The range of each element is 0...1.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns [0, 0, 0, 0].
  void GetColorRGBA(double color[4]) const;

  // Description:
  // Return the last set color.
  // The color is returned as a 3 element unsigned char array:
  // [red, green, blue]. The range of each element is 0...255.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns the `black` color.
  void GetColorRGB(unsigned char color[3]) const;

  // Description:
  // Return the last set color.
  // The color is returned as a 3 element double array:
  // [red, green, blue]. The range of each element is 0...1.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns the `black` color.
  void GetColorRGB(double color[3]) const;

  // Description:
  // Return the last set color.
  // The color is returned as four unsigned char variables:
  // red, green, blue, alpha. The range of each element is 0...255.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed all arguments are set to 0.
  void GetColor(unsigned char& r, unsigned char& g,
                unsigned char& b, unsigned char& a) const;

  // Description:
  // Return the last set color.
  // The color is returned as four double variables:
  // red, green, blue, alpha. The range of each element is 0...1.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed all arguments are set to 0.
  void GetColor(double& r, double& g, double& b, double& a) const;

  // Description:
  // Return the last set color.
  // The color is returned as three unsigned char variables:
  // red, green, blue. The range of each element is 0...255.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns the `black` color.
  void GetColor(unsigned char& r, unsigned char& g, unsigned char& b) const;

  // Description:
  // Return the last set color.
  // The color is returned as three double variables:
  // red, green, blue. The range of each element is 0...1.
  // If `SetColor` has never been invoked it returns the `black` color.
  // If the last `SetColor` invocation has failed it returns the `black` color.
  void GetColor(double& r, double& g, double& b) const;


protected:
  vtkColorString();
  ~vtkColorString();

  vtkColor4ub Color;
  ColorStringParser* Parser;

private:
  vtkColorString(const vtkColorString& );   // Not implemented.
  void operator=(const vtkColorString& );   // Not implemented.
};


#endif
