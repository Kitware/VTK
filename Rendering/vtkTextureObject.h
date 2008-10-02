/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextureObject - abstracts an OpenGL texture object.
// .SECTION Description
// vtkTextureObject represents an OpenGL texture object. It provides API to
// create textures using data already loaded into pixel buffer objects. It can
// also be used to create textures without uploading any data.
// .SECTION Caveats
// DON'T PLAY WITH IT YET.
#ifndef __vtkTextureObject_h
#define __vtkTextureObject_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkRenderWindow;
class vtkOpenGLExtensionManager;
class vtkPixelBufferObject;

class VTK_RENDERING_EXPORT vtkTextureObject : public vtkObject
{
public:
  static vtkTextureObject* New();
  vtkTypeRevisionMacro(vtkTextureObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the context. This does not increase the reference count of the 
  // context to avoid reference loops.
  // SetContext() may raise an error is the OpenGL context does not support the
  // required OpenGL extensions.
  void SetContext(vtkRenderWindow*);
  vtkRenderWindow* GetContext();

  // Description:
  // Get the texture dimensions.
  // These are the properties of the OpenGL texture this instance represents.
  vtkGetMacro(Width, unsigned int);
  vtkGetMacro(Height, unsigned int);
  vtkGetMacro(Depth, unsigned int);
  vtkGetMacro(Components, int);

  vtkGetMacro(NumberOfDimensions, int);

  // Description:
  // Returns OpenGL texture target to which the texture is/can be bound.
  vtkGetMacro(Target, int);

  // Description:
  // Returns the OpenGL handle.
  vtkGetMacro(Handle, unsigned int);

  // Description:
  // Activate the texture. The texture must have been created using Create().
  // RenderWindow must be set before calling this. 
  void Bind();
  void UnBind();

  // Description:
  // Create a 1D texture using the PBO.
  // Eventually we may start supporting creating a texture from subset of data
  // in the PBO, but for simplicity we'll begin with entire PBO data.
  // numComps must be in [1-4].
  // shaderSupportsTextureInt is true if the shader has an alternate
  // implementation supporting sampler with integer values.
  // Even if the card supports texture int, it does not mean that
  // the implementor of the shader made a version that supports texture int.
  bool Create1D(int numComps,
                vtkPixelBufferObject *pbo,
                bool shaderSupportsTextureInt);

  // Description:
  // Create a 2D texture using the PBO.
  // Eventually we may start supporting creating a texture from subset of data
  // in the PBO, but for simplicity we'll begin with entire PBO data.
  // numComps must be in [1-4].
  bool Create2D(unsigned int width, unsigned int height, int numComps,
                vtkPixelBufferObject *pbo,
                bool shaderSupportsTextureInt);

  // Description:
  // Create a 3D texture using the PBO.
  // Eventually we may start supporting creating a texture from subset of data
  // in the PBO, but for simplicity we'll begin with entire PBO data.
  // numComps must be in [1-4].
  bool Create3D(unsigned int width, unsigned int height, unsigned int depth, 
                int numComps, vtkPixelBufferObject *pbo,
                bool shaderSupportsTextureInt);


  // Description:
  // Create texture without uploading any data.
  // To create a DEPTH_COMPONENT texture, vtktype must be set to VTK_VOID and
  // numComps must be 1.
  bool Create2D(unsigned int width, unsigned int height, int numComps,
                int vtktype,
                bool shaderSupportsTextureInt);
  bool Create3D(unsigned int width, unsigned int height, unsigned int depth,
                int numComps, int vtktype,
                bool shaderSupportsTextureInt);

  // Description:
  // This is used to download raw data from the texture into a pixel bufer. The
  // pixel buffer API can then be used to download the pixel buffer data to CPU
  // arrays. The caller takes on the responsibility of deleting the returns
  // vtkPixelBufferObject once it done with it.
  vtkPixelBufferObject* Download();

  // Description:
  // Get the data type for the texture as a vtk type int i.e. VTK_INT etc.
  int GetDataType();

  int GetInternalFormat(int vtktype, int numComps,
                        bool shaderSupportsTextureInt);
  int GetFormat(int vtktype, int numComps,
                bool shaderSupportsTextureInt);

  // Description:
  // Returns if the context supports the required extensions.
  static bool IsSupported(vtkRenderWindow* renWin);
//BTX
protected:
  vtkTextureObject();
  ~vtkTextureObject();

  // Description:
  // Load all necessary extensions.
  bool LoadRequiredExtensions(vtkOpenGLExtensionManager*);

  // Description:
  // Creates a texture handle if not already created.
  void CreateTexture();

  // Description:
  // Destory the texture.
  void DestroyTexture();

  int NumberOfDimensions;
  unsigned int Width;
  unsigned int Height;
  unsigned int Depth;

  int Target; // GLenum
  int Format; // GLint
  int Type; // GLint
  int Components;

  vtkWeakPointer<vtkRenderWindow> Context;
  unsigned int Handle;
  bool SupportsTextureInteger;

private:
  vtkTextureObject(const vtkTextureObject&); // Not implemented.
  void operator=(const vtkTextureObject&); // Not implemented.
//ETX
};

#endif


