/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTextRenderer - Interface for generating images and path data from
// string data, using multiple backends.
//
// .SECTION Description
// vtkTextRenderer produces images, bounding boxes, and vtkPath
// objects that represent text. The advantage of using this class is to easily
// integrate mathematical expressions into renderings by automatically switching
// between FreeType and MathText backends. If the input string contains at least
// two "$" symbols separated by text, the MathText backend will be used. If
// the string does not meet this criteria, or if no MathText implementation is
// available, the faster FreeType rendering facilities are used. Literal $
// symbols can be used by escaping them with backslashes, "\$" (or "\\$" if the
// string is set programatically).
//
// For example, "Acceleration ($\\frac{m}{s^2}$)" will use MathText, but
// "\\$500, \\$100" will use FreeType.
//
// By default, the backend is set to Detect, which determines the backend based
// on the contents of the string. This can be changed by setting the
// DefaultBackend ivar.
//
// Note that this class is abstract -- link to the vtkRenderingFreetype module
// to get the default implementation.

#ifndef __vtkTextRenderer_h
#define __vtkTextRenderer_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkImageData;
class vtkPath;
class vtkStdString;
class vtkUnicodeString;
class vtkTextProperty;

namespace vtksys {
class RegularExpression;
}

class VTKRENDERINGCORE_EXPORT vtkTextRendererCleanup
{
public:
  vtkTextRendererCleanup();
  ~vtkTextRendererCleanup();
};

class VTKRENDERINGCORE_EXPORT vtkTextRenderer: public vtkObject
{
public:
  vtkTypeMacro(vtkTextRenderer, vtkObject)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // This is a singleton pattern New. There will be only ONE reference
  // to a vtkTextRenderer subclass object per process. Clients that
  // call this method must use Delete() on the object so that reference
  // counting will work. The single instance will be unreferenced when
  // the program exits. You should just use the static GetInstance() method
  // anyway to get the singleton. This method may return NULL if the object
  // factory cannot find an override.
  static vtkTextRenderer *New();

  // Description:
  // Return the singleton instance with no reference counting. May return NULL
  // if the object factory cannot find an override.
  static vtkTextRenderer* GetInstance();

  // Description:
  // Available backends. FreeType and MathText are provided in the default
  // implementation of this interface. Enum values less than 16 are reserved.
  // Custom overrides should define other backends starting at 16.
  enum Backend
    {
    Default = -1,
    Detect = 0,
    FreeType,
    MathText,

    UserBackend = 16
    };

  // Description:
  // The backend to use when none is specified. Default: Detect
  vtkSetMacro(DefaultBackend, int)
  vtkGetMacro(DefaultBackend, int)

  // Description:
  // Determine the appropriate back end needed to render the given string.
  virtual int DetectBackend(const vtkStdString &str);
  virtual int DetectBackend(const vtkUnicodeString &str);

  // Description:
  // Test for availability of various backends
  bool FreeTypeIsSupported() { return HasFreeType; }
  bool MathTextIsSupported() { return HasMathText; }

  // Description:
  // Given a text property and a string, get the bounding box [xmin, xmax] x
  // [ymin, ymax]. Note that this is the bounding box of the area
  // where actual pixels will be written, given a text/pen/baseline location
  // of (0,0).
  // For example, if the string starts with a 'space', or depending on the
  // orientation, you can end up with a [-20, -10] x [5, 10] bbox (the math
  // to get the real bbox is straightforward).
  // Some rendering backends need the DPI of the target. If it is not provided,
  // a DPI of 120 is assumed.
  // Return true on success, false otherwise.
  bool GetBoundingBox(vtkTextProperty *tprop, const vtkStdString &str,
                      int bbox[4], int dpi = 120, int backend = Default)
  {
    return this->GetBoundingBoxInternal(tprop, str, bbox, dpi, backend);
  }
  bool GetBoundingBox(vtkTextProperty *tprop, const vtkUnicodeString &str,
                      int bbox[4], int dpi = 120, int backend = Default)
  {
    return this->GetBoundingBoxInternal(tprop, str, bbox, dpi, backend);
  }

  // Description:
  // Given a text property and a string, this function initializes the
  // vtkImageData *data and renders it in a vtkImageData.
  // Return true on success, false otherwise. If using the overload that
  // specifies "textDims", the array will be overwritten with the pixel width
  // and height defining a tight bounding box around the text in the image,
  // starting from the upper-right corner. This is used when rendering for a
  // texture on graphics hardware that requires texture image dimensions to be
  // a power of two; textDims can be used to determine the texture coordinates
  // needed to cleanly fit the text on the target.
  // Some rendering backends need the DPI of the target. If it is not provided,
  // a DPI of 120 is assumed.
  bool RenderString(vtkTextProperty *tprop, const vtkStdString &str,
                    vtkImageData *data, int textDims[2] = NULL, int dpi = 120,
                    int backend = Default)
  {
    return this->RenderStringInternal(tprop, str, data, textDims, dpi, backend);
  }
  bool RenderString(vtkTextProperty *tprop, const vtkUnicodeString &str,
                    vtkImageData *data, int textDims[2] = NULL, int dpi = 120,
                    int backend = Default)
  {
    return this->RenderStringInternal(tprop, str, data, textDims, dpi, backend);
  }

  // Description:
  // This function returns the font size (in points) and sets the size in @a
  // tprop that is required to fit the string in the target rectangle. The
  // computed font size will be set in @a tprop as well. If an error occurs,
  // this function will return -1.
  // Some rendering backends need the DPI of the target. If it is not provided,
  // a DPI of 120 is assumed.
  int GetConstrainedFontSize(const vtkStdString &str, vtkTextProperty *tprop,
                             int targetWidth, int targetHeight, int dpi = 120,
                             int backend = Default)
  {
    return this->GetConstrainedFontSizeInternal(str, tprop, targetWidth,
                                                targetHeight, dpi, backend);
  }
  int GetConstrainedFontSize(const vtkUnicodeString &str, vtkTextProperty *tprop,
                             int targetWidth, int targetHeight, int dpi = 120,
                             int backend = Default)
  {
    return this->GetConstrainedFontSizeInternal(str, tprop, targetWidth,
                                                targetHeight, dpi, backend);
  }

  // Description:
  // Given a text property and a string, this function populates the vtkPath
  // path with the outline of the rendered string.
  // Return true on success, false otherwise.
  bool StringToPath(vtkTextProperty *tprop, const vtkStdString &str,
                    vtkPath *path, int backend = Default)
  {
    return this->StringToPathInternal(tprop, str, path, backend);
  }
  bool StringToPath(vtkTextProperty *tprop, const vtkUnicodeString &str,
                    vtkPath *path, int backend = Default)
  {
    return this->StringToPathInternal(tprop, str, path, backend);
  }

  // Description:
  // Set to true if the graphics implmentation requires texture image dimensions
  // to be a power of two. Default is true, but this member will be set
  // appropriately by vtkOpenGLRenderWindow::OpenGLInitContext when GL is
  // inited.
  void SetScaleToPowerOfTwo(bool scale)
  {
    this->SetScaleToPowerOfTwoInternal(scale);
  }

  friend class vtkTextRendererCleanup;

protected:
  vtkTextRenderer();
  ~vtkTextRenderer();

  // Description:
  // Virtual methods for concrete implementations of the public methods.
  virtual bool GetBoundingBoxInternal(vtkTextProperty *tprop,
                                      const vtkStdString &str,
                                      int bbox[4], int dpi, int backend) = 0;
  virtual bool GetBoundingBoxInternal(vtkTextProperty *tprop,
                                      const vtkUnicodeString &str,
                                      int bbox[4], int dpi, int backend) = 0;
  virtual bool RenderStringInternal(vtkTextProperty *tprop,
                                    const vtkStdString &str,
                                    vtkImageData *data, int textDims[2],
                                    int dpi, int backend) = 0;
  virtual bool RenderStringInternal(vtkTextProperty *tprop,
                                    const vtkUnicodeString &str,
                                    vtkImageData *data, int textDims[2],
                                    int dpi, int backend) = 0;
  virtual int GetConstrainedFontSizeInternal(const vtkStdString &str,
                                             vtkTextProperty *tprop,
                                             int targetWidth, int targetHeight,
                                             int dpi, int backend) = 0;
  virtual int GetConstrainedFontSizeInternal(const vtkUnicodeString &str,
                                             vtkTextProperty *tprop,
                                             int targetWidth, int targetHeight,
                                             int dpi, int backend) = 0;
  virtual bool StringToPathInternal(vtkTextProperty *tprop,
                                    const vtkStdString &str, vtkPath *path,
                                    int backend) = 0;
  virtual bool StringToPathInternal(vtkTextProperty *tprop,
                                    const vtkUnicodeString &str, vtkPath *path,
                                    int backend) = 0;
  virtual void SetScaleToPowerOfTwoInternal(bool scale) = 0;

  // Description:
  // Set the singleton instance. Call Delete() on the supplied
  // instance after setting it to fix the reference count.
  static void SetInstance(vtkTextRenderer *instance);

  // Description:
  // The singleton instance and the singleton cleanup instance.
  static vtkTextRenderer *Instance;
  static vtkTextRendererCleanup Cleanup;

  vtksys::RegularExpression *MathTextRegExp;
  vtksys::RegularExpression *MathTextRegExp2;

  // Description:
  // Replace all instances of "\$" with "$".
  virtual void CleanUpFreeTypeEscapes(vtkStdString &str);
  virtual void CleanUpFreeTypeEscapes(vtkUnicodeString &str);

  // Description:
  // Used to cache the availability of backends. Set these in the derived class
  // constructor
  bool HasFreeType;
  bool HasMathText;

  // Description:
  // The backend to use when none is specified. Default: Detect
  int DefaultBackend;

private:
  vtkTextRenderer(const vtkTextRenderer &); // Not implemented.
  void operator=(const vtkTextRenderer &); // Not implemented.
};

#endif //__vtkTextRenderer_h
