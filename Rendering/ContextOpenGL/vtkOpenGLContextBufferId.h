/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextBufferId.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkOpenGLContextBufferId - 2D array of ids stored in VRAM.
//
// .SECTION Description
// An 2D array where each element is the id of an entity drawn at the given
// pixel.

#ifndef vtkOpenGLContextBufferId_h
#define vtkOpenGLContextBufferId_h

#include "vtkRenderingContextOpenGLModule.h" // For export macro
#include "vtkAbstractContextBufferId.h"

class vtkTextureObject;
class vtkOpenGLRenderWindow;

class VTKRENDERINGCONTEXTOPENGL_EXPORT vtkOpenGLContextBufferId : public vtkAbstractContextBufferId
{
public:
  vtkTypeMacro(vtkOpenGLContextBufferId, vtkAbstractContextBufferId);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Painter object.
  static vtkOpenGLContextBufferId *New();

  // Description:
  // Release any graphics resources that are being consumed by this object.
  virtual void ReleaseGraphicsResources();

  // Description:
  // Set/Get the OpenGL context owning the texture object resource.
  virtual void SetContext(vtkRenderWindow *context);
  virtual vtkRenderWindow *GetContext();

  // Description:
  // Returns if the context supports the required extensions.
  // \pre context_is_set: this->GetContext()!=0
  virtual bool IsSupported();

  // Description:
  // Allocate the memory for at least Width*Height elements.
  // \pre positive_width: GetWidth()>0
  // \pre positive_height: GetHeight()>0
  // \pre context_is_set: this->GetContext()!=0
  virtual void Allocate();

  // Description:
  // Tell if the buffer has been allocated.
  virtual bool IsAllocated() const;

  // Description:
  // Copy the contents of the current read buffer to the internal texture
  // starting at lower left corner of the framebuffer (srcXmin,srcYmin).
  // \pre is_allocated: this->IsAllocated()
  virtual void SetValues(int srcXmin,
                         int srcYmin);

  // Description:
  // Return item under abscissa x and ordinate y.
  // Abscissa go from left to right.
  // Ordinate go from bottom to top.
  // The return value is -1 if there is no item.
  // \pre is_allocated: IsAllocated()
  // \post valid_result: result>=-1
  virtual vtkIdType GetPickedItem(int x, int y);

protected:
  vtkOpenGLContextBufferId();
  virtual ~vtkOpenGLContextBufferId();

  vtkOpenGLRenderWindow *Context;
  vtkTextureObject *Texture;

private:
  vtkOpenGLContextBufferId(const vtkOpenGLContextBufferId &); // Not implemented.
  void operator=(const vtkOpenGLContextBufferId &);   // Not implemented.
};

#endif // #ifndef vtkOpenGLContextBufferId_h
