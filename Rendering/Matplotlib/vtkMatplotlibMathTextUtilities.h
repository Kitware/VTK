/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatplotlibMathTextUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMatplotlibMathTextUtilities
 * @brief   Access to MatPlotLib MathText rendering
 *
 * vtkMatplotlibMathTextUtilities provides access to the MatPlotLib MathText
 * implementation.
 *
 * This class is aware of a number of environment variables that can be used to
 * configure and debug python initialization (all are optional):
 * - VTK_MATPLOTLIB_DEBUG: Enable verbose debugging output during initialization
 * of the python environment.
*/

#ifndef vtkMatplotlibMathTextUtilities_h
#define vtkMatplotlibMathTextUtilities_h

#include "vtkRenderingMatplotlibModule.h" // For export macro
#include "vtkMathTextUtilities.h"

struct _object;
typedef struct _object PyObject;
class vtkImageData;
class vtkPath;
class vtkPythonInterpreter;
class vtkTextProperty;

class VTKRENDERINGMATPLOTLIB_EXPORT vtkMatplotlibMathTextUtilities :
    public vtkMathTextUtilities
{
public:
  vtkTypeMacro(vtkMatplotlibMathTextUtilities, vtkMathTextUtilities);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkMatplotlibMathTextUtilities *New();

  virtual bool IsAvailable() VTK_OVERRIDE;

  /**
   * Given a text property and a string, get the bounding box {xmin, xmax,
   * ymin, ymax} of the rendered string in pixels. The origin of the bounding
   * box is the anchor point described by the horizontal and vertical
   * justification text property variables.
   * Returns true on success, false otherwise.
   */
  bool GetBoundingBox(vtkTextProperty *tprop, const char *str, int dpi,
                      int bbox[4]) VTK_OVERRIDE;

  bool GetMetrics(vtkTextProperty *tprop, const char *str, int dpi,
                  vtkTextRenderer::Metrics &metrics) VTK_OVERRIDE;

  /**
   * Render the given string @a str into the vtkImageData @a data with a
   * resolution of @a dpi. The image is resized automatically. textDims
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   * The origin of the image's extents is aligned with the anchor point
   * described by the text property's vertical and horizontal justification
   * options.
   */
  bool RenderString(const char *str, vtkImageData *data, vtkTextProperty *tprop,
                    int dpi, int textDims[2] = NULL) VTK_OVERRIDE;

  /**
   * Parse the MathText expression in str and fill path with a contour of the
   * glyphs. The origin of the path coordinates is aligned with the anchor point
   * described by the text property's horizontal and vertical justification
   * options.
   */
  bool StringToPath(const char *str, vtkPath *path, vtkTextProperty *tprop,
                    int dpi) VTK_OVERRIDE;

  //@{
  /**
   * Set to true if the graphics implmentation requires texture image dimensions
   * to be a power of two. Default is true, but this member will be set
   * appropriately when GL is inited.
   */
  void SetScaleToPowerOfTwo(bool val) VTK_OVERRIDE;
  bool GetScaleToPowerOfTwo() VTK_OVERRIDE;
  //@}

protected:
  vtkMatplotlibMathTextUtilities();
  virtual ~vtkMatplotlibMathTextUtilities();

  bool InitializeMaskParser();
  bool InitializePathParser();
  bool InitializeFontPropertiesClass();

  bool CheckForError();
  bool CheckForError(PyObject *object);

  /**
   * Returns a matplotlib.font_manager.FontProperties PyObject, initialized from
   * the vtkTextProperty tprop.
   */
  PyObject * GetFontProperties(vtkTextProperty *tprop);

  /**
   * Cleanup and destroy any python objects. This is called during destructor as
   * well as when the Python interpreter is finalized. Thus this class must
   * handle the case where the internal python objects disappear between calls.
   */
  void CleanupPythonObjects();

  vtkPythonInterpreter* Interpreter;
  PyObject *MaskParser;
  PyObject *PathParser;
  PyObject *FontPropertiesClass;

  static void GetJustifiedBBox(int rows, int cols, vtkTextProperty *tprop,
                               int bbox[4]);

  // Rotate the 4 2D corner points by the specified angle (degrees) around the
  // origin and calculate the bounding box
  static void RotateCorners(double angleDeg, double corners[4][2],
                            double bbox[4]);

  bool ScaleToPowerOfTwo;
  bool PrepareImageData(vtkImageData *data, int bbox[4]);

private:
  vtkMatplotlibMathTextUtilities(const vtkMatplotlibMathTextUtilities&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMatplotlibMathTextUtilities&) VTK_DELETE_FUNCTION;

  /**
   * Used for runtime checking of matplotlib's mathtext availability.
   * @sa IsAvailable
   */
  enum Availability
  {
    NOT_TESTED = 0,
    AVAILABLE,
    UNAVAILABLE
  };

  /**
   * Function used to check MPL availability and update MPLMathTextAvailable.
   * This will do tests only the first time this method is called. This method
   * is called internally when matplotlib rendering is first needed and is used
   * to implement IsAvailable.
   */
  static Availability CheckMPLAvailability();

  //@{
  /**
   * Cache the availability of matplotlib in the current python session.
   */
  static Availability MPLMathTextAvailable;
};
  //@}

#endif
