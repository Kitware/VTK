/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMathTextUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMathTextUtilities - Access to MatPlotLib MathText rendering
// .SECTION Description
// vtkMathTextUtilities provides access to the MatPlotLib MathText
// implementation.
// .CAVEATS
// Internal use only.
// EXPERIMENTAL for the moment.

#ifndef __vtkMathTextUtilities_h
#define __vtkMathTypeUtilities_h

#include "vtkMathTextModule.h" // For export macro
#include "vtkObject.h"

struct _object;
typedef struct _object PyObject;
class vtkImageData;
class vtkTextProperty;

//----------------------------------------------------------------------------
// Singleton cleanup

class VTKMATHTEXT_EXPORT vtkMathTextUtilitiesCleanup
{
public:
  vtkMathTextUtilitiesCleanup();
  ~vtkMathTextUtilitiesCleanup();
};

class VTKMATHTEXT_EXPORT vtkMathTextUtilities : public vtkObject
{
public:
  vtkTypeMacro(vtkMathTextUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is a singleton pattern New. There will be only ONE reference
  // to a vtkMathTextUtilities object per process.  Clients that
  // call this method must use Delete() on the object so that reference
  // counting will work. The single instance will be unreferenced when
  // the program exits. You should just use the static GetInstance() method
  // anyway to get the singleton.
  static vtkMathTextUtilities *New();

  // Description:
  // Return the singleton instance with no reference counting.
  static vtkMathTextUtilities* GetInstance();

  // Description:
  // Supply a user defined instance. Call Delete() on the supplied
  // instance after setting it to fix the reference count.
  static void SetInstance(vtkMathTextUtilities *instance);

  // Description:
  // Render the given string @a str into the vtkImageData @a data with a
  // resolution of @a dpi.
  bool RenderString(const char *str,
                    vtkImageData *data,
                    unsigned int dpi);

protected:
  vtkMathTextUtilities();
  virtual ~vtkMathTextUtilities();

  bool CheckForError();
  bool CheckForError(PyObject *object);

  PyObject *Parser;

private:
  vtkMathTextUtilities(const vtkMathTextUtilities&);  // Not implemented.
  void operator=(const vtkMathTextUtilities&);  // Not implemented.

  // Description:
  // The singleton instance and the singleton cleanup instance
  static vtkMathTextUtilities* Instance;
  static vtkMathTextUtilitiesCleanup Cleanup;
};

#endif
