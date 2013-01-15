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
// .NAME vtkMatplotlibMathTextUtilities - Access to MatPlotLib MathText rendering
// .SECTION Description
// vtkMatplotlibMathTextUtilities provides access to the MatPlotLib MathText
// implementation.
//
// This class is aware of a number of enviroment variables that can be used to
// configure and debug python initialization (all are optional):
// - VTK_MATPLOTLIB_DEBUG: Enable verbose debugging output during initialization
// of the python environment.
// - VTK_MATPLOTLIB_PYTHONINTERP: Path to the python interpreter. This will be
// passed to Py_SetProgramName prior to calling Py_Initialize.
// - VTK_MATPLOTLIB_PYTHONHOME: See the Python documentation on the PYTHONHOME
// environment variable. This will be passed to Py_SetPythonHome prior to
// calling Py_Initialize.
// - VTK_MATPLOTLIB_PYTHONPATH: A list of additional python module paths to be
// prepended to the sys.path object after initialization. Use ';' on windows and
// ':' on apple/linux to separate multiple paths.

#ifndef __vtkMatplotlibMathTextUtilities_h
#define __vtkMatplotlibMathTextUtilities_h

#include "vtkRenderingMatplotlibModule.h" // For export macro
#include "vtkMathTextUtilities.h"

struct _object;
typedef struct _object PyObject;
class vtkImageData;
class vtkPath;
class vtkTextProperty;

class VTKRENDERINGMATPLOTLIB_EXPORT vtkMatplotlibMathTextUtilities :
    public vtkMathTextUtilities
{
public:
  vtkTypeMacro(vtkMatplotlibMathTextUtilities, vtkMathTextUtilities);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkMatplotlibMathTextUtilities *New();

  // Description:
  // Determine the dimensions of the image that RenderString will produce for
  // a given str, tprop, and dpi
  bool GetBoundingBox(vtkTextProperty *tprop, const char *str,
                      unsigned int dpi, int bbox[4]);

  // Description:
  // Render the given string @a str into the vtkImageData @a data with a
  // resolution of @a dpi. The image is resized automatically. textDims
  // will be overwritten by the pixel width and height of the rendered string.
  // This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
  // not match the dimensions of the rendered text.
  bool RenderString(const char *str, vtkImageData *data, vtkTextProperty *tprop,
                    unsigned int dpi, int textDims[2] = NULL);

  // Description:
  // Parse the MathText expression in str and fill path with a contour of the
  // glyphs.
  bool StringToPath(const char *str, vtkPath *path, vtkTextProperty *tprop);

  // Description:
  // Set to true if the graphics implmentation requires texture image dimensions
  // to be a power of two. Default is true, but this member will be set
  // appropriately when GL is inited.
  vtkSetMacro(ScaleToPowerOfTwo, bool);
  vtkGetMacro(ScaleToPowerOfTwo, bool);

protected:
  vtkMatplotlibMathTextUtilities();
  virtual ~vtkMatplotlibMathTextUtilities();

  bool InitializeMaskParser();
  bool InitializePathParser();
  bool InitializeFontPropertiesClass();

  bool CheckForError();
  bool CheckForError(PyObject *object);

  // Description:
  // Returns a matplotlib.font_manager.FontProperties PyObject, initialized from
  // the vtkTextProperty tprop.
  PyObject * GetFontProperties(vtkTextProperty *tprop);

  PyObject *MaskParser;
  PyObject *PathParser;
  PyObject *FontPropertiesClass;

  // Rotate the 4 2D corner points by the specified angle (degrees) around the
  // origin and calculate the bounding box
  void RotateCorners(double angleDeg, double corners[4][2], double bbox[4]);

  // Description:
  // Used for runtime checking of matplotlib's mathtext availability.
  enum Availablity
    {
    NOT_TESTED = 0,
    AVAILABLE,
    UNAVAILABLE
    };
  static Availablity MPLMathTextAvailable;

  bool ScaleToPowerOfTwo;
  bool PrepareImageData(vtkImageData *data, int bbox[4]);

  // Description:
  // Set to true if this class initialized the python interpreter. This
  // is used to determine if Py_Finalize() should be called when destructed.
  static bool InitializedPython;

private:
  vtkMatplotlibMathTextUtilities(const vtkMatplotlibMathTextUtilities&); // Not implemented.
  void operator=(const vtkMatplotlibMathTextUtilities&); // Not implemented.
};

#endif
