/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataTransferHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataTransferHelper.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPixelBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"
#include <assert.h>

static void vtkGetDimensions(int extents[6], int dims[3])
{
  dims[0] = extents[1]-extents[0]+1;
  dims[1] = extents[3]-extents[2]+1;
  dims[2] = extents[5]-extents[4]+1;
}

vtkStandardNewMacro(vtkDataTransferHelper);
vtkCxxSetObjectMacro(vtkDataTransferHelper, Texture, vtkTextureObject);
vtkCxxSetObjectMacro(vtkDataTransferHelper, Array, vtkDataArray);
//----------------------------------------------------------------------------
vtkDataTransferHelper::vtkDataTransferHelper()
{
  this->Texture = 0;
  this->Context = 0;
  this->Array = 0;
  this->ShaderSupportsTextureInt=false;
  this->GPUExtent[0] = this->GPUExtent[1] =
    this->GPUExtent[2] = this->GPUExtent[3] =
    this->GPUExtent[4] = this->GPUExtent[5] = 0;
  this->CPUExtent[0] = this->CPUExtent[1] =
    this->CPUExtent[2] = this->CPUExtent[3] =
    this->CPUExtent[4] = this->CPUExtent[5] = 0;

  // invalid extent.
  this->TextureExtent[0] = 0;
  this->TextureExtent[1] = -1;
  this->TextureExtent[2] = 0;
  this->TextureExtent[3] = -1;
  this->TextureExtent[4] = 0;
  this->TextureExtent[5] = -1;

  this->MinTextureDimension=1;
}

//----------------------------------------------------------------------------
vtkDataTransferHelper::~vtkDataTransferHelper()
{
  this->SetTexture(0);
  this->SetArray(0);
  this->SetContext(0);
}

//----------------------------------------------------------------------------
// Description:
// Tells if the given extent (6 int) is valid. True if min
// extent<=max extent.
// \pre extent_exists: extent!=0
bool vtkDataTransferHelper::GetExtentIsValid(int *extent)
{
  assert("pre extent_exists:" && extent!=0);
  return extent[0] <= extent[1] && extent[2]<= extent[3]
    && extent[4]<= extent[5];
}

//----------------------------------------------------------------------------
// Description:
// Tells if CPUExtent is valid. True if min extent<=max extent.
bool vtkDataTransferHelper::GetCPUExtentIsValid()
{
  return this->GetExtentIsValid(this->CPUExtent);
}

//----------------------------------------------------------------------------
// Description:
// Tells if GPUExtent is valid. True if min extent<=max extent.
bool vtkDataTransferHelper::GetGPUExtentIsValid()
{
  return this->GetExtentIsValid(this->GPUExtent);
}

//----------------------------------------------------------------------------
// Description:
// Tells if TextureExtent is valid. True if min extent<=max extent.
bool vtkDataTransferHelper::GetTextureExtentIsValid()
{
  return this->GetExtentIsValid(this->TextureExtent);
}

//----------------------------------------------------------------------------
// Description:
// Returns if the context supports the required extensions.
bool vtkDataTransferHelper::IsSupported(vtkRenderWindow* renWin)
{
  return (vtkPixelBufferObject::IsSupported(renWin) &&
    vtkTextureObject::IsSupported(renWin));
}

//----------------------------------------------------------------------------
bool vtkDataTransferHelper::LoadRequiredExtensions(
  vtkOpenGLExtensionManager* vtkNotUsed(mgr))
{
  // This class doesn't need any particular extension. The extensions needed by
  // pixel buffer objects etc will be loaded by vtkPixelBufferObject.
  return true;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkDataTransferHelper::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void vtkDataTransferHelper::SetContext(vtkRenderWindow* renWin)
{
  if (renWin == this->Context)
    {
    return;
    }

  if (this->Texture && this->Texture->GetContext() != renWin)
    {
    this->SetTexture(0);
    }

  vtkOpenGLRenderWindow* openGLRenWin = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  this->Context = openGLRenWin;
  // release the old PBO.
  this->PBO = 0;
  if (openGLRenWin)
    {
    this->LoadRequiredExtensions(openGLRenWin->GetExtensionManager());
    }
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
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
bool vtkDataTransferHelper::Upload(int components,
                                      int *componentList)
{
  assert("pre: array_exists" && this->Array!=0);
  assert("pre: array_not_empty" && this->Array->GetNumberOfTuples()>0);
  assert("pre: valid_cpu_extent" &&  this->GetCPUExtentIsValid());
  assert("pre: valid_cpu_extent_size" &&
         ((this->CPUExtent[1]-this->CPUExtent[0]+1)*
          (this->CPUExtent[3]-this->CPUExtent[2]+1)*
          (this->CPUExtent[5]-this->CPUExtent[4]+1)
          ==this->Array->GetNumberOfTuples()));
  assert("pre: valid_gpu_extent" && this->GetGPUExtentIsValid());
  assert("pre: gpu_extent_in_cpu_extent" &&
         (this->CPUExtent[0]<=this->GPUExtent[0] &&
          this->GPUExtent[1]<=this->CPUExtent[1] &&
          this->CPUExtent[2]<=this->GPUExtent[2] &&
          this->GPUExtent[3]<=this->CPUExtent[3] &&
          this->CPUExtent[4]<=this->GPUExtent[4] &&
          this->GPUExtent[5]<=this->CPUExtent[5]));
  assert("pre: gpu_texture_size" &&
         (!this->GetTextureExtentIsValid() ||
          ((this->GPUExtent[1]-this->GPUExtent[0]+1)*
           (this->GPUExtent[3]-this->GPUExtent[2]+1)*
           (this->GPUExtent[5]-this->GPUExtent[4]+1)==
           (this->TextureExtent[1]-this->TextureExtent[0]+1)*
           (this->TextureExtent[3]-this->TextureExtent[2]+1)*
           (this->TextureExtent[5]-this->TextureExtent[4]+1))));
  assert("pre: texture_can_exist_or_not" && (this->Texture==0 ||
                                             this->Texture!=0));
  assert("pre: valid_components" &&
         ((components==0 && componentList==0 &&
           this->Array->GetNumberOfComponents()<=4) ||
          (components>=1 && components<=this->Array->GetNumberOfComponents()
           && components<=4 && componentList!=0)));

  if (!this->Context)
    {
    vtkErrorMacro("Cannot upload to GPU without context.");
    return false;
    }

  int cpudims[3];
  int gpudims[3];
  int texturedims[3];
  vtkGetDimensions(this->CPUExtent, cpudims);
  vtkGetDimensions(this->GPUExtent, gpudims);
  vtkDebugMacro( << "CPUDims: " << cpudims[0] << ", " << cpudims[1] 
                 << ", " << cpudims[2] << endl );
  vtkDebugMacro( << "GPUDims: " << gpudims[0] << ", " << gpudims[1] 
                 << ", " << gpudims[2] << endl );
  if(!this->GetTextureExtentIsValid())
    {
      // use GPU extent.
      texturedims[0]=gpudims[0];
      texturedims[1]=gpudims[1];
      texturedims[2]=gpudims[2];
    }
  else
    {
      vtkGetDimensions(this->TextureExtent, texturedims);
    }

  int numComps = this->Array->GetNumberOfComponents();

  // * Upload data to vtkPixelBufferObject.
  vtkSmartPointer<vtkPixelBufferObject> pbo = this->GetPBO();

  // We need to get the ContinuousIncrements as computed by the vtkImageData.
  // For that we create a dummy image data object,
  vtkSmartPointer<vtkImageData> tempImg = vtkSmartPointer<vtkImageData>::New();
  tempImg->SetDimensions(1, 1, 1);
  // scalars are needed for ComputeIncrements().
  tempImg->AllocateScalars();
  tempImg->SetExtent(this->CPUExtent);
  
  vtkIdType continuousInc[3];
  tempImg->GetContinuousIncrements(this->GPUExtent,
    continuousInc[0], continuousInc[1], continuousInc[2]);
  tempImg=0;

  int pt[3] = {this->GPUExtent[0]-this->CPUExtent[0], 
    this->GPUExtent[2]-this->CPUExtent[2], 
    this->GPUExtent[4]-this->CPUExtent[4]};

  vtkIdType ptId = vtkStructuredData::ComputePointId(cpudims, pt);
  if (!pbo->Upload3D(
    this->Array->GetDataType(), this->Array->GetVoidPointer(ptId*numComps),
    reinterpret_cast<unsigned int*>(gpudims), numComps, continuousInc,
    components,componentList))
    {
    vtkErrorMacro("Failed to load data to pixel buffer.");
    return false;
    }

  // * Now, we need to create a Texture for the uploaded data.
  if (!this->Texture)
    {
    vtkTextureObject* tex = vtkTextureObject::New();
    tex->SetContext(this->Context);
    this->SetTexture(tex);
    tex->Delete();
    }
  
  int tempdims[3] = {0, 0, 0};
  int dataDescription = vtkStructuredData::SetDimensions(texturedims, tempdims);
  int dimension = vtkStructuredData::GetDataDimension(dataDescription);


  bool uploaded = false;
  switch (dimension)
    {
    case 0: // 1 pixel image 
    case 1:
      {
        unsigned int length=0;
        switch (dataDescription)
          {
          case VTK_SINGLE_POINT:
            length=1;
            break;
          case VTK_X_LINE:
            length = static_cast<unsigned int>(texturedims[0]);
            break;
          case VTK_Y_LINE:
            length = static_cast<unsigned int>(texturedims[1]);
            break;
          case VTK_Z_LINE:
            length = static_cast<unsigned int>(texturedims[2]);
            break;
          }
        switch(this->MinTextureDimension)
          {
          case 1:
            uploaded = this->Texture->Create1D(numComps, pbo,
                                               this->ShaderSupportsTextureInt);
            break;
          case 2:
            uploaded = this->Texture->Create2D(length,1,numComps, pbo,
                                               this->ShaderSupportsTextureInt);
            break;
          case 3:
            uploaded = this->Texture->Create3D(length, 1, 1, numComps,
                                               pbo,
                                               this->ShaderSupportsTextureInt);
            break;
          default:
            assert("check: impossible case" && 0);
            break;
          }
      }
      break;

    case 2:
      {
        unsigned int width=0;
        unsigned int height=0;
#if 0
        switch (dataDescription)
          {
          case VTK_XY_PLANE:
            width = gpudims[0];
            height = gpudims[1];
            break;
            
          case VTK_YZ_PLANE:
            width = gpudims[1];
            height = gpudims[2];
            break;
            
          case VTK_XZ_PLANE:
            width = gpudims[0];
            height = gpudims[2];
            break;
          }
#else
#if 1
        switch (dataDescription)
          {
          case VTK_XY_PLANE:
            width = static_cast<unsigned int>(texturedims[0]);
            height = static_cast<unsigned int>(texturedims[1]);
            break;
            
          case VTK_YZ_PLANE:
            width = static_cast<unsigned int>(texturedims[1]);
            height = static_cast<unsigned int>(texturedims[2]);
            break;
            
          case VTK_XZ_PLANE:
            width = static_cast<unsigned int>(texturedims[0]);
            height = static_cast<unsigned int>(texturedims[2]);
            break;
          }
#else
        width = texturedims[0];
        height = texturedims[1];
#endif
#endif
        switch(this->MinTextureDimension)
          {
          case 1:
          case 2:
            uploaded =this->Texture->Create2D(width, height, numComps, pbo,
                                              this->ShaderSupportsTextureInt);
            break;
          case 3:
            uploaded =this->Texture->Create3D(width, height, 1, numComps, pbo,
                                              this->ShaderSupportsTextureInt);
            break;
          default:
            assert("check: impossible case" && 0);
            break;
          }
      }
      break;
      
    case 3:
      uploaded = this->Texture->Create3D(
        static_cast<unsigned int>(texturedims[0]),
        static_cast<unsigned int>(texturedims[1]),
        static_cast<unsigned int>(texturedims[2]),
        numComps, pbo,
        this->ShaderSupportsTextureInt);
      break;
    }

  pbo->ReleaseMemory();
  if (!uploaded)
    {
    vtkErrorMacro("Failed to upload data to texture.");
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
// Description:
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
bool vtkDataTransferHelper::Download()
{
  return (this->DownloadAsync1() && this->DownloadAsync2());
}

//----------------------------------------------------------------------------
bool vtkDataTransferHelper::DownloadAsync1()
{
  if (!this->Context)
    {
    vtkErrorMacro("Cannot download from GPU without context.");
    return false;
    }

  assert("pre: texture_exists" && this->Texture!=0);
  assert("pre: array_not_empty" && (this->Array==0 ||
      this->Array->GetNumberOfTuples()>0));
  assert("pre: valid_cpu_extent" && this->GetCPUExtentIsValid());
  assert("pre: valid_cpu_extent_size" &&
    (this->Array==0 ||
     ((this->CPUExtent[1]-this->CPUExtent[0]+1)*
      (this->CPUExtent[3]-this->CPUExtent[2]+1)*
      (this->CPUExtent[5]-this->CPUExtent[4]+1)==
      this->Array->GetNumberOfTuples())));
  assert("pre: valid_gpu_extent" && this->GetGPUExtentIsValid());
  assert("pre: gpu_extent_in_cpu_extent" &&
    (this->CPUExtent[0]<=this->GPUExtent[0] &&
     this->GPUExtent[1]<=this->CPUExtent[1] &&
     this->CPUExtent[2]<=this->GPUExtent[2] &&
     this->GPUExtent[3]<=this->CPUExtent[3] &&
     this->CPUExtent[4]<=this->GPUExtent[4] &&
     this->GPUExtent[5]<=this->CPUExtent[5]));
  assert("pre: gpu_texture_size" &&
    (!this->GetTextureExtentIsValid() ||
     ((this->GPUExtent[1]-this->GPUExtent[0]+1)*
      (this->GPUExtent[3]-this->GPUExtent[2]+1)*
      (this->GPUExtent[5]-this->GPUExtent[4]+1)==
      (this->TextureExtent[1]-this->TextureExtent[0]+1)*
      (this->TextureExtent[3]-this->TextureExtent[2]+1)*
      (this->TextureExtent[5]-this->TextureExtent[4]+1))));
  assert("pre: texture_can_exist_or_not" && (this->Texture==0 ||
      this->Texture!=0));
  assert("pre: valid_components" && (this->Array==0 ||
      this->Array->GetNumberOfComponents()<=4));
  assert("pre: components_match" && (this->Array==0 ||
      (this->Texture->GetComponents()==
       this->Array->GetNumberOfComponents())));

  int numComps = this->Texture->GetComponents();
  int cpudims[3];
  int gpudims[3];
  vtkGetDimensions(this->CPUExtent, cpudims);
  vtkGetDimensions(this->GPUExtent, gpudims);

  // * Download data to vtkPixelBufferObject.
  vtkSmartPointer<vtkPixelBufferObject> pbo;
  pbo.TakeReference(this->Texture->Download());

  if (!pbo.GetPointer())
    {
    vtkErrorMacro("Failed to download texture to a Pixel Buffer object.");
    return false;
    }

  if (pbo->GetSize() < 
    static_cast<unsigned int>(gpudims[0]*gpudims[1]*gpudims[2]*numComps))
    {
    vtkErrorMacro("GPU data size is smaller than GPUExtent.");
    return false;
    }

  this->AsyncDownloadPBO = pbo;
  return true;
}

//----------------------------------------------------------------------------
bool vtkDataTransferHelper::DownloadAsync2()
{
  if (!this->AsyncDownloadPBO)
    {
    vtkErrorMacro("DownloadAsync1() must be called successfully "
      "before calling DownloadAsync2().");
    return false;
    }

  int numComps = this->Texture->GetComponents();
  int cpudims[3];
  int gpudims[3];
  vtkGetDimensions(this->CPUExtent, cpudims);
  vtkGetDimensions(this->GPUExtent, gpudims);

  if (!this->Array)
    {
    vtkDataArray* array = vtkDataArray::CreateDataArray(
      this->Texture->GetDataType());
    this->SetArray(array);
    array->Delete();
    this->Array->SetNumberOfComponents(numComps);
    this->Array->SetNumberOfTuples(cpudims[0]*cpudims[1]*cpudims[2]);
    }

  // We need to get the ContinuousIncrements as computed by the vtkImageData.
  // For that we create a dummy image data object,
  vtkSmartPointer<vtkImageData> tempImg = vtkSmartPointer<vtkImageData>::New();
  tempImg->SetDimensions(1, 1, 1);
  // scalars are needed for ComputeIncrements().
  tempImg->AllocateScalars();
  tempImg->SetExtent(this->CPUExtent);
  
  vtkIdType continuousInc[3];
  tempImg->GetContinuousIncrements(this->GPUExtent,
    continuousInc[0], continuousInc[1], continuousInc[2]);
  tempImg=0;

  int pt[3] = {this->GPUExtent[0]-this->CPUExtent[0], 
    this->GPUExtent[2]-this->CPUExtent[2], 
    this->GPUExtent[4]-this->CPUExtent[4]};

  vtkIdType ptId = vtkStructuredData::ComputePointId(cpudims, pt);
  bool reply = this->AsyncDownloadPBO->Download3D(this->Array->GetDataType(),
    this->Array->GetVoidPointer(ptId*numComps),
    reinterpret_cast<unsigned int*>(gpudims), numComps,
    continuousInc);
  this->AsyncDownloadPBO = 0;
  return reply;
}

//----------------------------------------------------------------------------
bool vtkDataTransferHelper::GetShaderSupportsTextureInt()
{
  return this->ShaderSupportsTextureInt;
}

//----------------------------------------------------------------------------
void vtkDataTransferHelper::SetShaderSupportsTextureInt(bool value)
{
  this->ShaderSupportsTextureInt = value;
}

//----------------------------------------------------------------------------
vtkPixelBufferObject* vtkDataTransferHelper::GetPBO()
{
  if (!this->PBO.GetPointer())
    {
    this->PBO = vtkSmartPointer<vtkPixelBufferObject>::New();
    this->PBO->SetContext(this->Context);
    }

  return this->PBO;
}

//----------------------------------------------------------------------------
void vtkDataTransferHelper::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  
  os << indent << "Array: "               << this->Array               << endl;
  os << indent << "Texture: "             << this->Texture             << endl;
  os << indent << "MinTextureDimension: " << this->MinTextureDimension << endl;
  os << indent << "CPUExtent: ("     << this->CPUExtent[0] << ", "
                                     << this->CPUExtent[1] << ", "
                                     << this->CPUExtent[2] << ", "
                                     << this->CPUExtent[3] << ", "
                                     << this->CPUExtent[4] << ", "
                                     << this->CPUExtent[5] << ")"  << endl;
  os << indent << "GPUExtent: ("     << this->GPUExtent[0] << ", "
                                     << this->GPUExtent[1] << ", "
                                     << this->GPUExtent[2] << ", "
                                     << this->GPUExtent[3] << ", "
                                     << this->GPUExtent[4] << ", "
                                     << this->GPUExtent[5] << ")"  << endl;
  os << indent << "TextureExtent: (" << this->TextureExtent[0] << ", "
                                     << this->TextureExtent[1] << ", "
                                     << this->TextureExtent[2] << ", "
                                     << this->TextureExtent[3] << ", "
                                     << this->TextureExtent[4] << ", "
                                     << this->TextureExtent[5] << ")"  << endl;
}
