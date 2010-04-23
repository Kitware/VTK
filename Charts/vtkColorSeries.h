/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorSeries.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkColorSeries - stores a list of colors.
//
// .SECTION Description
// The vtkColorSeries stores a list of colors. There are several schemes
// available and functions to control several aspects of what colors are
// returned. In essence a color scheme is set and colors are returned based on
// the index requested.

#ifndef __vtkColorSeries_h
#define __vtkColorSeries_h

#include "vtkObject.h"
#include "vtkColor.h" // Needed for vtkColor3ub

class VTK_CHARTS_EXPORT vtkColorSeries : public vtkObject
{
public:
  vtkTypeMacro(vtkColorSeries, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Create a new vtkColorSeries with the SPECTRUM color scheme.
  static vtkColorSeries* New();

//BTX
  // Description:
  // Enum of the available color schemes
  enum {
    SPECTRUM = 0, ///< 7 different hues.
    WARM,         ///< 6 warm colors (red to yellow).
    COOL,         ///< 7 cool colors (green to purple).
    BLUES,        ///< 7 different blues.
    WILD_FLOWER,  ///< 7 colors from blue to magenta.
    CITRUS,       ///< 6 colors from green to orange.
    CUSTOM        ///< User specified color scheme.
    };
//ETX

  // Description:
  // Set the color scheme that should be used from those in the enum.
  void SetColorScheme(int scheme);

  // Description:
  // Get the color scheme that is currently being used.
  vtkGetMacro(ColorScheme, int);

  // Description:
  // Get the number of colors available in the current color scheme.
  int GetNumberOfColors();

//BTX
  // Description:
  // Get the color at the specified index. If the index is out of range then
  // black will be returned.
  vtkColor3ub GetColor(int index) const;

  // Description:
  // Get the color at the specified index. If the index is out of range then
  // the call wraps around, i.e. uses the mod operator.
  vtkColor3ub GetColorRepeating(int index) const;

  // Description:
  // Set the color at the specified index. Does nothing if the index is out of
  // range.
  void SetColor(int index, const vtkColor3ub &color);

  // Description:
  // Adds the color to the end of the list.
  void AddColor(const vtkColor3ub &color);

  // Description:
  // Inserts the color at the specified index in the list.
  void InsertColor(int index, const vtkColor3ub &color);
//ETX

  // Description:
  // Removes the color at the specified index in the list.
  void RemoveColor(int index);

  // Description:
  // Clears the list of colors.
  void ClearColors();

  // Description:
  // Make a deep copy of the supplied object.
  void DeepCopy(vtkColorSeries *chartColors);

//BTX
protected:
  vtkColorSeries();
  ~vtkColorSeries();

  // Description:
  // Private data pointer of the class, stores the color list.
  class Private;
  Private *Storage;

  // Description:
  // The color scheme being used.
  int ColorScheme;

private:
  vtkColorSeries(const vtkColorSeries &); // Not implemented.
  void operator=(const vtkColorSeries &);   // Not implemented.
//ETX
};

#endif //__vtkColorSeries_h
