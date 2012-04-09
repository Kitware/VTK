/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelBufferObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPixelBufferObject - abstracts an OpenGL pixel buffer object.
// .SECTION Description
// Provides low-level access to GPU memory. Used to pass raw data to GPU.
// The data is uploaded into a pixel buffer.
// .SECTION See Also
// OpenGL Pixel Buffer Object Extension Spec (ARB_pixel_buffer_object):
// http://www.opengl.org/registry/specs/ARB/pixel_buffer_object.txt
// .SECTION Caveats
// Since most GPUs don't support double format all double data is converted to
// float and then uploaded.
// DON'T PLAY WITH IT YET.

#ifndef __vtkPixelBufferObject_h
#define __vtkPixelBufferObject_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkRenderWindow;
class vtkOpenGLExtensionManager;

class VTKRENDERINGOPENGL_EXPORT vtkPixelBufferObject : public vtkObject
{
public:

  //BTX
  // Usage values.
  enum
  {
    StreamDraw=0,
    StreamRead,
    StreamCopy,
    StaticDraw,
    StaticRead,
    StaticCopy,
    DynamicDraw,
    DynamicRead,
    DynamicCopy,
    NumberOfUsages
  };
  //ETX

  static vtkPixelBufferObject* New();
  vtkTypeMacro(vtkPixelBufferObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the context. Context must be a vtkOpenGLRenderWindow.
  // This does not increase the reference count of the
  // context to avoid reference loops.
  // SetContext() may raise an error is the OpenGL context does not support the
  // required OpenGL extensions.
  void SetContext(vtkRenderWindow* context);
  vtkRenderWindow* GetContext();

  // Description:
  // Usage is a performance hint.
  // Valid values are:
  // - StreamDraw specified once by A, used few times S
  // - StreamRead specified once by R, queried a few times by A
  // - StreamCopy specified once by R, used a few times S
  // - StaticDraw specified once by A, used many times S
  // - StaticRead specificed once by R, queried many times by A
  // - StaticCopy specified once by R, used many times S
  // - DynamicDraw respecified repeatedly by A, used many times S
  // - DynamicRead respecified repeatedly by R, queried many times by A
  // - DynamicCopy respecified repeatedly by R, used many times S
  // A: the application
  // S: as the source for GL drawing and image specification commands.
  // R: reading data from the GL
  // Initial value is StaticDraw, as in OpenGL spec.
  vtkGetMacro(Usage,int);
  vtkSetMacro(Usage,int);

  // Description:
  // Upload data to GPU.
  // The input data can be freed after this call.
  // The data ptr is treated as an 1D array with the given number of tuples and
  // given number of components in each tuple to be copied to the GPU. increment
  // is the offset added after the last component in each tuple is transferred.
  // Look at the documentation for ContinuousIncrements in vtkImageData for
  // details about how increments are specified.
  bool Upload1D(int type, void* data,
    unsigned int numtuples, int comps, vtkIdType increment)
    {
    unsigned int newdims[3];
    newdims[0] = numtuples;
    newdims[1] = 1;
    newdims[2] = 1;
    vtkIdType newinc[3];
    newinc[0] = increment;
    newinc[1] = 0;
    newinc[2] = 0;
    return this->Upload3D(type, data, newdims, comps, newinc,0,0);
    }

  // Description:
  // Update data to GPU sourcing it from a 2D array.
  // The input data can be freed after this call.
  // The data ptr is treated as a 2D array with increments indicating how to
  // iterate over the data.
  // Look at the documentation for ContinuousIncrements in vtkImageData for
  // details about how increments are specified.
  bool Upload2D(int type, void* data,
    unsigned int dims[2],
    int comps,
    vtkIdType increments[2])
    {
    unsigned int newdims[3];
    newdims[0] = dims[0];
    newdims[1] = dims[1];
    newdims[2] = 1;
    vtkIdType newinc[3];
    newinc[0] = increments[0];
    newinc[1] = increments[1];
    newinc[2] = 0;
    return this->Upload3D(type, data, newdims, comps, newinc,0,0);
    }

  // Description:
  // Update data to GPU sourcing it from a 3D array.
  // The input data can be freed after this call.
  // The data ptr is treated as a 3D array with increments indicating how to
  // iterate over the data.
  // Look at the documentation for ContinuousIncrements in vtkImageData for
  // details about how increments are specified.
  bool Upload3D(int type, void* data,
                unsigned int dims[3], int comps,
                vtkIdType increments[3],
                int components,
                int *componentList);

  // Description:
  // Get the type with which the data is loaded into the GPU.
  // eg. VTK_FLOAT for float32, VTK_CHAR for byte, VTK_UNSIGNED_CHAR for
  // unsigned byte etc.
  vtkGetMacro(Type, int);

  // Description:
  // Get the size of the data loaded into the GPU. Size is in the number of
  // elements of the uploaded Type.
  vtkGetMacro(Size, unsigned int);

  // Description:
  // Get the openGL buffer handle.
  vtkGetMacro(Handle, unsigned int);

  // Description:
  // Download data from pixel buffer to the 1D array. The length of the array
  // must be equal to the size of the data in the memory.
  bool Download1D(
    int type, void* data,
    unsigned int dim,
    int numcomps, vtkIdType increment)
    {
    unsigned int newdims[3];
    newdims[0] = dim;
    newdims[1] = 1;
    newdims[2] = 1;
    vtkIdType newincrements[3];
    newincrements[0] = increment;
    newincrements[1] = 0;
    newincrements[2] = 0;
    return this->Download3D(type, data, newdims, numcomps, newincrements);
    }

  // Description:
  // Download data from pixel buffer to the 2D array. (lengthx * lengthy)
  // must be equal to the size of the data in the memory.
  bool Download2D(
    int type, void* data,
    unsigned int dims[2],
    int numcomps, vtkIdType increments[2])
    {
    unsigned int newdims[3];
    newdims[0] = dims[0];
    newdims[1] = dims[1];
    newdims[2] = 1;
    vtkIdType newincrements[3];
    newincrements[0] = increments[0];
    newincrements[1] = increments[1];
    newincrements[2] =  0;
    return this->Download3D(type, data, newdims, numcomps, newincrements);
    }

  // Description:
  // Download data from pixel buffer to the 3D array.
  // (lengthx * lengthy * lengthz) must be equal to the size of the data in
  // the memory.
  bool Download3D(int type, void* data,
    unsigned int dims[3],
    int numcomps, vtkIdType increments[3]);

  // Description:
  // For wrapping.
  void BindToPackedBuffer()
    { this->Bind(PACKED_BUFFER); }

  void BindToUnPackedBuffer()
    { this->Bind(UNPACKED_BUFFER); }

  // Description:
  // Inactivate the buffer.
  void UnBind();

//BTX
  // We can't use just PACKED because this is a cygwin macro defined as
  // __attribute__((packed))
  enum BufferType{
    PACKED_BUFFER,
    UNPACKED_BUFFER
  };

  // Description:
  // Make the buffer active.
  void Bind(BufferType buffer);

  // Description:
  // Allocate the memory. size is in number of bytes. type is a VTK type.
  void Allocate(unsigned int size,
                int type);

  // Description:
  // Release the memory allocated without destroying the PBO handle.
  void ReleaseMemory();

  // Description:
  // Returns if the context supports the required extensions.
  static bool IsSupported(vtkRenderWindow* renWin);

//ETX
//BTX
protected:
  vtkPixelBufferObject();
  ~vtkPixelBufferObject();

  // Description:
  // Loads all required OpenGL extensions. Must be called every time a new
  // context is set.
  bool LoadRequiredExtensions(vtkOpenGLExtensionManager* mgr);

  // Description:
  // Create the pixel buffer object.
  void CreateBuffer();

  // Description:
  // Destroys the pixel buffer object.
  void DestroyBuffer();

  int Usage;
  unsigned int BufferTarget; // GLenum
  int Type;
  unsigned int Size;
  vtkWeakPointer<vtkRenderWindow> Context;
  unsigned int Handle;
private:
  vtkPixelBufferObject(const vtkPixelBufferObject&); // Not implemented.
  void operator=(const vtkPixelBufferObject&); // Not implemented.
  //ETX
};

#endif


