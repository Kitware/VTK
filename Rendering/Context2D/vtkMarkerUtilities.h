/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkerUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkMarkerUtilities
 * @brief   Utilities for generating marker images
 *
 *
 * This class programmatically generates markers of a specified size
 * for various marker styles.
 *
 * @sa
 * vtkPlotLine, vtkPlotPoints
*/

#ifndef vtkMarkerUtilities_h
#define vtkMarkerUtilities_h

#include "vtkRenderingContext2DModule.h" // For export macro

#include "vtkObject.h"

class vtkImageData;

class VTKRENDERINGCONTEXT2D_EXPORT vtkMarkerUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkMarkerUtilities, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Enum containing various marker styles that can be used in a plot.
   */
  enum {
    NONE = 0,
    CROSS,
    PLUS,
    SQUARE,
    CIRCLE,
    DIAMOND
  };

  /**
   * Generate the requested symbol of a particular style and size.
   */
  static void GenerateMarker(vtkImageData *data, int style, int width);

protected:
  vtkMarkerUtilities();
  ~vtkMarkerUtilities() override;

private:
  vtkMarkerUtilities(const vtkMarkerUtilities &) = delete;
  void operator=(const vtkMarkerUtilities &) = delete;

};

#endif //vtkMarkerUtilities_h
