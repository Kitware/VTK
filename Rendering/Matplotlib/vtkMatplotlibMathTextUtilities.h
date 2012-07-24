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

#ifndef __vtkMatplotlibMathTextUtilities_h
#define __vtkMatplotlibMathTextUtilities_h

#include "vtkMatplotlibModule.h" // For export macro
#include "vtkMathTextUtilities.h"

struct _object;
typedef struct _object PyObject;
class vtkImageData;
class vtkPath;
class vtkTextProperty;

class VTKMATPLOTLIB_EXPORT vtkMatplotlibMathTextUtilities :
    public vtkMathTextUtilities
{
public:
  vtkTypeMacro(vtkMatplotlibMathTextUtilities, vtkMathTextUtilities);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkMatplotlibMathTextUtilities *New();

  // Description:
  // Render the given string @a str into the vtkImageData @a data with a
  // resolution of @a dpi.
  bool RenderString(const char *str, vtkImageData *data, vtkTextProperty *tprop,
                    unsigned int dpi);

  // Description:
  // Parse the MathText expression in str and fill path with a contour of the
  // glyphs.
  bool StringToPath(const char *str, vtkPath *path, vtkTextProperty *tprop);

protected:
  vtkMatplotlibMathTextUtilities();
  virtual ~vtkMatplotlibMathTextUtilities();

  bool InitializePython();
  bool InitializeMaskParser();
  bool InitializePathParser();
  bool InitializeFontPropertiesClass();

  bool CheckForError();
  bool CheckForError(PyObject *object);

  // Description:
  // Returns a matplotlib.font_manager.FontProperties PyObject, initialized from
  // the vtkTextProperty tprop.
  PyObject * GetFontProperties(vtkTextProperty *tprop);

  bool PythonIsInitialized;
  PyObject *MaskParser;
  PyObject *PathParser;
  PyObject *FontPropertiesClass;

private:
  vtkMatplotlibMathTextUtilities(const vtkMatplotlibMathTextUtilities&); // Not implemented.
  void operator=(const vtkMatplotlibMathTextUtilities&); // Not implemented.
};

#endif
