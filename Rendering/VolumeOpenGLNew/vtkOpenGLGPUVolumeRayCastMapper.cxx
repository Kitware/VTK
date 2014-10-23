/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGPUVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLGPUVolumeRayCastMapper.h"

#include "vtkOpenGLGradientOpacityTable.h"
#include "vtkOpenGLOpacityTable.h"
#include "vtkOpenGLRGBTable.h"
#include "vtkVolumeShaderComposer.h"
#include "vtkVolumeShader.h"
#include "vtkVolumeStateRAII.h"

// Include compiled shader code
#include <raycasterfs.h>
#include <raycastervs.h>

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkClipConvexPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDensifyPolyData.h>
#include <vtkFloatArray.h>
#include <vtk_glew.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPerlinNoise.h>
#include <vtkPlaneCollection.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkTessellatedBoxSource.h>
#include <vtkTimerLog.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkVolumeMask.h>
#include <vtkVolumeProperty.h>

// C/C++ includes
#include <cassert>
#include <limits>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkOpenGLGPUVolumeRayCastMapper);

//----------------------------------------------------------------------------
class vtkOpenGLGPUVolumeRayCastMapper::vtkInternal
{
public:
  // Constructor
  //--------------------------------------------------------------------------
  vtkInternal(vtkOpenGLGPUVolumeRayCastMapper* parent)
    {
    this->Parent = parent;
    this->Initialized = false;
    this->ValidTransferFunction = false;
    this->LoadDepthTextureExtensionsSucceeded = false;
    this->CameraWasInsideInLastUpdate = false;
    this->CubeVBOId = 0;
#ifndef __APPLE__
    this->CubeVAOId = 0;
#endif
    this->CubeIndicesId = 0;
    this->VolumeTextureId = 0;
    this->NoiseTextureId = 0;
    this->DepthTextureId = 0;
    this->TextureWidth = 1024;
    this->ActualSampleDistance = 1.0;
    this->RGBTable = 0;
    this->OpacityTables = 0;
    this->Mask1RGBTable = 0;
    this->Mask2RGBTable =  0;
    this->GradientOpacityTables = 0;
    this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = -1;
    this->TextureSize[0] = this->TextureSize[1] = this->TextureSize[2] = -1;
    this->CellScale[0] = this->CellScale[1] = this->CellScale[2] = 0.0;
    this->NoiseTextureData = 0;

    this->Extents[0] = VTK_INT_MAX;
    this->Extents[1] = VTK_INT_MIN;
    this->Extents[2] = VTK_INT_MAX;
    this->Extents[3] = VTK_INT_MIN;
    this->Extents[4] = VTK_INT_MAX;
    this->Extents[5] = VTK_INT_MIN;

    this->MaskTextures = new vtkMapMaskTextureId;

    this->PrevInput = NULL;
    }

  // Destructor
  //--------------------------------------------------------------------------
  ~vtkInternal()
    {
    delete this->RGBTable;
    this->RGBTable = 0;

    if(this->Mask1RGBTable!=0)
      {
      delete this->Mask1RGBTable;
      this->Mask1RGBTable=0;
      }

    if(this->Mask2RGBTable!=0)
      {
      delete this->Mask2RGBTable;
      this->Mask2RGBTable=0;
      }

    delete this->OpacityTables;
    this->OpacityTables = 0;

    delete this->GradientOpacityTables;
    this->GradientOpacityTables = 0;

    delete this->NoiseTextureData;
    this->NoiseTextureData = 0;

    if (this->MaskTextures != 0)
      {
      if (!this->MaskTextures->Map.empty())
        {
        std::map<vtkImageData*,vtkVolumeMask*>::iterator it =
          this->MaskTextures->Map.begin();
        while(it != this->MaskTextures->Map.end())
          {
          vtkVolumeMask* texture = (*it).second;
          delete texture;
          ++it;
          }
        this->MaskTextures->Map.clear();
        }
      }
    }

  // Helper methods
  //--------------------------------------------------------------------------
  template<typename T>
  static void ToFloat(const T& in1, const T& in2, float (&out)[2]);
  template<typename T>
  static void ToFloat(const T& in1, const T& in2, const T& in3,
                      float (&out)[3]);
  template<typename T>
  static void ToFloat(T* in, float* out, int numberOfComponents);
  template<typename T>
  static void ToFloat(T (&in)[3], float (&out)[3]);
  template<typename T>
  static void ToFloat(T (&in)[2], float (&out)[2]);
  template<typename T>
  static void ToFloat(T& in, float& out);
  static void VtkToGlMatrix(vtkMatrix4x4* in, float (&out)[16]);

  void Initialize(vtkRenderer* ren, vtkVolume* vol);

  bool LoadVolume(vtkImageData* imageData, vtkDataArray* scalars);

  bool LoadMask(vtkImageData* input,
                vtkImageData* maskInput,
                int textureExtent[6],
                vtkVolume* volume);

  bool IsInitialized();

  void CompileAndLinkShader(const string& vertexShader,
                            const string& fragmentShader);

  void ComputeBounds(vtkImageData* input);


  // Update transfer color function based on the incoming inputs and number of
  // scalar components.
  // TODO Deal with numberOfScalarComponents > 1
  int UpdateColorTransferFunction(vtkVolume* vol, int numberOfScalarComponents);

  // Update opacity transfer function (not gradient opacity)
  int UpdateOpacityTransferFunction(vtkVolume* vol,
                                    int numberOfScalarComponents,
                                    unsigned int level);

  // Update gradient opacity function
  int UpdateGradientOpacityTransferFunction(vtkVolume* vol,
                                            int numberOfScalarComponents,
                                            unsigned int level);

  // Update noise texture (used to reduce rendering artifacts
  // specifically banding effects)
  void UpdateNoiseTexture();

  // Update depth texture (used for early termination of the ray)
  void UpdateDepthTexture(vtkRenderer* ren, vtkVolume* vol);

  // Test if camera is inside the volume geometry
  bool IsCameraInside(vtkRenderer* ren, vtkVolume* vol);

  // Update the volume geometry
  void UpdateVolumeGeometry(vtkRenderer* ren, vtkVolume* vol,
                            vtkImageData* input);

  // Update cropping params to shader
  void UpdateCropping(vtkRenderer* ren, vtkVolume* vol);

  // Update clipping params to shader
  void UpdateClipping(vtkRenderer* ren, vtkVolume* vol);

  // Update the interval of sampling
  void UpdateSamplingDistance(vtkImageData *input,
                              vtkRenderer* ren, vtkVolume* vol);

  // Load OpenGL extensiosn required to grab depth sampler buffer
  void LoadRequireDepthTextureExtensions(vtkRenderWindow* renWin);

  // Create GL buffers
  void CreateBufferObjects();

  // Dispose / free GL buffers
  void DeleteBufferObjects();

  // Private member variables
  //--------------------------------------------------------------------------
  vtkOpenGLGPUVolumeRayCastMapper* Parent;

  bool Initialized;
  bool ValidTransferFunction;
  bool LoadDepthTextureExtensionsSucceeded;
  bool CameraWasInsideInLastUpdate;

  GLuint CubeVBOId;
#ifndef __APPLE__
  GLuint CubeVAOId;
#endif
  GLuint CubeIndicesId;

  GLuint VolumeTextureId;
  GLuint NoiseTextureId;
  GLuint DepthTextureId;

  vtkVolumeShader Shader;

  int TextureWidth;

  double Scale;
  double Bias;

  float* NoiseTextureData;
  GLint NoiseTextureSize;

  float ActualSampleDistance;

  int LastProjectionParallel;
  int Dimensions[3];
  int TextureSize[3];
  int WindowLowerLeft[2];
  int WindowSize[2];

  double ScalarsRange[2];
  double LoadedBounds[6];
  int Extents[6];
  double DatasetStepSize[3];
  double CellScale[3];
  double CellStep[3];
  double CellSpacing[3];

  std::ostringstream ExtensionsStringStream;

  vtkOpenGLRGBTable* RGBTable;
  vtkOpenGLOpacityTables* OpacityTables;
  vtkOpenGLRGBTable* Mask1RGBTable;
  vtkOpenGLRGBTable* Mask2RGBTable;
  vtkOpenGLGradientOpacityTables* GradientOpacityTables;

  vtkTimeStamp ShaderBuildTime;

  vtkNew<vtkMatrix4x4> TextureToDataSetMat;
  vtkNew<vtkMatrix4x4> InverseTextureToDataSetMat;

  vtkNew<vtkMatrix4x4> InverseProjectionMat;
  vtkNew<vtkMatrix4x4> InverseModelViewMat;
  vtkNew<vtkMatrix4x4> InverseVolumeMat;

  vtkSmartPointer<vtkPolyData> BBoxPolyData;

  vtkMapMaskTextureId* MaskTextures;
  vtkVolumeMask* CurrentMask;

  vtkImageData* PrevInput;
};

//----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  const T& in1, const T& in2, float (&out)[2])
{
  out[0] = static_cast<float>(in1);
  out[1] = static_cast<float>(in2);
}

template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  const T& in1, const T& in2, const T& in3, float (&out)[3])
{
  out[0] = static_cast<float>(in1);
  out[1] = static_cast<float>(in2);
  out[2] = static_cast<float>(in3);
}

//----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T* in, float* out, int numberOfComponents)
{
  for (int i = 0; i < numberOfComponents; ++i)
    {
    out[i] = static_cast<float>(in[i]);
    }
}

//----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T (&in)[3], float (&out)[3])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
  out[2] = static_cast<float>(in[2]);
}

//----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T (&in)[2], float (&out)[2])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
}

//----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T& in, float& out)
{
  out = static_cast<float>(in);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::VtkToGlMatrix(
  vtkMatrix4x4* in, float (&out)[16])
{
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      out[j * 4 + i] = in->Element[i][j];
      }
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::Initialize(
  vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol))
{
  GLenum err = glewInit();
  if (GLEW_OK != err)
    {
    cerr <<"Error: "<< glewGetErrorString(err)<<endl;
    }

  // This is to ignore INVALID ENUM error 1282
  err = glGetError();

  // Create RGB lookup table
  this->RGBTable = new vtkOpenGLRGBTable();

  if (this->Parent->MaskInput != 0 &&
      this->Parent->MaskType == LabelMapMaskType)
    {
    if(this->Mask1RGBTable == 0)
      {
      this->Mask1RGBTable = new vtkOpenGLRGBTable();
      }
    if(this->Mask2RGBTable == 0)
      {
      this->Mask2RGBTable = new vtkOpenGLRGBTable();
      }
    }

  // TODO Currently we are supporting only one level
  // Create opacity lookup table
  this->OpacityTables = new vtkOpenGLOpacityTables(1);

  this->Initialized = true;
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadVolume(
  vtkImageData* imageData, vtkDataArray* scalars)
{
  // Generate OpenGL texture
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &this->VolumeTextureId);
  glBindTexture(GL_TEXTURE_3D, this->VolumeTextureId);

  // Set the texture parameters
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  GLfloat borderColor[4]={0.0,0.0,0.0,0.0};
  glTexParameterfv(GL_TEXTURE_3D,GL_TEXTURE_BORDER_COLOR, borderColor);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // Allocate data with internal format and foramt as (GL_RED)
  GLint internalFormat = 0;
  GLenum format = 0;
  GLenum type = 0;

  double shift = 0.0;
  double scale = 1.0;
  bool handleLargeDataTypes = false;

  int scalarType = scalars->GetDataType();
  if (scalars->GetNumberOfComponents()==4)
    {
    internalFormat = GL_RGBA16;
    format = GL_RGBA;
    type = GL_UNSIGNED_BYTE;
    }
  else
    {
    switch(scalarType)
      {
      case VTK_FLOAT:
      if (glewIsSupported("GL_ARB_texture_float"))
        {
        internalFormat = GL_INTENSITY16F_ARB;
        }
      else
        {
        internalFormat = GL_INTENSITY16;
        }
        format = GL_RED;
        type = GL_FLOAT;
        shift=-ScalarsRange[0];
        scale = 1/(this->ScalarsRange[1]-this->ScalarsRange[0]);
        break;
      case VTK_UNSIGNED_CHAR:
        internalFormat = GL_INTENSITY8;
        format = GL_RED;
        type = GL_UNSIGNED_BYTE;
        shift = -this->ScalarsRange[0]/VTK_UNSIGNED_CHAR_MAX;
        scale =
          VTK_UNSIGNED_CHAR_MAX/(this->ScalarsRange[1]-this->ScalarsRange[0]);
        break;
      case VTK_SIGNED_CHAR:
        internalFormat = GL_INTENSITY8;
        format = GL_RED;
        type = GL_BYTE;
        shift = -(2 * this->ScalarsRange[0] + 1)/VTK_UNSIGNED_CHAR_MAX;
        scale = VTK_SIGNED_CHAR_MAX / (this->ScalarsRange[1] -
                                       this->ScalarsRange[0]);
        break;
      case VTK_CHAR:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_BIT:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_ID_TYPE:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_INT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_INT;
        shift=-(2*this->ScalarsRange[0]+1)/VTK_UNSIGNED_INT_MAX;
        scale = VTK_INT_MAX/(this->ScalarsRange[1]-this->ScalarsRange[0]);
        break;
      case VTK_DOUBLE:
      case VTK___INT64:
      case VTK_LONG:
      case VTK_LONG_LONG:
      case VTK_UNSIGNED___INT64:
      case VTK_UNSIGNED_LONG:
      case VTK_UNSIGNED_LONG_LONG:
        handleLargeDataTypes = true;
        if (glewIsSupported("GL_ARB_texture_float"))
          {
          internalFormat=GL_INTENSITY16F_ARB;
          }
        else
          {
          internalFormat=GL_INTENSITY16;
          }
        format = GL_RED;
        type = GL_FLOAT;
        shift = -this->ScalarsRange[0];
        scale = 1 / (this->ScalarsRange[1] - this->ScalarsRange[0]);
        break;
      case VTK_SHORT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_SHORT;
        shift = -(2*this->ScalarsRange[0]+1)/VTK_UNSIGNED_SHORT_MAX;
        scale = VTK_SHORT_MAX / (this->ScalarsRange[1] -
                                 this->ScalarsRange[0]);
        break;
      case VTK_STRING:
        // not supported
        assert("check: impossible case" && 0);
        break;
      case VTK_UNSIGNED_SHORT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_UNSIGNED_SHORT;

        shift=-this->ScalarsRange[0]/VTK_UNSIGNED_SHORT_MAX;
        scale=
          VTK_UNSIGNED_SHORT_MAX / (this->ScalarsRange[1] -
                                    this->ScalarsRange[0]);
        break;
      case VTK_UNSIGNED_INT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_UNSIGNED_INT;
        shift=-this->ScalarsRange[0]/VTK_UNSIGNED_INT_MAX;
        scale = VTK_UNSIGNED_INT_MAX / (this->ScalarsRange[1] -
                                        this->ScalarsRange[0]);
        break;
      default:
        assert("check: impossible case" && 0);
        break;
      }
    }

  // Update scale and bias
  this->Scale = scale;
  this->Bias = shift * this->Scale;

  imageData->GetExtent(this->Extents);

  int i = 0;
  while(i < 3)
    {
    this->TextureSize[i] = this->Extents[2*i+1] - this->Extents[2*i] + 1;
    ++i;
    }

  if (!handleLargeDataTypes)
    {
    void* dataPtr = scalars->GetVoidPointer(0);

    glPixelTransferf(GL_RED_SCALE,static_cast<GLfloat>(this->Scale));
    glPixelTransferf(GL_RED_BIAS,static_cast<GLfloat>(this->Bias));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
                 this->TextureSize[0],this->TextureSize[1],
                 this->TextureSize[2], 0,
                 format, type, dataPtr);



    // Set scale and bias to their defaults
    glPixelTransferf(GL_RED_SCALE,1.0);
    glPixelTransferf(GL_RED_BIAS, 0.0);
    }
  else
    {
    // Convert and send to the GPU, z-slice by z-slice so that we won't allocate
    // memory at once.Allocate memory on the GPU (NULL data pointer with the
    // right dimensions). Here we are assuming that
    // GL_ARB_texture_non_power_of_two is available
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
                 this->TextureSize[0], this->TextureSize[1],
                 this->TextureSize[2], 0, format, type, 0);

    // Send the slices one by one to the GPU. We are not sending all of them
    // together so as to avoid allocating big data on the GPU which may not
    // work if the original dataset is big as well.
    vtkFloatArray* sliceArray=vtkFloatArray::New();
    sliceArray->SetNumberOfComponents(1);
    sliceArray->SetNumberOfTuples(this->TextureSize[0] * this->TextureSize[1]);
    void* slicePtr = sliceArray->GetVoidPointer(0);
    int k = 0;
    int kInc = (this->Dimensions[0] - this->Parent->CellFlag) *
               (this->Dimensions[1] - this->Parent->CellFlag);
    int kOffset = (this->Extents[4] *
                  (this->Dimensions[1] - this->Parent->CellFlag) +
                   this->Extents[2]) *
                  (this->Dimensions[0] - this->Parent->CellFlag) +
                   this->Extents[0];
    while(k < this->TextureSize[2])
      {
      int j = 0;
      int jOffset = 0;
      int jDestOffset = 0;
      while(j < this->TextureSize[1])
        {
        i = 0;
        while(i < this->TextureSize[0])
          {
          sliceArray->SetTuple1(jDestOffset + i,
                                (scalars->GetTuple1(kOffset + jOffset + i) +
                                 shift)*scale);
          ++i;
          }
        ++j;
        jOffset += this->Dimensions[0] - this->Parent->CellFlag;
        jDestOffset += this->TextureSize[0];
        }

      // Here we are assuming that GL_ARB_texture_non_power_of_two is
      // available
      glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, k,
                      this->TextureSize[0], this->TextureSize[1], 1,
                      format,type, slicePtr);
      ++k;
      kOffset += kInc;
      }
    sliceArray->Delete();
    }

  return 1;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadMask(
  vtkImageData* vtkNotUsed(input), vtkImageData* maskInput,
  int textureExtent[6], vtkVolume* vtkNotUsed(volume))
{
  bool result = true;

  // Mask
  if(maskInput != 0)
    {
    // Find the texture.
    std::map<vtkImageData *,vtkVolumeMask*>::iterator it2 =
      this->MaskTextures->Map.find(maskInput);

    vtkVolumeMask* mask;
    if(it2 == this->MaskTextures->Map.end())
      {
      mask = new vtkVolumeMask();
      this->MaskTextures->Map[maskInput] = mask;
      }
    else
      {
      mask = (*it2).second;
      }

    mask->Update(maskInput,
                 this->Parent->CellFlag,
                 textureExtent,
                 this->Parent->ScalarMode,
                 this->Parent->ArrayAccessMode,
                 this->Parent->ArrayId,
                 this->Parent->ArrayName,
                 static_cast<vtkIdType>(static_cast<float>(
                   this->Parent->MaxMemoryInBytes) *
                   this->Parent->MaxMemoryFraction));

    result = result && mask->IsLoaded();
    this->CurrentMask = mask;
    }

  return result;
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::IsInitialized()
{
  return this->Initialized;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CompileAndLinkShader(
  const std::string& vertexShader, const std::string& fragmentShader)
{
  this->Shader.LoadFromString(GL_VERTEX_SHADER, vertexShader);
  this->Shader.LoadFromString(GL_FRAGMENT_SHADER, fragmentShader);

  // Compile and link the shader
  this->Shader.CreateAndLinkProgram();
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ComputeBounds(
  vtkImageData* input)
{
  double origin[3];

  input->GetSpacing(this->CellSpacing);
  input->GetOrigin(origin);
  input->GetExtent(this->Extents);

  int swapBounds[3];
  swapBounds[0] = (this->CellSpacing[0] < 0);
  swapBounds[1] = (this->CellSpacing[1] < 0);
  swapBounds[2] = (this->CellSpacing[2] < 0);

  // Loaded data represents points
  if (!this->Parent->CellFlag)
    {
    // If spacing is negative, we may have to rethink the equation
    // between real point and texture coordinate...
    this->LoadedBounds[0] = origin[0] +
      static_cast<double>(this->Extents[0 + swapBounds[0]]) *
      this->CellSpacing[0];
    this->LoadedBounds[2] = origin[1] +
      static_cast<double>(this->Extents[2 + swapBounds[1]]) *
      this->CellSpacing[1];
    this->LoadedBounds[4] = origin[2] +
      static_cast<double>(this->Extents[4 + swapBounds[2]]) *
      this->CellSpacing[2];
    this->LoadedBounds[1] = origin[0] +
      static_cast<double>(this->Extents[1 - swapBounds[0]]) *
      this->CellSpacing[0];
    this->LoadedBounds[3] = origin[1] +
      static_cast<double>(this->Extents[3 - swapBounds[1]]) *
      this->CellSpacing[1];
    this->LoadedBounds[5] = origin[2] +
      static_cast<double>(this->Extents[5 - swapBounds[2]]) *
      this->CellSpacing[2];
    }
  // Loaded extents represent cells
  else
    {
    int wholeTextureExtent[6];
    input->GetExtent(wholeTextureExtent);
    int i = 1;
    while (i < 6)
      {
      wholeTextureExtent[i]--;
      i += 2;
      }

    i = 0;
    while (i < 3)
      {
      if(this->Extents[2 * i] == wholeTextureExtent[2 * i])
        {
        this->LoadedBounds[2 * i + swapBounds[i]] = origin[i];
        }
      else
        {
        this->LoadedBounds[2 * i + swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i]) + 0.5) *
          this->CellSpacing[i];
        }

      if(this->Extents[2 * i + 1] == wholeTextureExtent[2 * i + 1])
        {
        this->LoadedBounds[2 * i + 1 - swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i + 1]) + 1.0) *
          this->CellSpacing[i];
        }
      else
        {
        this->LoadedBounds[2 * i + 1-swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i + 1]) + 0.5) *
          this->CellSpacing[i];
        }
      ++i;
      }
    }
}

//----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateColorTransferFunction(
  vtkVolume* vol, int numberOfScalarComponents)
{
  // Build the colormap in a 1D texture.
  // 1D RGB-texture=mapping from scalar values to color values
  // build the table.
  if(numberOfScalarComponents == 1)
    {
    vtkVolumeProperty* volumeProperty = vol->GetProperty();
    vtkColorTransferFunction* colorTransferFunction =
      volumeProperty->GetRGBTransferFunction(0);

    // Add points only if its not being added before
    if (colorTransferFunction->GetSize() < 1)
      {
      colorTransferFunction->AddRGBPoint(this->ScalarsRange[0], 0.0, 0.0, 0.0);
      colorTransferFunction->AddRGBPoint(this->ScalarsRange[1], 1.0, 1.0, 1.0);
      }

    this->RGBTable->Update(
      colorTransferFunction, this->ScalarsRange,
      volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION);
    }

  if (this->Parent->MaskInput != 0 &&
      this->Parent->MaskType == LabelMapMaskType)
    {
    vtkVolumeProperty* volumeProperty = vol->GetProperty();

    vtkColorTransferFunction* colorTransferFunc =
      volumeProperty->GetRGBTransferFunction(1);
    this->Mask1RGBTable->Update(colorTransferFunc, this->ScalarsRange,
                                false, 7);

    colorTransferFunc = volumeProperty->GetRGBTransferFunction(2);
    this->Mask2RGBTable->Update(colorTransferFunc, this->ScalarsRange,
                                false, 8);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateOpacityTransferFunction(
  vtkVolume* vol, int vtkNotUsed(numberOfScalarComponents), unsigned int level)
{
  if (!vol)
    {
    std::cerr << "Invalid m_volume" << std::endl;
    return 1;
    }

  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  vtkPiecewiseFunction* scalarOpacity = volumeProperty->GetScalarOpacity();

  // TODO: Do a better job to create the default opacity map
  // Add points only if its not being added before
  if (scalarOpacity->GetSize() < 1)
    {
    scalarOpacity->AddPoint(this->ScalarsRange[0], 0.0);
    scalarOpacity->AddPoint(this->ScalarsRange[1], 0.5);
    }

  this->OpacityTables->GetTable(level)->Update(
    scalarOpacity,this->Parent->BlendMode,
    this->ActualSampleDistance,
    this->ScalarsRange,
    volumeProperty->GetScalarOpacityUnitDistance(),
    volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION);
  return 0;
}

//----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::
  UpdateGradientOpacityTransferFunction(vtkVolume* vol,
    int vtkNotUsed(numberOfScalarComponents), unsigned int level)
{
  if (!vol)
    {
    std::cerr << "Invalid m_volume" << std::endl;
    return 1;
    }

  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  vtkPiecewiseFunction* gradientOpacity = volumeProperty->GetGradientOpacity();

  if (!this->GradientOpacityTables && gradientOpacity)
    {
    // NOTE Handling only one component
    this->GradientOpacityTables = new vtkOpenGLGradientOpacityTables(1);
    }

  // TODO: Do a better job to create the default opacity map
  // Add points only if its not being added before
  if (gradientOpacity->GetSize() < 1)
    {
    gradientOpacity->AddPoint(this->ScalarsRange[0], 0.0);
    gradientOpacity->AddPoint(this->ScalarsRange[1], 0.5);
    }

  this->GradientOpacityTables->GetTable(level)->Update(
    gradientOpacity,
    this->ActualSampleDistance,
    this->ScalarsRange,
    volumeProperty->GetScalarOpacityUnitDistance(),
    volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION);

  return 0;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateNoiseTexture()
{
  if (!this->NoiseTextureId)
    {
    glActiveTexture(GL_TEXTURE3);
    glGenTextures(1, &this->NoiseTextureId);
    glBindTexture(GL_TEXTURE_2D, this->NoiseTextureId);

    GLsizei size = 128;
    GLint maxSize;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (size > maxSize)
      {
      size=maxSize;
      }

    if (this->NoiseTextureData != 0 && this->NoiseTextureSize != size)
      {
      delete[] this->NoiseTextureData;
      this->NoiseTextureData = 0;
      }

    if (this->NoiseTextureData == 0)
      {
      this->NoiseTextureData = new float[size * size];
      this->NoiseTextureSize = size;
      vtkNew<vtkPerlinNoise> noiseGenerator;
      noiseGenerator->SetFrequency(size, 1.0, 1.0);
      noiseGenerator->SetPhase(0.0, 0.0, 0.0);
      // -0.5 and 0.5 range
      noiseGenerator->SetAmplitude(0.5);
      int j = 0;
      while(j < size)
        {
        int i = 0;
        while(i < size)
          {
          this->NoiseTextureData[j * size + i] =
            static_cast<float>(noiseGenerator->EvaluateFunction(i, j, 0.0));
          ++i;
          }
        ++j;
        }
      }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, size, size, 0,
                 GL_RED, GL_FLOAT, this->NoiseTextureData);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glActiveTexture(GL_TEXTURE0);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateDepthTexture(
  vtkRenderer* ren, vtkVolume* vtkNotUsed(vol))
{
  // Make sure our render window is the current OpenGL context
  ren->GetRenderWindow()->MakeCurrent();

  // Load required extensions for grabbing depth sampler buffer
  if (!this->LoadDepthTextureExtensionsSucceeded)
    {
    this->LoadRequireDepthTextureExtensions(ren->GetRenderWindow());
    }

  // If we can't load the necessary extensions, provide
  // feedback on why it failed.
  if(!this->LoadDepthTextureExtensionsSucceeded)
    {
    std::cerr << this->ExtensionsStringStream.str() << std::endl;
    return;
    }

  // Now grab the depth sampler buffer as texture
  ren->GetTiledSizeAndOrigin(this->WindowSize, this->WindowSize + 1,
                             this->WindowLowerLeft, this->WindowLowerLeft + 1);

  glActiveTexture(GL_TEXTURE4);
  if (!this->DepthTextureId)
    {
    // TODO Use framebuffer objects for best performance
    glGenTextures(1, &this->DepthTextureId);
    glBindTexture(GL_TEXTURE_2D, this->DepthTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
    }
  glBindTexture(GL_TEXTURE_2D, this->DepthTextureId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
               this->WindowSize[0], this->WindowSize[1],
               0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0 , 0,
                      this->WindowLowerLeft[0], this->WindowLowerLeft[1],
                      this->WindowSize[0], this->WindowSize[1]);
  glActiveTexture(GL_TEXTURE0);
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::IsCameraInside(
  vtkRenderer* ren, vtkVolume* vtkNotUsed(vol))
{
  vtkNew<vtkMatrix4x4> tempMat;

  vtkMatrix4x4::Transpose(this->InverseVolumeMat.GetPointer(),
                          tempMat.GetPointer());

  vtkCamera* cam = ren->GetActiveCamera();
  double camWorldRange[2];
  double camWorldPos[4];
  double camFocalWorldPoint[4];
  double camWorldDirection[4];
  double camPos[4];
  double camPlaneNormal[4];

  cam->GetPosition(camWorldPos);
  camWorldPos[3] = 1.0;
  this->InverseVolumeMat->MultiplyPoint( camWorldPos, camPos );
  if ( camPos[3] )
    {
    camPos[0] /= camPos[3];
    camPos[1] /= camPos[3];
    camPos[2] /= camPos[3];
    }

  cam->GetFocalPoint(camFocalWorldPoint);
  camFocalWorldPoint[3]=1.0;

  // The range (near/far) must also be transformed
  // into the local coordinate system.
  camWorldDirection[0] = camFocalWorldPoint[0] - camWorldPos[0];
  camWorldDirection[1] = camFocalWorldPoint[1] - camWorldPos[1];
  camWorldDirection[2] = camFocalWorldPoint[2] - camWorldPos[2];
  camWorldDirection[3] = 1.0;

  // Compute the normalized near plane normal
  tempMat->MultiplyPoint( camWorldDirection, camPlaneNormal );

  vtkMath::Normalize(camWorldDirection);
  vtkMath::Normalize(camPlaneNormal);

  double camNearWorldPoint[4];
  double camNearPoint[4];

  cam->GetClippingRange(camWorldRange);
  camNearWorldPoint[0] = camWorldPos[0] + camWorldRange[0]*camWorldDirection[0];
  camNearWorldPoint[1] = camWorldPos[1] + camWorldRange[0]*camWorldDirection[1];
  camNearWorldPoint[2] = camWorldPos[2] + camWorldRange[0]*camWorldDirection[2];
  camNearWorldPoint[3] = 1.;

  this->InverseVolumeMat->MultiplyPoint( camNearWorldPoint, camNearPoint );
  if (camNearPoint[3]!=0.0)
    {
    camNearPoint[0] /= camNearPoint[3];
    camNearPoint[1] /= camNearPoint[3];
    camNearPoint[2] /= camNearPoint[3];
    }

  double tolerance[3] = { 1e-12, 1e-12, 1e-12 };
  if (vtkMath::PointIsWithinBounds(camNearPoint, this->LoadedBounds, tolerance))
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateVolumeGeometry(
  vtkRenderer* ren, vtkVolume* vol, vtkImageData* input)
{
  if (input != this->PrevInput || this->IsCameraInside(ren, vol) ||
      this->CameraWasInsideInLastUpdate)
    {
    vtkNew<vtkTessellatedBoxSource> boxSource;
    boxSource->SetBounds(this->LoadedBounds);
    boxSource->QuadsOn();
    boxSource->SetLevel(0);

    vtkNew<vtkDensifyPolyData> densityPolyData;

    if (input == this->PrevInput && this->IsCameraInside(ren, vol))
      {
      // Normals should be transformed using the transpose of inverse
      // InverseVolumeMat
      vtkNew<vtkMatrix4x4> tempMat;
      vtkMatrix4x4::Transpose(this->InverseVolumeMat.GetPointer(),
                              tempMat.GetPointer());

      vtkCamera* cam = ren->GetActiveCamera();
      double camWorldRange[2];
      double camWorldPos[4];
      double camFocalWorldPoint[4];
      double camWorldDirection[4];
      double camPos[4];
      double camPlaneNormal[4];

      cam->GetPosition(camWorldPos);
      camWorldPos[3] = 1.0;
      this->InverseVolumeMat->MultiplyPoint( camWorldPos, camPos );
      if ( camPos[3] )
        {
        camPos[0] /= camPos[3];
        camPos[1] /= camPos[3];
        camPos[2] /= camPos[3];
        }

      cam->GetFocalPoint(camFocalWorldPoint);
      camFocalWorldPoint[3]=1.0;

      // The range (near/far) must also be transformed
      // into the local coordinate system.
      camWorldDirection[0] = camFocalWorldPoint[0] - camWorldPos[0];
      camWorldDirection[1] = camFocalWorldPoint[1] - camWorldPos[1];
      camWorldDirection[2] = camFocalWorldPoint[2] - camWorldPos[2];
      camWorldDirection[3] = 1.0;

      // Compute the normalized near plane normal
      tempMat->MultiplyPoint( camWorldDirection, camPlaneNormal );

      vtkMath::Normalize(camWorldDirection);
      vtkMath::Normalize(camPlaneNormal);

      double camNearWorldPoint[4];
      double camFarWorldPoint[4];
      double camNearPoint[4];
      double camFarPoint[4];

      cam->GetClippingRange(camWorldRange);
      camNearWorldPoint[0] = camWorldPos[0] + camWorldRange[0]*camWorldDirection[0];
      camNearWorldPoint[1] = camWorldPos[1] + camWorldRange[0]*camWorldDirection[1];
      camNearWorldPoint[2] = camWorldPos[2] + camWorldRange[0]*camWorldDirection[2];
      camNearWorldPoint[3] = 1.;

      camFarWorldPoint[0] = camWorldPos[0] + camWorldRange[1]*camWorldDirection[0];
      camFarWorldPoint[1] = camWorldPos[1] + camWorldRange[1]*camWorldDirection[1];
      camFarWorldPoint[2] = camWorldPos[2] + camWorldRange[1]*camWorldDirection[2];
      camFarWorldPoint[3] = 1.;

      this->InverseVolumeMat->MultiplyPoint( camNearWorldPoint, camNearPoint );
      if (camNearPoint[3]!=0.0)
        {
        camNearPoint[0] /= camNearPoint[3];
        camNearPoint[1] /= camNearPoint[3];
        camNearPoint[2] /= camNearPoint[3];
        }

      this->InverseVolumeMat->MultiplyPoint( camFarWorldPoint, camFarPoint );
      if (camFarPoint[3]!=0.0)
        {
        camFarPoint[0] /= camFarPoint[3];
        camFarPoint[1] /= camFarPoint[3];
        camFarPoint[2] /= camFarPoint[3];
        }

      vtkNew<vtkPlane> nearPlane;

      // We add an offset to the near plane to avoid hardware clipping of the
      // near plane due to floating-point precision.
      // camPlaneNormal is a unit vector, if the offset is larger than the
      // distance between near and far point, it will not work, in this case we
      // pick a fraction of the near-far distance.
      // 100.0 and 1000.0 are chosen based on the typical epsilon values on
      // x86 systems.
      double offset =  static_cast<double>(
                         std::numeric_limits<float>::epsilon()) * 100.0;
      if(offset > 0.001)
        {
        double newOffset = sqrt(vtkMath::Distance2BetweenPoints(
                             camNearPoint, camFarPoint)) / 1000.0;
        offset = offset > newOffset ? newOffset : offset;
        }

      camNearPoint[0] += camPlaneNormal[0]*offset;
      camNearPoint[1] += camPlaneNormal[1]*offset;
      camNearPoint[2] += camPlaneNormal[2]*offset;

      nearPlane->SetOrigin( camNearPoint );
      nearPlane->SetNormal( camPlaneNormal );

      vtkNew<vtkPlaneCollection> planes;
      planes->RemoveAllItems();
      planes->AddItem(nearPlane.GetPointer());

      vtkNew<vtkClipConvexPolyData> clip;
      clip->SetInputConnection(boxSource->GetOutputPort());
      clip->SetPlanes(planes.GetPointer());

      densityPolyData->SetInputConnection(clip->GetOutputPort());

      this->CameraWasInsideInLastUpdate = true;
      }
    else
      {
      densityPolyData->SetInputConnection(boxSource->GetOutputPort());
      this->CameraWasInsideInLastUpdate = false;
      }

    densityPolyData->SetNumberOfSubdivisions(2);
    densityPolyData->Update();

    this->BBoxPolyData = vtkSmartPointer<vtkPolyData>::New();
    this->BBoxPolyData->ShallowCopy(densityPolyData->GetOutput());
    vtkPoints* points = this->BBoxPolyData->GetPoints();
    vtkCellArray* cells = this->BBoxPolyData->GetPolys();

    vtkNew<vtkUnsignedIntArray> polys;
    polys->SetNumberOfComponents(3);
    vtkIdType npts;
    vtkIdType *pts;

    while(cells->GetNextCell(npts, pts))
      {
      polys->InsertNextTuple3(pts[0], pts[1], pts[2]);
      }

    // Dispose any previously created buffers
    this->DeleteBufferObjects();

    // Now create new ones
    this->CreateBufferObjects();

#ifndef __APPLE__
    glBindVertexArray(this->CubeVAOId);
#endif
    // Pass cube vertices to buffer object memory
    glBindBuffer (GL_ARRAY_BUFFER, this->CubeVBOId);
    glBufferData (GL_ARRAY_BUFFER, points->GetData()->GetDataSize() *
                  points->GetData()->GetDataTypeSize(),
                  points->GetData()->GetVoidPointer(0), GL_STATIC_DRAW);

    // Enable vertex attributre array for position
    // and pass indices to element array  buffer
    glEnableVertexAttribArray(this->Shader["m_in_vertex_pos"]);
    glVertexAttribPointer(this->Shader["m_in_vertex_pos"],
                          3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, polys->GetDataSize() *
                  polys->GetDataTypeSize(), polys->GetVoidPointer(0),
                  GL_STATIC_DRAW);
    }
  else
    {
#ifndef __APPLE__
    glBindVertexArray(this->CubeVAOId);
#else
    glBindBuffer (GL_ARRAY_BUFFER, this->CubeVBOId);
    glEnableVertexAttribArray(this->Shader["m_in_vertex_pos"]);
    glVertexAttribPointer(this->Shader["m_in_vertex_pos"],
                          3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
#endif
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateCropping(
  vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol))
{
  if (this->Parent->GetCropping())
    {
    int cropFlags = this->Parent->GetCroppingRegionFlags();
    double croppingRegionPlanes[6];
    this->Parent->GetCroppingRegionPlanes(croppingRegionPlanes);

    // Clamp it
    croppingRegionPlanes[0] = croppingRegionPlanes[0] < this->LoadedBounds[0] ?
                              this->LoadedBounds[0] : croppingRegionPlanes[0];
    croppingRegionPlanes[0] = croppingRegionPlanes[0] > this->LoadedBounds[1] ?
                              this->LoadedBounds[1] : croppingRegionPlanes[0];
    croppingRegionPlanes[1] = croppingRegionPlanes[1] < this->LoadedBounds[0] ?
                              this->LoadedBounds[0] : croppingRegionPlanes[1];
    croppingRegionPlanes[1] = croppingRegionPlanes[1] > this->LoadedBounds[1] ?
                              this->LoadedBounds[1] : croppingRegionPlanes[1];

    croppingRegionPlanes[2] = croppingRegionPlanes[2] < this->LoadedBounds[2] ?
                              this->LoadedBounds[2] : croppingRegionPlanes[2];
    croppingRegionPlanes[2] = croppingRegionPlanes[2] > this->LoadedBounds[3] ?
                              this->LoadedBounds[3] : croppingRegionPlanes[2];
    croppingRegionPlanes[3] = croppingRegionPlanes[3] < this->LoadedBounds[2] ?
                              this->LoadedBounds[2] : croppingRegionPlanes[3];
    croppingRegionPlanes[3] = croppingRegionPlanes[3] > this->LoadedBounds[3] ?
                              this->LoadedBounds[3] : croppingRegionPlanes[3];

    croppingRegionPlanes[4] = croppingRegionPlanes[4] < this->LoadedBounds[4] ?
                              this->LoadedBounds[4] : croppingRegionPlanes[4];
    croppingRegionPlanes[4] = croppingRegionPlanes[4] > this->LoadedBounds[5] ?
                              this->LoadedBounds[5] : croppingRegionPlanes[4];
    croppingRegionPlanes[5] = croppingRegionPlanes[5] < this->LoadedBounds[4] ?
                              this->LoadedBounds[4] : croppingRegionPlanes[5];
    croppingRegionPlanes[5] = croppingRegionPlanes[5] > this->LoadedBounds[5] ?
                              this->LoadedBounds[5] : croppingRegionPlanes[5];

    float cropPlanes[6] = { static_cast<float>(croppingRegionPlanes[0]),
                            static_cast<float>(croppingRegionPlanes[1]),
                            static_cast<float>(croppingRegionPlanes[2]),
                            static_cast<float>(croppingRegionPlanes[3]),
                            static_cast<float>(croppingRegionPlanes[4]),
                            static_cast<float>(croppingRegionPlanes[5]) };

    glUniform1fv(this->Shader("cropping_planes"), 6, cropPlanes);
    const int numberOfRegions = 32;
    int cropFlagsArray[numberOfRegions];
    cropFlagsArray[0] = 0;
    int i = 1;
    while(cropFlags && i < 32)
      {
      cropFlagsArray[i] = cropFlags & 1;
      cropFlags = cropFlags >> 1;
      ++i;
      }
    for (; i < 32; ++i)
      {
      cropFlagsArray[i] = 0;
      }

    glUniform1iv(this->Shader("cropping_flags"), numberOfRegions,
                 cropFlagsArray);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateClipping(
  vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol))
{
  if (this->Parent->GetClippingPlanes())
    {
    std::vector<float> clippingPlanes;
    // Currently we don't have any clipping plane
    clippingPlanes.push_back(0);

    this->Parent->ClippingPlanes->InitTraversal();
    vtkPlane* plane;
    while ((plane = this->Parent->ClippingPlanes->GetNextItem()))
      {
      // Planes are in world coordinates
      double planeOrigin[3], planeNormal[3];
      plane->GetOrigin(planeOrigin);
      plane->GetNormal(planeNormal);

      clippingPlanes.push_back(planeOrigin[0]);
      clippingPlanes.push_back(planeOrigin[1]);
      clippingPlanes.push_back(planeOrigin[2]);
      clippingPlanes.push_back(planeNormal[0]);
      clippingPlanes.push_back(planeNormal[1]);
      clippingPlanes.push_back(planeNormal[2]);
      }

    double croppingRegionPlanes[6];
    this->Parent->GetCroppingRegionPlanes(croppingRegionPlanes);

    clippingPlanes[0] = clippingPlanes.size() > 0 ?
      (clippingPlanes.size() - 1) : 0;

    glUniform1fv(this->Shader("m_clipping_planes"),
                 static_cast<int>(clippingPlanes.size()),
                 &clippingPlanes[0]);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateSamplingDistance(
  vtkImageData* input, vtkRenderer* vtkNotUsed(ren), vtkVolume* vol)
{
  if(!this->Parent->AutoAdjustSampleDistances)
    {
    this->ActualSampleDistance = this->Parent->SampleDistance;
    }
  else
    {
    input->GetSpacing(this->CellSpacing);

    vtkMatrix4x4* worldToDataset = vol->GetMatrix();
    double minWorldSpacing = VTK_DOUBLE_MAX;
    int i = 0;
    while (i < 3)
      {
      double tmp = worldToDataset->GetElement(0,i);
      double tmp2 = tmp * tmp;
      tmp = worldToDataset->GetElement(1,i);
      tmp2 += tmp * tmp;
      tmp = worldToDataset->GetElement(2,i);
      tmp2 += tmp * tmp;

      // We use fabs() in case the spacing is negative.
      double worldSpacing = fabs(this->CellSpacing[i] * sqrt(tmp2));
      if(worldSpacing < minWorldSpacing)
        {
        minWorldSpacing = worldSpacing;
        }
      ++i;
      }

    // minWorldSpacing is the optimal sample distance in world space.
    // To go faster (reduceFactor<1.0), we multiply this distance
    // by 1/reduceFactor.
    this->ActualSampleDistance = static_cast<float>(minWorldSpacing);

    // TODO: Support reduction factor
    //    if (this->ReductionFactor < 1.0)
    //      {
    //      this->ActualSampleDistance /=
    //      static_cast<GLfloat>(this->ReductionFactor*0.5);
    //      }
    //    }
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::
  LoadRequireDepthTextureExtensions(vtkRenderWindow* vtkNotUsed(renWin))
{
  // Reset the message stream for extensions
  this->ExtensionsStringStream.str("");
  this->ExtensionsStringStream.clear();

  if (!GLEW_VERSION_2_0)
    {
    this->ExtensionsStringStream << "Requires OpenGL 2.0 or higher";
    return;
    }

  // Check for npot even though it should be supported since
  // it is in core since 2.0 as per specification
  if (!glewIsSupported("GL_ARB_texture_non_power_of_two"))
    {
    this->ExtensionsStringStream << "Required extension "
      << " GL_ARB_texture_non_power_of_two is not supported";
    return;
    }

  // Check for float texture support. This extension became core
  // in 3.0
  if (!glewIsSupported("GL_ARB_texture_float"))
    {
    this->ExtensionsStringStream << "Required extension "
      << " GL_ARB_texture_float is not supported";
    return;
    }

  // Check for framebuffer objects. Framebuffer objects
  // are core since version 3.0 only
  if (!glewIsSupported("GL_EXT_framebuffer_object"))
    {
    this->ExtensionsStringStream << "Required extension "
      << " GL_EXT_framebuffer_object is not supported";
    return;
    }

  // NOTE: Support for depth sampler texture made into the core since version
  // 1.4 and therefore we are no longer checking for it.
  this->LoadDepthTextureExtensionsSucceeded = true;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CreateBufferObjects()
{
#ifndef __APPLE__
  glGenVertexArrays(1, &this->CubeVAOId);
#endif
  glGenBuffers(1, &this->CubeVBOId);
  glGenBuffers(1, &this->CubeIndicesId);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::DeleteBufferObjects()
{
#ifndef __APPLE__
  if (this->CubeVAOId)
    {
    glDeleteVertexArrays(1, &this->CubeVAOId);
    }
#endif

  if (this->CubeVBOId)
    {
    glDeleteBuffers(1, &this->CubeVBOId);
    }

  if (this->CubeIndicesId)
   {
   glDeleteBuffers(1, &this->CubeIndicesId);
   }
}

//----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::vtkOpenGLGPUVolumeRayCastMapper() :
  vtkGPUVolumeRayCastMapper()
{
  this->Impl = new vtkInternal(this);
}

///
//----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::~vtkOpenGLGPUVolumeRayCastMapper()
{
  delete this->Impl;
  this->Impl = 0;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dimensions: " << this->Impl->Dimensions[0] << ", " <<
                                    this->Impl->Dimensions[1] << ", " <<
                                    this->Impl->Dimensions[2] << ", " << "\n";
  os << indent << "Bounds: " << this->Impl->LoadedBounds[0] << ", " <<
                                this->Impl->LoadedBounds[1] << ", " <<
                                this->Impl->LoadedBounds[2] << "\n";
  os << indent << "ActualSampleDistance: " <<
    this->Impl->ActualSampleDistance << "\n";
  os << indent << "LastProjectionParallel: " <<
    this->Impl->LastProjectionParallel << "\n";
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReleaseGraphicsResources(
  vtkWindow *window)
{
  this->Impl->DeleteBufferObjects();

  if(this->Impl->VolumeTextureId)
    {
    window->MakeCurrent();
    GLuint volumeTextureObject = static_cast<GLuint>(
                                   this->Impl->VolumeTextureId);
    glDeleteTextures(1, &volumeTextureObject);
    this->Impl->VolumeTextureId = 0;
    }

  if(this->Impl->NoiseTextureId)
    {
    window->MakeCurrent();
    GLuint noiseTextureObject = static_cast<GLuint>(
                                  this->Impl->NoiseTextureId);
    glDeleteTextures(1, &noiseTextureObject);
    this->Impl->NoiseTextureId = 0;
    }

  if(this->Impl->DepthTextureId)
    {
    window->MakeCurrent();
    GLuint depthTextureObject = static_cast<GLuint>(
                                  this->Impl->DepthTextureId);
    glDeleteTextures(1, &depthTextureObject);
    this->Impl->DepthTextureId = 0;
    }

  if(this->Impl->MaskTextures != 0)
    {
    if(!this->Impl->MaskTextures->Map.empty())
      {
      std::map<vtkImageData*, vtkVolumeMask*>::iterator it =
        this->Impl->MaskTextures->Map.begin();
      while(it != this->Impl->MaskTextures->Map.end())
        {
        vtkVolumeMask* texture = (*it).second;
        delete texture;
        ++it;
        }
      this->Impl->MaskTextures->Map.clear();
      }
    }

  if(this->Impl->RGBTable != 0)
    {
    delete this->Impl->RGBTable;
    this->Impl->RGBTable = 0;
    }

  if(this->Impl->Mask1RGBTable != 0)
    {
    delete this->Impl->Mask1RGBTable;
    this->Impl->Mask1RGBTable = 0;
    }

  if(this->Impl->Mask2RGBTable != 0)
    {
    delete this->Impl->Mask2RGBTable;
    this->Impl->Mask2RGBTable = 0;
    }

  if(this->Impl->OpacityTables != 0)
    {
    delete this->Impl->OpacityTables;
    this->Impl->OpacityTables = 0;
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::BuildShader(vtkRenderer* ren,
                                                  vtkVolume* vol,
                                                  int noOfComponents)
{
  vtkVolumeProperty* volProperty = vol->GetProperty();
  this->Impl->Shader.DeleteShaderProgram();

  std::string vertexShader (raycastervs);
  std::string fragmentShader (raycasterfs);

  vertexShader = vtkvolume::replace(vertexShader, "//VTK::ComputeClipPos::Impl",
    vtkvolume::ComputeClip(ren, this, vol), true);
  vertexShader = vtkvolume::replace(vertexShader, "//VTK::ComputeTextureCoords::Impl",
    vtkvolume::ComputeTextureCoords(ren, this, vol), true);

  vertexShader = vtkvolume::replace(vertexShader, "//VTK::Base::Dec",
    vtkvolume::BaseGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Base::Dec",
    vtkvolume::BaseGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Base::Init",
    vtkvolume::BaseInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Base::Impl",
    vtkvolume::BaseIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Base::Exit",
    vtkvolume::BaseExit(ren, this, vol), true);

  vertexShader = vtkvolume::replace(vertexShader, "//VTK::Termination::Dec",
    vtkvolume::TerminationGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Termination::Dec",
    vtkvolume::TerminationGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Terminate::Init",
    vtkvolume::TerminationInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Terminate::Impl",
    vtkvolume::TerminationIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Terminate::Exit",
    vtkvolume::TerminationExit(ren, this, vol), true);

  vertexShader = vtkvolume::replace(vertexShader, "//VTK::Shading::Dec",
    vtkvolume::ShadingGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Shading::Dec",
    vtkvolume::ShadingGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Shading::Init",
    vtkvolume::ShadingInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Shading::Impl",
    vtkvolume::ShadingIncrement(ren, this, vol, this->MaskInput,
                                this->Impl->CurrentMask,
                                this->MaskType), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Shading::Exit",
    vtkvolume::ShadingExit(ren, this, vol), true);

  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::ComputeOpacity::Dec",
    vtkvolume::OpacityTransferFunc(ren, this, vol, noOfComponents), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::ComputeGradient::Dec",
    vtkvolume::GradientsComputeFunc(ren, this, vol, noOfComponents), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::ColorTransferFunc::Dec",
     vtkvolume::ColorTransferFunc(ren, this, vol, noOfComponents), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::ComputeLighting::Dec",
    vtkvolume::LightComputeFunc(ren, this, vol,noOfComponents), true);
  fragmentShader = vtkvolume::replace(fragmentShader,
                                      "//VTK::RayDirectionFunc::Dec",
    vtkvolume::RayDirectionFunc(ren, this, vol,noOfComponents), true);

  vertexShader = vtkvolume::replace(vertexShader, "//VTK::Cropping::Dec",
    vtkvolume::CroppingGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Cropping::Dec",
    vtkvolume::CroppingGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Cropping::Init",
    vtkvolume::CroppingInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Cropping::Impl",
    vtkvolume::CroppingIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Cropping::Exit",
    vtkvolume::CroppingExit(ren, this, vol), true);

  vertexShader = vtkvolume::replace(vertexShader, "//VTK::Clipping::Dec",
    vtkvolume::ClippingGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Clipping::Dec",
    vtkvolume::ClippingGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Clipping::Init",
    vtkvolume::ClippingInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Clipping::Impl",
    vtkvolume::ClippingIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::Clipping::Exit",
    vtkvolume::ClippingExit(ren, this, vol), true);

  fragmentShader = vtkvolume::replace(fragmentShader,
                                      "//VTK::BinaryMask::Dec",
    vtkvolume::BinaryMaskGlobalsFrag(ren, this, vol, this->MaskInput,
                                     this->Impl->CurrentMask,
                                     this->MaskType), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "//VTK::BinaryMask::Impl",
    vtkvolume::BinaryMaskIncrement(ren, this, vol, this->MaskInput,
                                   this->Impl->CurrentMask,
                                   this->MaskType), true);

  fragmentShader = vtkvolume::replace(fragmentShader,
                                      "//VTK::CompositeMask::Dec",
    vtkvolume::CompositeMaskGlobalsFrag(ren, this, vol, this->MaskInput,
                                        this->Impl->CurrentMask,
                                        this->MaskType), true);
  fragmentShader = vtkvolume::replace(fragmentShader,
                                      "//VTK::CompositeMask::Impl",
    vtkvolume::CompositeMaskIncrement(ren, this, vol, this->MaskInput,
                                      this->Impl->CurrentMask,
                                      this->MaskType), true);

  // Compile and link it
  this->Impl->CompileAndLinkShader(vertexShader, fragmentShader);

  // Add attributes and uniforms
  this->Impl->Shader.AddAttribute("m_in_vertex_pos");

  this->Impl->Shader.AddUniform("m_volume_matrix");
  this->Impl->Shader.AddUniform("m_inverse_volume_matrix");
  this->Impl->Shader.AddUniform("m_modelview_matrix");
  this->Impl->Shader.AddUniform("m_inverse_modelview_matrix");
  this->Impl->Shader.AddUniform("m_projection_matrix");
  this->Impl->Shader.AddUniform("m_inverse_projection_matrix");
  this->Impl->Shader.AddUniform("m_texture_dataset_matrix");
  this->Impl->Shader.AddUniform("m_inverse_texture_dataset_matrix");
  this->Impl->Shader.AddUniform("m_volume");
  this->Impl->Shader.AddUniform("m_camera_pos");
  this->Impl->Shader.AddUniform("m_light_pos");
  this->Impl->Shader.AddUniform("m_cell_step");
  this->Impl->Shader.AddUniform("m_cell_scale");
  this->Impl->Shader.AddUniform("m_cell_spacing");
  this->Impl->Shader.AddUniform("m_sample_distance");
  this->Impl->Shader.AddUniform("m_scalars_range");

  if (noOfComponents == 1 &&
      this->BlendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
    {
    this->Impl->Shader.AddUniform("m_color_transfer_func");
    }

  this->Impl->Shader.AddUniform("m_opacity_transfer_func");

  if (volProperty->GetGradientOpacity())
    {
    this->Impl->Shader.AddUniform("m_gradient_transfer_func");
    }

  this->Impl->Shader.AddUniform("m_noise_sampler");
  this->Impl->Shader.AddUniform("m_depth_sampler");
  this->Impl->Shader.AddUniform("m_vol_extents_min");
  this->Impl->Shader.AddUniform("m_vol_extents_max");
  this->Impl->Shader.AddUniform("m_texture_extents_min");
  this->Impl->Shader.AddUniform("m_texture_extents_max");
  this->Impl->Shader.AddUniform("m_ambient");
  this->Impl->Shader.AddUniform("m_diffuse");
  this->Impl->Shader.AddUniform("m_specular");
  this->Impl->Shader.AddUniform("m_shininess");
  this->Impl->Shader.AddUniform("m_window_lower_left_corner");
  this->Impl->Shader.AddUniform("m_inv_original_window_size");
  this->Impl->Shader.AddUniform("m_inv_window_size");

  if (this->GetCropping())
    {
    this->Impl->Shader.AddUniform("cropping_planes");
    this->Impl->Shader.AddUniform("cropping_flags");
    }

  if (this->GetClippingPlanes())
    {
    this->Impl->Shader.AddUniform("m_clipping_planes");
    this->Impl->Shader.AddUniform("m_clipping_planes_size");
    }

  if (this->Impl->CurrentMask)
    {
    this->Impl->Shader.AddUniform("m_mask");
    }

  if (this->MaskInput && this->MaskType == LabelMapMaskType)
    {
      this->Impl->Shader.AddUniform("m_mask_1");
      this->Impl->Shader.AddUniform("m_mask_2");
      this->Impl->Shader.AddUniform("m_mask_blendfactor");
    }

  if (ren->GetActiveCamera()->GetParallelProjection())
    {
    this->Impl->Shader.AddUniform("m_projection_direction");
    }

  this->Impl->ShaderBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GPURender(vtkRenderer* ren,
                                                vtkVolume* vol)
{
  // Make sure the context is current
  ren->GetRenderWindow()->MakeCurrent();

  // Update m_volume first to make sure states are current
  vol->Update();

  vtkImageData* input = this->GetTransformedInput();

  // Set OpenGL states
  vtkVolumeStateRAII glState;

  if (!this->Impl->IsInitialized())
    {
    this->Impl->Initialize(ren, vol);
    }

  vtkDataArray* scalars = this->GetScalars(input,
                          this->ScalarMode,
                          this->ArrayAccessMode,
                          this->ArrayId,
                          this->ArrayName,
                          this->CellFlag);

  // How many components are there?
  int numberOfScalarComponents = scalars->GetNumberOfComponents();

  // If it is just one, then get the range from the scalars
  if(numberOfScalarComponents == 1)
    {
    // NOTE: here, we ignore the blank cells.
    scalars->GetRange(this->Impl->ScalarsRange);
    }
  // If it is 3, then use the 4th component's range since that is
  // the component that will be passed through the scalar opacity
  // transfer function to look up opacity
  else
    {
    // Note that we've already checked data type and we know this is
    // unsigned char
    scalars->GetRange(this->Impl->ScalarsRange, 3);
    }

  // Invert the volume matrix
  // Will require transpose of this matrix for OpenGL
  // Scene matrix
  this->Impl->InverseVolumeMat->DeepCopy(vol->GetMatrix());
  this->Impl->InverseVolumeMat->Invert();

  // Update the volume if needed
  if (input != this->Impl->PrevInput)
    {
    input->GetDimensions(this->Impl->Dimensions);

    // Update bounds, data, and geometry
    this->Impl->ComputeBounds(input);
    this->Impl->LoadVolume(input, scalars);
    this->Impl->LoadMask(input, this->MaskInput,
                         this->Impl->Extents, vol);
    }

  this->Impl->UpdateVolumeGeometry(ren, vol, input);

  // Mask
  vtkVolumeMask* mask = 0;
  if(this->MaskInput != 0)
    {
    std::map<vtkImageData *,vtkVolumeMask*>::iterator it2 =
      this->Impl->MaskTextures->Map.find(this->MaskInput);
    if(it2 == this->Impl->MaskTextures->Map.end())
      {
      mask=0;
      }
    else
      {
      mask=(*it2).second;
      }
    }
  this->Impl->CurrentMask = mask;
  if (this->Impl->CurrentMask != 0)
   {
   this->Impl->CurrentMask->Bind();
   }

  this->Impl->UpdateSamplingDistance(input, ren, vol);

  // Build shader
  if (vol->GetProperty()->GetMTime() >
      this->Impl->ShaderBuildTime.GetMTime() ||
      this->GetMTime() > this->Impl->ShaderBuildTime.GetMTime() ||
      ren->GetActiveCamera()->GetParallelProjection() !=
      this->Impl->LastProjectionParallel
      )
    {
    this->Impl->LastProjectionParallel =
      ren->GetActiveCamera()->GetParallelProjection();
    this->BuildShader(ren, vol, numberOfScalarComponents);
    }

  // Update opacity transfer function
  // TODO Passing level 0 for now
  this->Impl->UpdateOpacityTransferFunction(vol,
    scalars->GetNumberOfComponents(), 0);

  this->Impl->UpdateGradientOpacityTransferFunction(vol,
    scalars->GetNumberOfComponents(), 0);

  // Update transfer color functions
  this->Impl->UpdateColorTransferFunction(vol,
    scalars->GetNumberOfComponents());

  // Update noise sampler texture
  this->Impl->UpdateNoiseTexture();

  // Grab depth sampler buffer (to handle cases when we are rendering geometry
  // and m_volume together
  this->Impl->UpdateDepthTexture(ren, vol);

  // Temporary variables
  float fvalue2[2];
  float fvalue3[3];
  float fvalue16[16];

  // Update sampling distance
  int* loadedExtent = input->GetExtent();

  this->Impl->CellStep[0] =
    (1.0/static_cast<double>(loadedExtent[1] - loadedExtent[0]));
  this->Impl->CellStep[1] =
    (1.0/static_cast<double>(loadedExtent[3] - loadedExtent[2]));
  this->Impl->CellStep[2] =
    (1.0/static_cast<double>(loadedExtent[5] -loadedExtent[4]));

  this->Impl->CellScale[0] = (this->Impl->LoadedBounds[1] -
                              this->Impl->LoadedBounds[0]) * 0.5;
  this->Impl->CellScale[1] = (this->Impl->LoadedBounds[3] -
                              this->Impl->LoadedBounds[2]) * 0.5;
  this->Impl->CellScale[2] = (this->Impl->LoadedBounds[5] -
                              this->Impl->LoadedBounds[4]) * 0.5;

  this->Impl->DatasetStepSize[0] = 1.0 / (this->Impl->LoadedBounds[1] -
                                          this->Impl->LoadedBounds[0]);
  this->Impl->DatasetStepSize[1] = 1.0 / (this->Impl->LoadedBounds[3] -
                                          this->Impl->LoadedBounds[2]);
  this->Impl->DatasetStepSize[2] = 1.0 / (this->Impl->LoadedBounds[5] -
                                          this->Impl->LoadedBounds[4]);

  // Now use the shader
  this->Impl->Shader.Use();

  if (ren->GetActiveCamera()->GetParallelProjection())
    {
    double dir[4];
    ren->GetActiveCamera()->GetDirectionOfProjection(dir);
    vtkInternal::ToFloat(dir[0], dir[1], dir[2], fvalue3);
    glUniform3f(this->Impl->Shader("m_projection_direction"),
                fvalue3[0], fvalue3[1], fvalue3[2]);
    }

  // Pass constant uniforms at initialization
  // Step should be dependant on the bounds and not on the texture size
  // since we can have non uniform voxel size / spacing / aspect ratio
  vtkInternal::ToFloat(this->Impl->CellStep, fvalue3);
  glUniform3f(this->Impl->Shader("m_cell_step"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  vtkInternal::ToFloat(this->Impl->CellScale, fvalue3);
  glUniform3f(this->Impl->Shader("m_cell_scale"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  vtkInternal::ToFloat(this->Impl->CellSpacing, fvalue3);
  glUniform3f(this->Impl->Shader("m_cell_spacing"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  glUniform1f(this->Impl->Shader("m_sample_distance"),
              this->Impl->ActualSampleDistance);

  vtkInternal::ToFloat(this->Impl->ScalarsRange, fvalue2);
  glUniform2f(this->Impl->Shader("m_scalars_range"),
              fvalue2[0], fvalue2[1]);

  glUniform1i(this->Impl->Shader("m_volume"), 0);
  glUniform1i(this->Impl->Shader("m_opacity_transfer_func"), 2);
  glUniform1i(this->Impl->Shader("m_noise_sampler"), 3);
  glUniform1i(this->Impl->Shader("m_depth_sampler"), 4);
  glUniform1i(this->Impl->Shader("m_gradient_transfer_func"), 5);

  if (this->Impl->CurrentMask)
    {
    glUniform1i(this->Impl->Shader("m_mask"), 6);
    }

  if(numberOfScalarComponents == 1 &&
     this->BlendMode!=vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
    {
    glUniform1i(this->Impl->Shader("m_color_transfer_func"), 1);

    if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
      {
      glUniform1i(this->Impl->Shader("m_mask_1"), 7);
      glUniform1i(this->Impl->Shader("m_mask_2"), 8);
      glUniform1f(this->Impl->Shader("m_mask_blendfactor"),
                  this->MaskBlendFactor);
      }
    }

  fvalue3[0] = fvalue3[1] = fvalue3[2] = vol->GetProperty()->GetAmbient();
  glUniform3f(this->Impl->Shader("m_ambient"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  fvalue3[0] = fvalue3[1] = fvalue3[2] = vol->GetProperty()->GetDiffuse();
  glUniform3f(this->Impl->Shader("m_diffuse"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  fvalue3[0] = fvalue3[1] = fvalue3[2] = vol->GetProperty()->GetSpecular();
  glUniform3f(this->Impl->Shader("m_specular"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  fvalue3[0] = vol->GetProperty()->GetSpecularPower();
  glUniform1f(this->Impl->Shader("m_shininess"), fvalue3[0]);

  // Bind textures
  // Volume texture is at unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, this->Impl->VolumeTextureId);

  // Color texture is at unit 1
  if (numberOfScalarComponents == 1)
    {
    this->Impl->RGBTable->Bind();

    if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
      {
      this->Impl->Mask1RGBTable->Bind(7);
      this->Impl->Mask2RGBTable->Bind(8);
      }
    }

  // Opacity texture is at unit 2
  // TODO Supports only one table for now
  this->Impl->OpacityTables->GetTable(0)->Bind();

  // Noise texture is at unit 3
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, this->Impl->NoiseTextureId);

  // Depth texture is at unit 4
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, this->Impl->DepthTextureId);

  // Look at the OpenGL Camera for the exact aspect computation
  double aspect[2];
  ren->ComputeAspect();
  ren->GetAspect(aspect);

  double clippingRange[2];
  ren->GetActiveCamera()->GetClippingRange(clippingRange);

  // Will require transpose of this matrix for OpenGL
  vtkMatrix4x4* projectionMat4x4 = ren->GetActiveCamera()->
    GetProjectionTransformMatrix(aspect[0]/aspect[1], -1, 1);
  this->Impl->InverseProjectionMat->DeepCopy(projectionMat4x4);
  this->Impl->InverseProjectionMat->Invert();
  vtkInternal::VtkToGlMatrix(projectionMat4x4, fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_projection_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Impl->InverseProjectionMat.GetPointer(),
                             fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_inverse_projection_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  // Will require transpose of this matrix for OpenGL
  vtkMatrix4x4* modelviewMat4x4 =
    ren->GetActiveCamera()->GetViewTransformMatrix();
  this->Impl->InverseModelViewMat->DeepCopy(modelviewMat4x4);
  this->Impl->InverseModelViewMat->Invert();

  vtkInternal::VtkToGlMatrix(modelviewMat4x4, fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_modelview_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Impl->InverseModelViewMat.GetPointer(),
                             fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_inverse_modelview_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  // Will require transpose of this matrix for OpenGL
  // Scene matrix
  vtkMatrix4x4* volumeMatrix4x4 = vol->GetMatrix();
  this->Impl->InverseVolumeMat->DeepCopy(volumeMatrix4x4);
  this->Impl->InverseVolumeMat->Invert();

  vtkInternal::VtkToGlMatrix(volumeMatrix4x4, fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_volume_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Impl->InverseVolumeMat.GetPointer(),
                             fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_inverse_volume_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  // Compute texture to dataset matrix
  this->Impl->TextureToDataSetMat->Identity();
  this->Impl->TextureToDataSetMat->SetElement(0, 0,
    (1.0 / this->Impl->DatasetStepSize[0]));
  this->Impl->TextureToDataSetMat->SetElement(1, 1,
    (1.0 / this->Impl->DatasetStepSize[1]));
  this->Impl->TextureToDataSetMat->SetElement(2, 2,
    (1.0 / this->Impl->DatasetStepSize[2]));
  this->Impl->TextureToDataSetMat->SetElement(3, 3,
    1.0);
  this->Impl->TextureToDataSetMat->SetElement(0, 3,
    this->Impl->LoadedBounds[0]);
  this->Impl->TextureToDataSetMat->SetElement(1, 3,
    this->Impl->LoadedBounds[2]);
  this->Impl->TextureToDataSetMat->SetElement(2, 3,
    this->Impl->LoadedBounds[4]);

  this->Impl->InverseTextureToDataSetMat->DeepCopy(
    this->Impl->TextureToDataSetMat.GetPointer());
  this->Impl->InverseTextureToDataSetMat->Invert();
  vtkInternal::VtkToGlMatrix(this->Impl->TextureToDataSetMat.GetPointer(),
                             fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_texture_dataset_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));
  vtkInternal::VtkToGlMatrix(
    this->Impl->InverseTextureToDataSetMat.GetPointer(), fvalue16);
  glUniformMatrix4fv(this->Impl->Shader("m_inverse_texture_dataset_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::ToFloat(ren->GetActiveCamera()->GetPosition(), fvalue3, 3);
  glUniform3fv(this->Impl->Shader("m_camera_pos"), 1, &(fvalue3[0]));

  // NOTE Assuming that light is located on the camera
  glUniform3fv(this->Impl->Shader("m_light_pos"), 1, &(fvalue3[0]));

  vtkInternal::ToFloat(this->Impl->LoadedBounds[0],
                       this->Impl->LoadedBounds[2],
                       this->Impl->LoadedBounds[4], fvalue3);
  glUniform3fv(this->Impl->Shader("m_vol_extents_min"), 1,
               &(fvalue3[0]));

  vtkInternal::ToFloat(this->Impl->LoadedBounds[1],
                       this->Impl->LoadedBounds[3],
                       this->Impl->LoadedBounds[5], fvalue3);
  glUniform3fv(this->Impl->Shader("m_vol_extents_max"), 1,
               &(fvalue3[0]));

  vtkInternal::ToFloat(this->Impl->Extents[0],
                       this->Impl->Extents[2],
                       this->Impl->Extents[4], fvalue3);
  glUniform3fv(this->Impl->Shader("m_texture_extents_min"), 1,
               &(fvalue3[0]));
  vtkInternal::ToFloat(this->Impl->Extents[1],
                       this->Impl->Extents[3],
                       this->Impl->Extents[5], fvalue3);
  glUniform3fv(this->Impl->Shader("m_texture_extents_max"), 1,
               &(fvalue3[0]));

  // TODO Take consideration of reduction factor
  vtkInternal::ToFloat(this->Impl->WindowLowerLeft, fvalue2);
  glUniform2fv(this->Impl->Shader("m_window_lower_left_corner"),
               1, &fvalue2[0]);

  vtkInternal::ToFloat(1.0 / this->Impl->WindowSize[0],
                       1.0 / this->Impl->WindowSize[1], fvalue2);
  glUniform2fv(this->Impl->Shader("m_inv_original_window_size"),
               1, &fvalue2[0]);

  vtkInternal::ToFloat(1.0 / this->Impl->WindowSize[0],
                       1.0 / this->Impl->WindowSize[1], fvalue2);
  glUniform2fv(this->Impl->Shader("m_inv_window_size"), 1, &fvalue2[0]);

  // Updating cropping if enabled
  this->Impl->UpdateCropping(ren, vol);

  // Updating clipping if enabled
  this->Impl->UpdateClipping(ren, vol);

#ifndef __APPLE__
  glBindVertexArray(this->Impl->CubeVAOId);
#endif
  glDrawElements(GL_TRIANGLES,
                 this->Impl->BBoxPolyData->GetNumberOfCells() * 3,
                 GL_UNSIGNED_INT, 0);

  // Undo binds and state changes
  // TODO Provide a stack Impl
  this->Impl->Shader.UnUse();

  this->Impl->PrevInput = input;
}
