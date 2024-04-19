// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKRENDERINGCONTEXT2D_EXPORT vtkMarkerUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkMarkerUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Enum containing various marker styles that can be used in a plot.
   */
  enum
  {
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
  static void GenerateMarker(vtkImageData* data, int style, int width);

protected:
  vtkMarkerUtilities();
  ~vtkMarkerUtilities() override;

private:
  vtkMarkerUtilities(const vtkMarkerUtilities&) = delete;
  void operator=(const vtkMarkerUtilities&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkMarkerUtilities_h
