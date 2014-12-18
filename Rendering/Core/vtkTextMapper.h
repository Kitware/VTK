/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextMapper - 2D text annotation
// .SECTION Description
// vtkTextMapper provides 2D text annotation support for VTK.  It is a
// vtkMapper2D that can be associated with a vtkActor2D and placed into a
// vtkRenderer.
//
// To use vtkTextMapper, specify an input text string.
//
// .SECTION See Also
// vtkActor2D vtkTextActor vtkTextActor3D vtkTextProperty vtkTextRenderer
//
// .SECTION Note
// This class will be overridden by the older vtkOpenGLFreeTypeTextMapper when
// the vtkRenderingFreeTypeOpenGL library is linked into the executable. That
// class provides legacy support for regression testing, but lacks many of the
// newer features provided by this implementation (such as unicode and MathText
// strings). Do not link with that library if such features are needed.

#ifndef vtkTextMapper_h
#define vtkTextMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper2D.h"

#include "vtkNew.h" // For vtkNew

class vtkActor2D;
class vtkImageData;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkTextProperty;
class vtkTexture;
class vtkTimeStamp;
class vtkViewport;

class VTKRENDERINGCORE_EXPORT vtkTextMapper : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkTextMapper,vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a new text mapper.
  static vtkTextMapper *New();

  // Description:
  // Return the size[2]/width/height of the rectangle required to draw this
  // mapper (in pixels).
  virtual void GetSize(vtkViewport*, int size[2]);
  virtual int GetWidth(vtkViewport*v);
  virtual int GetHeight(vtkViewport*v);

  // Description:
  // The input text string to the mapper.
  vtkSetStringMacro(Input)
  vtkGetStringMacro(Input)

  // Description:
  // Set/Get the text property.
  virtual void SetTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(TextProperty,vtkTextProperty);

  // Description:
  // Shallow copy of an actor.
  void ShallowCopy(vtkTextMapper *tm);

  // Description:
  // Determine the number of lines in the input string.
  // @deprecated This is a legacy method that was used in an older
  // implementation, and may be removed in the future.
  VTK_LEGACY(int GetNumberOfLines(const char *input));

  // Description:
  // Get the number of lines in this mapper's input.
  // @deprecated This is a legacy method that was used in an older
  // implementation, and may be removed in the future.
  VTK_LEGACY(int GetNumberOfLines());

  // Description:
  // Set and return the font size (in points) required to make this mapper fit
  // in a given
  // target rectangle (width x height, in pixels). A static version of the method
  // is also available for convenience to other classes (e.g., widgets).
  virtual int SetConstrainedFontSize(vtkViewport*, int targetWidth, int targetHeight);
  static int SetConstrainedFontSize(vtkTextMapper*, vtkViewport*, int targetWidth, int targetHeight);

  // Description:
  // Set and return the font size (in points) required to make each element of
  // an array
  // of mappers fit in a given rectangle (width x height, in pixels).  This
  // font size is the smallest size that was required to fit the largest
  // mapper in this constraint.
  static int SetMultipleConstrainedFontSize(vtkViewport*,
                                            int targetWidth, int targetHeight,
                                            vtkTextMapper** mappers,
                                            int nbOfMappers,
                                            int* maxResultingSize);

  // Description:
  // Use these methods when setting font size relative to the renderer's size. These
  // methods are static so that external classes (e.g., widgets) can easily use them.
  static int SetRelativeFontSize(vtkTextMapper*, vtkViewport*, int *winSize,
                                 int *stringSize, float sizeFactor=0.0);
  static int SetMultipleRelativeFontSize(vtkViewport *viewport,
                                         vtkTextMapper **textMappers,
                                         int nbOfMappers, int *winSize,
                                         int *stringSize, float sizeFactor);

  // Description:
  // Get the available system font size matching a font size.
  // @deprecated This is a legacy method that was used in an older
  // implementation, and may be removed in the future.
  VTK_LEGACY(virtual int GetSystemFontSize(int size));

  void RenderOverlay(vtkViewport *, vtkActor2D *);
  void ReleaseGraphicsResources(vtkWindow *);
  unsigned long GetMTime();

protected:
  vtkTextMapper();
  ~vtkTextMapper();

  char* Input;
  vtkTextProperty *TextProperty;

private:
  vtkTextMapper(const vtkTextMapper&);  // Not implemented.
  void operator=(const vtkTextMapper&);  // Not implemented.

  void UpdateQuad(vtkActor2D *actor);
  void UpdateImage();

  int TextDims[2];

  vtkTimeStamp CoordsTime;
  vtkTimeStamp TCoordsTime;
  vtkNew<vtkImageData> Image;
  vtkNew<vtkPoints> Points;
  vtkNew<vtkPolyData> PolyData;
  vtkNew<vtkPolyDataMapper2D> Mapper;
  vtkNew<vtkTexture> Texture;
};


#ifndef VTK_LEGACY_REMOVE
inline int vtkTextMapper::GetSystemFontSize(int size)
{
  VTK_LEGACY_BODY(vtkTextMapper::GetSystemFontSize, "VTK 6.0")
  return size;
}
#endif // VTK_LEGACY_REMOVE

#endif

