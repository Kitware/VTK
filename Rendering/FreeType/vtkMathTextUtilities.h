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
// .NAME vtkMathTextUtilities - Abstract interface to equation rendering.
// .SECTION Description
// vtkMathTextUtilities defines an interface for equation rendering. Intended
// for use with the python matplotlib.mathtext module (implemented in the
// vtkMatplotlib module).

#ifndef vtkMathTextUtilities_h
#define vtkMathTextUtilities_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkObject.h"
#include "vtkTextRenderer.h" // for metrics

class vtkImageData;
class vtkPath;
class vtkTextProperty;
class vtkTextActor;
class vtkViewport;

//----------------------------------------------------------------------------
// Singleton cleanup

class VTKRENDERINGFREETYPE_EXPORT vtkMathTextUtilitiesCleanup
{
public:
  vtkMathTextUtilitiesCleanup();
  ~vtkMathTextUtilitiesCleanup();

private:
  vtkMathTextUtilitiesCleanup(const vtkMathTextUtilitiesCleanup& other); // no copy constructor
  vtkMathTextUtilitiesCleanup& operator=(const vtkMathTextUtilitiesCleanup& rhs); // no copy assignment
};

class VTKRENDERINGFREETYPE_EXPORT vtkMathTextUtilities : public vtkObject
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
  // Determine the dimensions of the image that RenderString will produce for
  // a given str, tprop, and dpi
  virtual bool GetBoundingBox(vtkTextProperty *tprop, const char *str,
                              unsigned int dpi, int bbox[4]) = 0;

  // Description:
  // Return the metrics for the rendered str, tprop, and dpi.
  virtual bool GetMetrics(vtkTextProperty *tprop, const char *str,
                          unsigned int dpi,
                          vtkTextRenderer::Metrics &metrics) = 0;

  // Description:
  // Render the given string @a str into the vtkImageData @a data with a
  // resolution of @a dpi. textDims, will be overwritten by the pixel width and
  // height of the rendered string. This is useful when ScaleToPowerOfTwo is
  // set to true, and the image dimensions may not match the dimensions of the
  // rendered text.
 virtual bool RenderString(const char *str, vtkImageData *data,
                           vtkTextProperty *tprop,
                           unsigned int dpi, int textDims[2] = NULL) = 0;

  // Description:
  // Parse the MathText expression in str and fill path with a contour of the
  // glyphs.
  virtual bool StringToPath(const char *str, vtkPath *path,
                            vtkTextProperty *tprop) = 0;

  // Description:
  // This function returns the font size (in points) required to fit the string
  // in the target rectangle. The font size of tprop is updated to the computed
  // value as well. If an error occurs (e.g. an improperly formatted MathText
  // string), -1 is returned.
  virtual int GetConstrainedFontSize(const char *str,
                                     vtkTextProperty *tprop,
                                     int targetWidth, int targetHeight,
                                     unsigned int dpi);

  // Description:
  // Set to true if the graphics implmentation requires texture image dimensions
  // to be a power of two. Default is true, but this member will be set
  // appropriately when GL is inited.
  virtual bool GetScaleToPowerOfTwo() = 0;
  virtual void SetScaleToPowerOfTwo(bool scale) = 0;

protected:
  vtkMathTextUtilities();
  virtual ~vtkMathTextUtilities();

private:
  vtkMathTextUtilities(const vtkMathTextUtilities&);  // Not implemented.
  void operator=(const vtkMathTextUtilities&);  // Not implemented.

  // Description:
  // The singleton instance and the singleton cleanup instance
  static vtkMathTextUtilities* Instance;
  static vtkMathTextUtilitiesCleanup Cleanup;
};

#endif
