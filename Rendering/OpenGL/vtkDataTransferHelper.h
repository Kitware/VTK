/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataTransferHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataTransferHelper - is a helper class that aids in transferring
//  data between CPU memory and GPU memory.
//
// .SECTION Description
//  vtkDataTransferHelper is a helper class that aids in transferring data
//  between the CPU memory and the GPU memory. The data in GPU memory is
//  stored as textures which that in CPU memory is stored as vtkDataArray.
//  vtkDataTransferHelper provides API to transfer only a sub-extent of CPU
//  structured data to/from the GPU.
//
// .SECTION see also
//  vtkPixelBufferObject vtkTextureObject vtkOpenGLExtensionManager

#ifndef vtkDataTransferHelper_h
#define vtkDataTransferHelper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkDataArray;
class vtkPixelBufferObject;
class vtkTextureObject;
class vtkOpenGLExtensionManager;
class vtkRenderWindow;

class VTKRENDERINGOPENGL_EXPORT vtkDataTransferHelper : public vtkObject
{
public:
  static vtkDataTransferHelper* New();
  vtkTypeMacro(vtkDataTransferHelper, vtkObject);
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
  // Set the CPU data extent. The extent matches the vtkDataArray size.
  // If the vtkDataArray comes from an vtkImageData and it is part of the
  // point data, it is usually the vtkImageData extent.
  // It can be on cell data too, but in this case it does not match the
  // vtkImageData extent.
  // If the vtkDataArray comes from a vtkDataSet, just
  // set it to a one-dimenstional extent equal to the number of tuples.
  // Initial value is (0,0,0,0,0,0), a valid one tuple array.
  vtkSetVector6Macro(CPUExtent, int);
  vtkGetVector6Macro(CPUExtent, int);

  // Description:
  // Set the GPU data extent. This is the sub-extent to copy from or to the GPU.
  // This extent matches the size of the data to transfer.
  // GPUExtent and TextureExtent don't have to match (GPUExtent can be 1D
  // whereas TextureExtent is 2D) but the number of elements have to match.
  // Initial value is (0,0,0,0,0,0), a valid one tuple array.
  vtkSetVector6Macro(GPUExtent, int);
  vtkGetVector6Macro(GPUExtent, int);

  // Description:
  // Set the texture data extent. This is the extent of the texture image that
  // will receive the data. This extent matches the size of the data to
  // transfer. If it is set to an invalid extent, GPUExtent is used.
  // See more comment on GPUExtent.
  // Initial value is an invalid extent.
  vtkSetVector6Macro(TextureExtent, int);
  vtkGetVector6Macro(TextureExtent, int);

  // Description:
  // Tells if the given extent (6 int) is valid. True if min
  // extent<=max extent.
  // \pre extent_exists: extent!=0
  bool GetExtentIsValid(int *extent);

  // Description:
  // Tells if CPUExtent is valid. True if min extent<=max extent.
  bool GetCPUExtentIsValid();

  // Description:
  // Tells if GPUExtent is valid. True if min extent<=max extent.
  bool GetGPUExtentIsValid();

  // Description:
  // Tells if TextureExtent is valid. True if min extent<=max extent.
  bool GetTextureExtentIsValid();

  // Description:
  // Define the minimal dimension of the texture regardless of the dimensions
  // of the TextureExtent. Initial value is 1.
  // A texture extent can have a given dimension 0D (one value), 1D, 2D or 3D.
  // By default 0D and 1D are translated into a 1D texture, 2D is translated
  // into a 2D texture, 3D is translated into a 3D texture. To make life easier
  // when writing GLSL code and use only one type of sampler (ex: sampler2d),
  // the default behavior can be changed by forcing a type of texture with
  // this ivar.
  // 1: default behavior. Initial value.
  // 2: force 0D and 1D to be in a 2D texture
  // 3: force 0D, 1D and 2D texture to be in a 3D texture.
  vtkSetMacro(MinTextureDimension,int);
  vtkGetMacro(MinTextureDimension,int);

  // Description:
  // Get/Set the CPU data buffer. Initial value is 0.
  vtkGetObjectMacro(Array, vtkDataArray);
  void SetArray(vtkDataArray* array);

  // Description:
  // Get/Set the GPU data buffer. Initial value is 0.
  vtkGetObjectMacro(Texture, vtkTextureObject);
  void SetTexture(vtkTextureObject* texture);

  // Description:
  // Old comment.
  // Upload Extent from CPU data buffer to GPU.
  // The WholeExtent must match the Array size.
  // New comment.
  // Upload GPUExtent from CPU vtkDataArray to GPU texture.
  // It is possible to send a subset of the components or to specify and
  // order of components or both. If components=0, componentList is ignored
  // and all components are passed, a texture cannot have more than 4
  // components.
  // \pre array_exists: array!=0
  // \pre array_not_empty: array->GetNumberOfTuples()>0
  // \pre valid_cpu_extent: this->GetCPUExtentIsValid()
  // \pre valid_cpu_extent_size: (CPUExtent[1]-CPUExtent[0]+1)*(CPUExtent[3]-CPUExtent[2]+1)*(CPUExtent[5]-CPUExtent[4]+1)==array->GetNumberOfTuples()
  // \pre valid_gpu_extent: this->GetGPUExtentIsValid()
  // \pre gpu_extent_in_cpu_extent: CPUExtent[0]<=GPUExtent[0] && GPUExtent[1]<=CPUExtent[1] && CPUExtent[2]<=GPUExtent[2] && GPUExtent[3]<=CPUExtent[3] && CPUExtent[4]<=GPUExtent[4] && GPUExtent[5]<=CPUExtent[5]
  // \pre gpu_texture_size: !this->GetTextureExtentIsValid() || (GPUExtent[1]-GPUExtent[0]+1)*(GPUExtent[3]-GPUExtent[2]+1)*(GPUExtent[5]-GPUExtent[4]+1)==(TextureExtent[1]-TextureExtent[0]+1)*(TextureExtent[3]-TextureExtent[2]+1)*(TextureExtent[5]-TextureExtent[4]+1)
  // \pre texture_can_exist_or_not: texture==0 || texture!=0
  // \pre valid_components: (components==0 && componentList==0 && array->GetNumberOfComponents()<=4) || (components>=1 && components<=array->GetNumberOfComponents() && components<=4 && componentList!=0)
  bool Upload(int components=0,
              int *componentList=NULL);

  // Description:
  // old comment:
  // Download Extent from GPU data buffer to CPU.
  // GPU data size must exactly match Extent.
  // CPU data buffer will be resized to match WholeExtent in which only the
  // Extent will be filled with the GPU data.
  // new comment:
  // Download GPUExtent from GPU texture to CPU vtkDataArray.
  // If Array is not provided, it will be created with the size of CPUExtent.
  // But only the tuples covered by GPUExtent will be download. In this case,
  // if GPUExtent does not cover all GPUExtent, some of the vtkDataArray will
  // be uninitialized.
  // Reminder: A=>B <=> !A||B
  // \pre texture_exists: texture!=0
  // \pre array_not_empty: array==0 || array->GetNumberOfTuples()>0
  // \pre valid_cpu_extent: this->GetCPUExtentIsValid()
  // \pre valid_cpu_extent_size: array==0 || (CPUExtent[1]-CPUExtent[0]+1)*(CPUExtent[3]-CPUExtent[2]+1)*(CPUExtent[5]-CPUExtent[4]+1)==array->GetNumberOfTuples()
  // \pre valid_gpu_extent: this->GetGPUExtentIsValid()
  // \pre gpu_extent_in_cpu_extent: CPUExtent[0]<=GPUExtent[0] && GPUExtent[1]<=CPUExtent[1] && CPUExtent[2]<=GPUExtent[2] && GPUExtent[3]<=CPUExtent[3] && CPUExtent[4]<=GPUExtent[4] && GPUExtent[5]<=CPUExtent[5]
  // \pre gpu_texture_size: !this->GetTextureExtentIsValid() || (GPUExtent[1]-GPUExtent[0]+1)*(GPUExtent[3]-GPUExtent[2]+1)*(GPUExtent[5]-GPUExtent[4]+1)==(TextureExtent[1]-TextureExtent[0]+1)*(TextureExtent[3]-TextureExtent[2]+1)*(TextureExtent[5]-TextureExtent[4]+1)
  // \pre valid_components: array==0 || array->GetNumberOfComponents()<=4
  // \pre components_match: array==0 || (texture->GetComponents()==array->GetNumberOfComponents())
  bool Download();

  // Description:
  // Splits the download in two operations
  // * Asynchronously download from texture memory to PBO (DownloadAsync1()).
  // * Copy from pbo to user array (DownloadAsync2()).
  bool DownloadAsync1();
  bool DownloadAsync2();

  bool GetShaderSupportsTextureInt();
  void SetShaderSupportsTextureInt(bool value);

  // Description:
  // Returns if the context supports the required extensions.
  static bool IsSupported(vtkRenderWindow* renWin);
//BTX
protected:
  vtkDataTransferHelper();
  ~vtkDataTransferHelper();

  // Description:
  // Load all necessary extensions.
  bool LoadRequiredExtensions(vtkOpenGLExtensionManager*);

  int CPUExtent[6];
  int GPUExtent[6];
  int TextureExtent[6];

  vtkWeakPointer<vtkRenderWindow> Context;
  vtkTextureObject* Texture;
  vtkDataArray* Array;
  bool ShaderSupportsTextureInt;
  int MinTextureDimension;

  vtkSmartPointer<vtkPixelBufferObject> AsyncDownloadPBO;

  vtkPixelBufferObject* GetPBO();

  // We try to reuse the PBO if possible.
  vtkSmartPointer<vtkPixelBufferObject> PBO;
private:
  vtkDataTransferHelper(const vtkDataTransferHelper&); // Not implemented.
  void operator=(const vtkDataTransferHelper&); // Not implemented.
//ETX
};

#endif
