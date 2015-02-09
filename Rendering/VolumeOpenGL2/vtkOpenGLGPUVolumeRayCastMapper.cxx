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

#include "vtkOpenGLVolumeGradientOpacityTable.h"
#include "vtkOpenGLVolumeOpacityTable.h"
#include "vtkOpenGLVolumeRGBTable.h"
#include "vtkVolumeShaderComposer.h"
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
#include <vtkLight.h>
#include <vtkLightCollection.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLError.h>
#include <vtkOpenGLShaderCache.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkPerlinNoise.h>
#include <vtkPlaneCollection.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkShader.h>
#include <vtkShaderProgram.h>
#include <vtkSmartPointer.h>
#include <vtkTessellatedBoxSource.h>
#include <vtkTextureObject.h>
#include <vtkTimerLog.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkVolumeMask.h>
#include <vtkVolumeProperty.h>
#include <vtkWeakPointer.h>

// C/C++ includes
#include <cassert>
#include <limits>
#include <map>
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
    this->ValidTransferFunction = false;
    this->LoadDepthTextureExtensionsSucceeded = false;
    this->CameraWasInsideInLastUpdate = false;
    this->CubeVBOId = 0;
#ifndef __APPLE__
    this->CubeVAOId = 0;
#endif
    this->CubeIndicesId = 0;
    this->VolumeTextureObject = 0;
    this->NoiseTextureObject = 0;
    this->DepthTextureObject = 0;
    this->TextureWidth = 1024;
    this->ActualSampleDistance = 1.0;
    this->RGBTables = 0;
    this->OpacityTables = 0;
    this->Mask1RGBTable = 0;
    this->Mask2RGBTable =  0;
    this->GradientOpacityTables = 0;
    this->CurrentMask = 0;
    this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = -1;
    this->TextureSize[0] = this->TextureSize[1] = this->TextureSize[2] = -1;
    this->CellScale[0] = this->CellScale[1] = this->CellScale[2] = 0.0;
    this->NoiseTextureData = 0;

    this->NumberOfLights = 0;
    this->LightComplexity = 0;

    this->Extents[0] = VTK_INT_MAX;
    this->Extents[1] = VTK_INT_MIN;
    this->Extents[2] = VTK_INT_MAX;
    this->Extents[3] = VTK_INT_MIN;
    this->Extents[4] = VTK_INT_MAX;
    this->Extents[5] = VTK_INT_MIN;

    this->MaskTextures = new vtkMapMaskTextureId;
    }

  // Destructor
  //--------------------------------------------------------------------------
  ~vtkInternal()
    {
    if (this->NoiseTextureData)
      {
      delete [] this->NoiseTextureData;
      this->NoiseTextureData = 0;
      }

    if (this->NoiseTextureObject)
      {
      this->NoiseTextureObject->Delete();
      this->NoiseTextureObject = 0;
      }

    if (this->DepthTextureObject)
      {
      this->DepthTextureObject->Delete();
      this->DepthTextureObject = 0;
      }

    if (this->MaskTextures)
      {
      delete this->MaskTextures;
      this->MaskTextures = 0;
      }

    this->DeleteTransferFunctions();
    }

  // Helper methods
  //--------------------------------------------------------------------------
  template<typename T>
  static void ToFloat(const T& in1, const T& in2, float (&out)[2]);
  template<typename T>
  static void ToFloat(const T& in1, const T& in2, const T& in3,
                      float (&out)[3]);
  template<typename T>
  static void ToFloat(T* in, float* out, int noOfComponents);
  template<typename T>
  static void ToFloat(T (&in)[3], float (&out)[3]);
  template<typename T>
  static void ToFloat(T (&in)[2], float (&out)[2]);
  template<typename T>
  static void ToFloat(T& in, float& out);
  static void VtkToGlMatrix(vtkMatrix4x4* in, float (&out)[16],
                            int row = 4, int col = 4);

  void Initialize(vtkRenderer* ren, vtkVolume* vol,
                  int noOfComponents, int independentComponents);

  bool LoadVolume(vtkRenderer* ren, vtkImageData* imageData,
                  vtkDataArray* scalars);

  bool LoadMask(vtkRenderer* ren, vtkImageData* input,
                vtkImageData* maskInput, int textureExtent[6],
                vtkVolume* volume);

  void DeleteTransferFunctions();

  void ComputeBounds(vtkImageData* input);

  // Update transfer color function based on the incoming inputs
  // and number of scalar components.
  int UpdateColorTransferFunction(vtkRenderer* ren,
                                  vtkVolume* vol,
                                  int noOfComponents,
                                  unsigned int component);

  // Update opacity transfer function (not gradient opacity)
  int UpdateOpacityTransferFunction(vtkRenderer* ren,
                                    vtkVolume* vol,
                                    int noOfComponents,
                                    unsigned int component);

  // Update gradient opacity function
  int UpdateGradientOpacityTransferFunction(vtkRenderer* ren,
                                            vtkVolume* vol,
                                            int noOfComponents,
                                            unsigned int component);

  // Update noise texture (used to reduce rendering artifacts
  // specifically banding effects)
  void UpdateNoiseTexture(vtkRenderer* ren);

  // Update depth texture (used for early termination of the ray)
  void UpdateDepthTexture(vtkRenderer* ren, vtkVolume* vol);

  // Update parameters for lighting that will be used in the shader.
  void UpdateLightingParameters(vtkRenderer* ren, vtkVolume* vol);

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

  bool ValidTransferFunction;
  bool LoadDepthTextureExtensionsSucceeded;
  bool CameraWasInsideInLastUpdate;

  GLuint CubeVBOId;
#ifndef __APPLE__
  GLuint CubeVAOId;
#endif
  GLuint CubeIndicesId;

  vtkTextureObject* VolumeTextureObject;
  vtkTextureObject* NoiseTextureObject;
  vtkTextureObject* DepthTextureObject;

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

  int NumberOfLights;
  int LightComplexity;

  std::ostringstream ExtensionsStringStream;

  vtkOpenGLVolumeRGBTables* RGBTables;
  std::map<int, std::string> RGBTablesMap;

  vtkOpenGLVolumeOpacityTables* OpacityTables;
  std::map<int, std::string> OpacityTablesMap;

  vtkOpenGLVolumeRGBTable* Mask1RGBTable;
  vtkOpenGLVolumeRGBTable* Mask2RGBTable;
  vtkOpenGLVolumeGradientOpacityTables* GradientOpacityTables;
  std::map<int, std::string> GradientOpacityTablesMap;

  vtkTimeStamp ShaderBuildTime;

  vtkNew<vtkMatrix4x4> TextureToDataSetMat;
  vtkNew<vtkMatrix4x4> InverseTextureToDataSetMat;

  vtkNew<vtkMatrix4x4> InverseProjectionMat;
  vtkNew<vtkMatrix4x4> InverseModelViewMat;
  vtkNew<vtkMatrix4x4> InverseVolumeMat;

  vtkNew<vtkMatrix4x4> TextureToEyeTransposeInverse;

  vtkSmartPointer<vtkPolyData> BBoxPolyData;

  vtkMapMaskTextureId* MaskTextures;
  vtkVolumeMask* CurrentMask;

  vtkTimeStamp InitializationTime;
  vtkTimeStamp InputUpdateTime;

  vtkShaderProgram* ShaderProgram;
  vtkOpenGLShaderCache* ShaderCache;
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
  T* in, float* out, int noOfComponents)
{
  for (int i = 0; i < noOfComponents; ++i)
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
  vtkMatrix4x4* in, float (&out)[16], int row, int col)
{
  for (int i = 0; i < row; ++i)
    {
    for (int j = 0; j < col; ++j)
      {
      out[j * row + i] = in->Element[i][j];
      }
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::Initialize(
  vtkRenderer* vtkNotUsed(ren), vtkVolume* vol, int
  noOfComponents, int independentComponents)
{
  GLenum err = glewInit();
  if (GLEW_OK != err)
    {
    cerr <<"Error: "<< glewGetErrorString(err)<<endl;
    }

  // This is to ignore INVALID ENUM error 1282
  err = glGetError();

  this->DeleteTransferFunctions();

  // Create RGB lookup table
  if (noOfComponents > 1 && independentComponents)
    {
    this->RGBTables = new vtkOpenGLVolumeRGBTables(noOfComponents);
    }
  else
    {
    this->RGBTables = new vtkOpenGLVolumeRGBTables(1);
    }

  if (this->Parent->MaskInput != 0 &&
      this->Parent->MaskType == LabelMapMaskType)
    {
    if(this->Mask1RGBTable == 0)
      {
      this->Mask1RGBTable = new vtkOpenGLVolumeRGBTable();
      }
    if(this->Mask2RGBTable == 0)
      {
      this->Mask2RGBTable = new vtkOpenGLVolumeRGBTable();
      }
    }

  // We support upto four components
  if (noOfComponents > 1 && independentComponents)
    {
    this->OpacityTables = new vtkOpenGLVolumeOpacityTables(noOfComponents);
    }
  else
    {
    this->OpacityTables = new vtkOpenGLVolumeOpacityTables(1);
    }

  if (noOfComponents > 1 && independentComponents)
    {
    // Assuming that all four components has gradient opacity for now
    this->GradientOpacityTables =
      new vtkOpenGLVolumeGradientOpacityTables(noOfComponents);
    }
  else
    {
    if (vol->GetProperty()->HasGradientOpacity())
      {
      this->GradientOpacityTables =
        new vtkOpenGLVolumeGradientOpacityTables(1);
      }
    }

  this->OpacityTablesMap.clear();
  this->RGBTablesMap.clear();
  this->GradientOpacityTablesMap.clear();

  std::ostringstream numeric;
  for (int i = 0; i < noOfComponents; ++i)
    {
    numeric << i;
    if (i > 0)
      {
      this->OpacityTablesMap[i] = std::string("in_opacityTransferFunc") +
                                  numeric.str();
      this->RGBTablesMap[i] = std::string("in_colorTransferFunc") +
                              numeric.str();
      this->GradientOpacityTablesMap[i] = std::string("in_gradientTransferFunc") +
                                          numeric.str();
      }
    else
      {
      this->OpacityTablesMap[i] = std::string("in_opacityTransferFunc");
      this->RGBTablesMap[i] = std::string("in_colorTransferFunc");
      this->GradientOpacityTablesMap[i] = std::string("in_gradientTransferFunc");
      }
    numeric.str("");
    numeric.clear();
    }

  this->InitializationTime.Modified();
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadVolume(vtkRenderer* ren,
  vtkImageData* imageData, vtkDataArray* scalars)
{
  // Allocate data with internal format and foramt as (GL_RED)
  GLint internalFormat = 0;
  GLenum format = 0;
  GLenum type = 0;

  double shift = 0.0;
  double scale = 1.0;
  bool handleLargeDataTypes = false;

  int scalarType = scalars->GetDataType();
  if (scalars->GetNumberOfComponents() == 4)
    {
    internalFormat = GL_RGBA8;
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
        shift=-this->ScalarsRange[0] / VTK_UNSIGNED_INT_MAX;
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

  // Update texture size
  imageData->GetExtent(this->Extents);

  int i = 0;
  while(i < 3)
    {
    this->TextureSize[i] = this->Extents[2*i+1] - this->Extents[2*i] + 1;
    ++i;
    }

  if (!this->VolumeTextureObject)
    {
    this->VolumeTextureObject = vtkTextureObject::New();

    }

  this->VolumeTextureObject->SetContext(vtkOpenGLRenderWindow::SafeDownCast(
                                         ren->GetRenderWindow()));

  this->VolumeTextureObject->SetDataType(type);
  this->VolumeTextureObject->SetFormat(format);
  this->VolumeTextureObject->SetInternalFormat(internalFormat);

  if (!handleLargeDataTypes)
    {
    void* dataPtr = scalars->GetVoidPointer(0);

    // TODO: glPixelTransfer is not supported in GL 3.2 or higher.
    // When we tried to apply scale and bias in the shader, then something
    // didn't work out quite right.
    glPixelTransferf(GL_RED_SCALE,static_cast<GLfloat>(this->Scale));
    glPixelTransferf(GL_RED_BIAS,static_cast<GLfloat>(this->Bias));
    this->VolumeTextureObject->Create3DFromRaw(
      this->TextureSize[0],
      this->TextureSize[1],
      this->TextureSize[2],
      scalars->GetNumberOfComponents(),
      scalarType,
      dataPtr);
    this->VolumeTextureObject->Activate();
    this->VolumeTextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
    this->VolumeTextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
    this->VolumeTextureObject->SetWrapR(vtkTextureObject::ClampToEdge);
    this->VolumeTextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->VolumeTextureObject->SetMinificationFilter(vtkTextureObject::Linear);
    this->VolumeTextureObject->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);

    glPixelTransferf(GL_RED_SCALE, 1.0);
    glPixelTransferf(GL_RED_BIAS, 0.0);
    }
  else
    {
    // Convert and send to the GPU, z-slice by z-slice so that we won't allocate
    // memory at once.Allocate memory on the GPU (NULL data pointer with the
    // right dimensions). Here we are assuming that
    // GL_ARB_texture_non_power_of_two is available
    this->VolumeTextureObject->Create3DFromRaw(
      this->TextureSize[0],
      this->TextureSize[1],
      this->TextureSize[2],
      scalars->GetNumberOfComponents(),
      scalarType,
      0);
    this->VolumeTextureObject->Activate();
    this->VolumeTextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
    this->VolumeTextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
    this->VolumeTextureObject->SetWrapR(vtkTextureObject::ClampToEdge);
    this->VolumeTextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->VolumeTextureObject->SetMinificationFilter(vtkTextureObject::Linear);
    this->VolumeTextureObject->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Send the slices one by one to the GPU. We are not sending all of them
    // together so as to avoid allocating big data on the GPU which may not
    // work if the original dataset is big as well.
    vtkFloatArray* sliceArray = vtkFloatArray::New();
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
                      format, type, slicePtr);
      ++k;
      kOffset += kInc;
      }
    sliceArray->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadMask(vtkRenderer* ren,
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

    vtkVolumeMask* mask = 0;
    if(it2 == this->MaskTextures->Map.end())
      {
      mask = new vtkVolumeMask();
      this->MaskTextures->Map[maskInput] = mask;
      }
    else
      {
      mask = (*it2).second;
      }

    mask->Update(ren,
                 maskInput,
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
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::DeleteTransferFunctions()
{
  if (this->RGBTables)
    {
    delete this->RGBTables;
    this->RGBTables = 0;
    }

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

  if (this->OpacityTables)
    {
    delete this->OpacityTables;
    this->OpacityTables = 0;
    }

  if (this->GradientOpacityTables)
    {
    delete this->GradientOpacityTables;
    this->GradientOpacityTables = 0;
    }

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
  vtkRenderer* ren, vtkVolume* vol, int vtkNotUsed(noOfComponents),
  unsigned int component)
{
  // Volume property cannot be null.
  vtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Build the colormap in a 1D texture.
  // 1D RGB-texture=mapping from scalar values to color values
  // build the table.
  vtkColorTransferFunction* colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(component);

  // Add points only if its not being added before
  if (colorTransferFunction->GetSize() < 1)
    {
    colorTransferFunction->AddRGBPoint(this->ScalarsRange[0], 0.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(this->ScalarsRange[1], 1.0, 1.0, 1.0);
    }

  int filterVal =
    volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION ?
      vtkTextureObject::Linear : vtkTextureObject::Nearest;

  this->RGBTables->GetTable(component)->Update(
    volumeProperty->GetRGBTransferFunction(component),
    this->ScalarsRange,
    filterVal,
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  if (this->Parent->MaskInput != 0 &&
      this->Parent->MaskType == LabelMapMaskType)
    {
    vtkColorTransferFunction* colorTransferFunc =
      volumeProperty->GetRGBTransferFunction(1);
    this->Mask1RGBTable->Update(colorTransferFunc, this->ScalarsRange,
                                vtkTextureObject::Nearest,
                                vtkOpenGLRenderWindow::SafeDownCast(
                                  ren->GetRenderWindow()));

    colorTransferFunc = volumeProperty->GetRGBTransferFunction(2);
    this->Mask2RGBTable->Update(colorTransferFunc, this->ScalarsRange,
                                vtkTextureObject::Nearest,
                                vtkOpenGLRenderWindow::SafeDownCast(
                                  ren->GetRenderWindow()));
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateOpacityTransferFunction(
  vtkRenderer* ren, vtkVolume* vol, int vtkNotUsed(noOfComponents),
  unsigned int component)
{
  if (!vol)
    {
    return 1;
    }

  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  vtkPiecewiseFunction* scalarOpacity =
    volumeProperty->GetScalarOpacity(component);

  if (scalarOpacity->GetSize() < 1)
    {
    scalarOpacity->AddPoint(this->ScalarsRange[0], 0.0);
    scalarOpacity->AddPoint(this->ScalarsRange[1], 0.5);
    }

  int filterVal =
    volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION ?
      vtkTextureObject::Linear : vtkTextureObject::Nearest;

  this->OpacityTables->GetTable(component)->Update(
    scalarOpacity,this->Parent->BlendMode,
    this->ActualSampleDistance,
    this->ScalarsRange,
    volumeProperty->GetScalarOpacityUnitDistance(component),
    filterVal,
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

//----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::
  UpdateGradientOpacityTransferFunction(vtkRenderer* ren, vtkVolume* vol,
    int vtkNotUsed(noOfComponents), unsigned int component)
{
  if (!vol)
    {
    return 1;
    }

  vtkVolumeProperty* volumeProperty = vol->GetProperty();

  // TODO Currently we expect the all of the tables will
  // be initialized once and if at that time, the gradient
  // opacity was not enabled then it is not used later.
  if (!volumeProperty->HasGradientOpacity(component) ||
      !this->GradientOpacityTables)
    {
    return 1;
    }

  vtkPiecewiseFunction* gradientOpacity =
    volumeProperty->GetGradientOpacity(component);

  if (gradientOpacity->GetSize() < 1)
    {
    gradientOpacity->AddPoint(this->ScalarsRange[0], 0.0);
    gradientOpacity->AddPoint(this->ScalarsRange[1], 0.5);
    }

  int filterVal =
    volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION ?
      vtkTextureObject::Linear : vtkTextureObject::Nearest;

  this->GradientOpacityTables->GetTable(component)->Update(
    gradientOpacity,
    this->ActualSampleDistance,
    this->ScalarsRange,
    volumeProperty->GetScalarOpacityUnitDistance(),
    filterVal,
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  return 0;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateNoiseTexture(
  vtkRenderer* ren)
{
  if (!this->NoiseTextureObject)
    {
    this->NoiseTextureObject = vtkTextureObject::New();
    }

  this->NoiseTextureObject->SetContext(vtkOpenGLRenderWindow::SafeDownCast(
                                         ren->GetRenderWindow()));

  if (!this->NoiseTextureObject->GetHandle())
    {
    GLsizei size = 128;
    GLint maxSize;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (size > maxSize)
      {
      size = maxSize;
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
      noiseGenerator->SetAmplitude(0.1);
      int j = 0;
      while(j < size)
        {
        int i = 0;
        while(i < size)
          {
          this->NoiseTextureData[j * size + i] =
            static_cast<float>(noiseGenerator->EvaluateFunction(i, j, 0.0) + 0.1);
          ++i;
          }
        ++j;
        }
      }

    this->NoiseTextureObject->Create2DFromRaw(size,
                                              size,
                                              1,
                                              VTK_FLOAT,
                                              this->NoiseTextureData);
    this->NoiseTextureObject->SetWrapS(vtkTextureObject::Repeat);
    this->NoiseTextureObject->SetWrapT(vtkTextureObject::Repeat);
    this->NoiseTextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->NoiseTextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
    this->NoiseTextureObject->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);
    this->NoiseTextureObject->Activate();
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

  if (!this->DepthTextureObject)
    {
    this->DepthTextureObject = vtkTextureObject::New();
    }

  this->DepthTextureObject->SetContext(vtkOpenGLRenderWindow::SafeDownCast(
                                        ren->GetRenderWindow()));
  if (!this->DepthTextureObject->GetHandle())
    {
    // First set the parameters
    this->DepthTextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
    this->DepthTextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
    this->DepthTextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->DepthTextureObject->SetMinificationFilter(vtkTextureObject::Linear);
    this->DepthTextureObject->AllocateDepth(this->WindowSize[0],
                                            this->WindowSize[1],
                                            4);
    }

  this->DepthTextureObject->CopyFromFrameBuffer(this->WindowLowerLeft[0],
                                                this->WindowLowerLeft[1],
                                                0, 0,
                                                this->WindowSize[0],
                                                this->WindowSize[1]);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateLightingParameters(
  vtkRenderer* ren, vtkVolume* vtkNotUsed(vol))
{
  if (!this->ShaderProgram)
    {
    return;
    }

  this->ShaderProgram->SetUniformi("in_twoSidedLighting",
                                   ren->GetTwoSidedLighting());

  // for lightkit case there are some parameters to set
  vtkCamera* cam = ren->GetActiveCamera();
  vtkTransform* viewTF = cam->GetModelViewTransformObject();

  // Bind some light settings
  int numberOfLights = 0;
  vtkLightCollection *lc = ren->GetLights();
  vtkLight *light;

  vtkCollectionSimpleIterator sit;
  float lightAmbientColor[6][3];
  float lightDiffuseColor[6][3];
  float lightSpecularColor[6][3];
  float lightDirection[6][3];
  for(lc->InitTraversal(sit);
      (light = lc->GetNextLight(sit)); )
    {
    float status = light->GetSwitch();
    if (status > 0.0)
      {
      double* aColor = light->GetAmbientColor();
      double* dColor = light->GetDiffuseColor();
      double* sColor = light->GetDiffuseColor();
      double intensity = light->GetIntensity();
      lightAmbientColor[numberOfLights][0] = aColor[0] * intensity;
      lightAmbientColor[numberOfLights][1] = aColor[1] * intensity;
      lightAmbientColor[numberOfLights][2] = aColor[2] * intensity;
      lightDiffuseColor[numberOfLights][0] = dColor[0] * intensity;
      lightDiffuseColor[numberOfLights][1] = dColor[1] * intensity;
      lightDiffuseColor[numberOfLights][2] = dColor[2] * intensity;
      lightSpecularColor[numberOfLights][0] = sColor[0] * intensity;
      lightSpecularColor[numberOfLights][1] = sColor[1] * intensity;
      lightSpecularColor[numberOfLights][2] = sColor[2] * intensity;
      // Get required info from light
      double* lfp = light->GetTransformedFocalPoint();
      double* lp = light->GetTransformedPosition();
      double lightDir[3];
      vtkMath::Subtract(lfp, lp, lightDir);
      vtkMath::Normalize(lightDir);
      double *tDir = viewTF->TransformNormal(lightDir);
      lightDirection[numberOfLights][0] = tDir[0];
      lightDirection[numberOfLights][1] = tDir[1];
      lightDirection[numberOfLights][2] = tDir[2];
      numberOfLights++;
      }
    }

  this->ShaderProgram->SetUniform3fv("in_lightAmbientColor",
                                     numberOfLights, lightAmbientColor);
  this->ShaderProgram->SetUniform3fv("in_lightDiffuseColor",
                                     numberOfLights, lightDiffuseColor);
  this->ShaderProgram->SetUniform3fv("in_lightSpecularColor",
                                     numberOfLights, lightSpecularColor);
  this->ShaderProgram->SetUniform3fv("in_lightDirection",
                                     numberOfLights, lightDirection);
  this->ShaderProgram->SetUniformi("in_numberOfLights",
                                   numberOfLights);

  // we are done unless we have positional lights
  if (this->LightComplexity < 3)
    {
    return;
    }

  // if positional lights pass down more parameters
  float lightAttenuation[6][3];
  float lightPosition[6][3];
  float lightConeAngle[6];
  float lightExponent[6];
  int lightPositional[6];
  numberOfLights = 0;
  for(lc->InitTraversal(sit);
      (light = lc->GetNextLight(sit)); )
    {
    float status = light->GetSwitch();
    if (status > 0.0)
      {
      double* attn = light->GetAttenuationValues();
      lightAttenuation[numberOfLights][0] = attn[0];
      lightAttenuation[numberOfLights][1] = attn[1];
      lightAttenuation[numberOfLights][2] = attn[2];
      lightExponent[numberOfLights] = light->GetExponent();
      lightConeAngle[numberOfLights] = light->GetConeAngle();
      double* lp = light->GetTransformedPosition();
      double* tlp = viewTF->TransformPoint(lp);
      lightPosition[numberOfLights][0] = tlp[0];
      lightPosition[numberOfLights][1] = tlp[1];
      lightPosition[numberOfLights][2] = tlp[2];
      lightPositional[numberOfLights] = light->GetPositional();
      numberOfLights++;
      }
    }
  this->ShaderProgram->SetUniform3fv("in_lightAttenuation", numberOfLights, lightAttenuation);
  this->ShaderProgram->SetUniform1iv("in_lightPositional", numberOfLights, lightPositional);
  this->ShaderProgram->SetUniform3fv("in_lightPosition", numberOfLights, lightPosition);
  this->ShaderProgram->SetUniform1fv("in_lightExponent", numberOfLights, lightExponent);
  this->ShaderProgram->SetUniform1fv("in_lightConeAngle", numberOfLights, lightConeAngle);
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
  if (input->GetMTime() > this->InputUpdateTime.GetMTime() ||
      this->IsCameraInside(ren, vol) ||
      this->CameraWasInsideInLastUpdate)
    {
    vtkNew<vtkTessellatedBoxSource> boxSource;
    boxSource->SetBounds(this->LoadedBounds);
    boxSource->QuadsOn();
    boxSource->SetLevel(0);

    vtkNew<vtkDensifyPolyData> densityPolyData;

    if (input->GetMTime() <= this->InputUpdateTime.GetMTime() &&
        this->IsCameraInside(ren, vol))
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

    this->ShaderProgram->EnableAttributeArray("in_vertexPos");
    this->ShaderProgram->UseAttributeArray("in_vertexPos", 0, 0, VTK_FLOAT,
                                           3, vtkShaderProgram::NoNormalize);

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
    this->ShaderProgram->EnableAttributeArray("in_vertexPos");
    this->ShaderProgram->UseAttributeArray("in_vertexPos", 0, 0, VTK_FLOAT,
                                           3, vtkShaderProgram::NoNormalize);
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

    this->ShaderProgram->SetUniform1fv("cropping_planes", 6, cropPlanes);
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

    this->ShaderProgram->SetUniform1iv("cropping_flags",
                                       numberOfRegions,
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

    this->ShaderProgram->SetUniform1fv("in_clippingPlanes",
                                       static_cast<int>(clippingPlanes.size()),
                                       &clippingPlanes[0]);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateSamplingDistance(
  vtkImageData* input, vtkRenderer* vtkNotUsed(ren), vtkVolume* vol)
{
  if (!this->Parent->AutoAdjustSampleDistances)
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

    if (this->Parent->ReductionFactor < 1.0)
      {
      // 0.5 is done to increase the impact factor
      this->ActualSampleDistance /=
        static_cast<GLfloat>(this->Parent->ReductionFactor * 0.5);
      }
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

  if (this->Impl->VolumeTextureObject)
    {
    this->Impl->VolumeTextureObject->ReleaseGraphicsResources(window);
    this->Impl->VolumeTextureObject->Delete();
    this->Impl->VolumeTextureObject = 0;
    }

  if (this->Impl->NoiseTextureObject)
    {
    this->Impl->NoiseTextureObject->ReleaseGraphicsResources(window);
    this->Impl->NoiseTextureObject->Delete();
    this->Impl->NoiseTextureObject = 0;
    }

  if (this->Impl->DepthTextureObject)
    {
    this->Impl->DepthTextureObject->ReleaseGraphicsResources(window);
    this->Impl->DepthTextureObject->Delete();
    this->Impl->DepthTextureObject = 0;
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
        texture->ReleaseGraphicsResources(window);
        delete texture;
        ++it;
        }
      this->Impl->MaskTextures->Map.clear();
      }
    }

  if(this->Impl->RGBTables)
    {
    this->Impl->RGBTables->ReleaseGraphicsResources(window);
    delete this->Impl->RGBTables;
    this->Impl->RGBTables = 0;
    }

  if(this->Impl->Mask1RGBTable)
    {
    this->Impl->Mask1RGBTable->ReleaseGraphicsResources(window);
    delete this->Impl->Mask1RGBTable;
    this->Impl->Mask1RGBTable = 0;
    }

  if(this->Impl->Mask2RGBTable)
    {
    this->Impl->Mask2RGBTable->ReleaseGraphicsResources(window);
    delete this->Impl->Mask2RGBTable;
    this->Impl->Mask2RGBTable = 0;
    }

  if(this->Impl->OpacityTables)
    {
    this->Impl->OpacityTables->ReleaseGraphicsResources(window);
    delete this->Impl->OpacityTables;
    this->Impl->OpacityTables = 0;
    }

  if (this->Impl->GradientOpacityTables)
    {
    this->Impl->GradientOpacityTables->ReleaseGraphicsResources(window);
    delete this->Impl->GradientOpacityTables;
    this->Impl->GradientOpacityTables = 0;
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::BuildShader(vtkRenderer* ren,
                                                  vtkVolume* vol,
                                                  int noOfComponents)
{
  std::string vertexShader (raycastervs);
  std::string fragmentShader (raycasterfs);

  // Every volume should have a property (cannot be NULL);
  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  int independentComponents = volumeProperty->GetIndependentComponents();

  if (volumeProperty->GetShade())
    {
    vtkLightCollection* lc = ren->GetLights();
    vtkLight* light;
    // Compute light complexity.
    vtkCollectionSimpleIterator sit;
    for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit)); )
      {
      float status = light->GetSwitch();
      if (status > 0.0)
        {
        this->Impl->NumberOfLights++;
        if (this->Impl->LightComplexity == 0)
          {
          this->Impl->LightComplexity = 1;
          }
        }

      if (this->Impl->LightComplexity == 1
          && (this->Impl->NumberOfLights > 1
            || light->GetIntensity() != 1.0
            || light->GetLightType() != VTK_LIGHT_TYPE_HEADLIGHT))
        {
        this->Impl->LightComplexity = 2;
        }

      if (this->Impl->LightComplexity < 3
          && (light->GetPositional()))
        {
        this->Impl->LightComplexity = 3;
        break;
        }
      }
    }

  // Base methods replacements
  //--------------------------------------------------------------------------
  vertexShader = vtkvolume::replace(
    vertexShader,
    "//VTK::ComputeClipPos::Impl",
    vtkvolume::ComputeClip(ren, this, vol),
    true);

  vertexShader = vtkvolume::replace(
    vertexShader,
    "//VTK::ComputeTextureCoords::Impl",
    vtkvolume::ComputeTextureCoords(ren, this, vol),
    true);

  vertexShader = vtkvolume::replace(
    vertexShader,
    "//VTK::Base::Dec",
    vtkvolume::BaseGlobalsVert(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Base::Dec",
    vtkvolume::BaseGlobalsFrag(ren, this, vol, this->Impl->NumberOfLights,
                               this->Impl->LightComplexity, noOfComponents,
                               independentComponents),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Base::Init",
    vtkvolume::BaseInit(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Base::Impl",
    vtkvolume::BaseIncrement(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Base::Exit",
    vtkvolume::BaseExit(ren, this, vol),
    true);

  // Termination methods replacements
  //--------------------------------------------------------------------------
  vertexShader = vtkvolume::replace(
    vertexShader,
    "//VTK::Termination::Dec",
    vtkvolume::TerminationGlobalsVert(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Termination::Dec",
    vtkvolume::TerminationGlobalsFrag(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Terminate::Init",
    vtkvolume::TerminationInit(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Terminate::Impl",
    vtkvolume::TerminationIncrement(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Terminate::Exit",
    vtkvolume::TerminationExit(ren, this, vol),
    true);

  // Shading methods replacements
  //--------------------------------------------------------------------------
  vertexShader = vtkvolume::replace(
    vertexShader,
    "//VTK::Shading::Dec",
    vtkvolume::ShadingGlobalsVert(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Shading::Dec",
    vtkvolume::ShadingGlobalsFrag(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Shading::Init",
    vtkvolume::ShadingInit(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Shading::Impl",
    vtkvolume::ShadingIncrement(ren, this, vol, this->MaskInput,
                                this->Impl->CurrentMask,
                                this->MaskType, noOfComponents,
                                independentComponents),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Shading::Exit",
    vtkvolume::ShadingExit(ren, this, vol, noOfComponents,
                           independentComponents),
    true);


  // Compute methods replacements
  //--------------------------------------------------------------------------
  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::ComputeOpacity::Dec",
    vtkvolume::OpacityTransferFunc(ren, this, vol, noOfComponents,
                                  independentComponents,
                                   this->Impl->OpacityTablesMap),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::ComputeGradient::Dec",
    vtkvolume::GradientsComputeFunc(ren, this, vol, noOfComponents,
                                    independentComponents,
                                    this->Impl->GradientOpacityTablesMap),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::ColorTransferFunc::Dec",
    vtkvolume::ColorTransferFunc(ren, this, vol, noOfComponents,
                                 independentComponents,
                                 this->Impl->RGBTablesMap),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::ComputeLighting::Dec",
    vtkvolume::LightComputeFunc(ren, this, vol, noOfComponents,
                                independentComponents,
                                this->Impl->NumberOfLights,
                                this->Impl->LightComplexity),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::RayDirectionFunc::Dec",
    vtkvolume::RayDirectionFunc(ren, this, vol,noOfComponents),
    true);

  // Cropping methods replacements
  //--------------------------------------------------------------------------
  vertexShader = vtkvolume::replace(
    vertexShader,
    "//VTK::Cropping::Dec",
    vtkvolume::CroppingGlobalsVert(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Cropping::Dec",
    vtkvolume::CroppingGlobalsFrag(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Cropping::Init",
    vtkvolume::CroppingInit(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Cropping::Impl",
    vtkvolume::CroppingIncrement(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Cropping::Exit",
    vtkvolume::CroppingExit(ren, this, vol),
    true);

  // Clipping methods replacements
  //--------------------------------------------------------------------------
  vertexShader = vtkvolume::replace(
    vertexShader,
    "//VTK::Clipping::Dec",
    vtkvolume::ClippingGlobalsVert(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Clipping::Dec",
    vtkvolume::ClippingGlobalsFrag(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Clipping::Init",
    vtkvolume::ClippingInit(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Clipping::Impl",
    vtkvolume::ClippingIncrement(ren, this, vol),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::Clipping::Exit",
    vtkvolume::ClippingExit(ren, this, vol),
    true);

  // Masking methods replacements
  //--------------------------------------------------------------------------
  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::BinaryMask::Dec",
    vtkvolume::BinaryMaskGlobalsFrag(ren, this, vol, this->MaskInput,
                                     this->Impl->CurrentMask,
                                     this->MaskType),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::BinaryMask::Impl",
    vtkvolume::BinaryMaskIncrement(ren, this, vol, this->MaskInput,
                                   this->Impl->CurrentMask,
                                   this->MaskType),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::CompositeMask::Dec",
    vtkvolume::CompositeMaskGlobalsFrag(ren, this, vol, this->MaskInput,
                                        this->Impl->CurrentMask,
                                        this->MaskType),
    true);

  fragmentShader = vtkvolume::replace(
    fragmentShader,
    "//VTK::CompositeMask::Impl",
    vtkvolume::CompositeMaskIncrement(ren, this, vol, this->MaskInput,
                                      this->Impl->CurrentMask,
                                      this->MaskType),
    true);

  // Now compile the shader
  //--------------------------------------------------------------------------
  this->Impl->ShaderProgram = this->Impl->ShaderCache->ReadyShader(
    vertexShader.c_str(), fragmentShader.c_str(), "");
  if (!this->Impl->ShaderProgram->GetCompiled())
    {
    vtkErrorMacro("Shader failed to compile");
    }


  this->Impl->ShaderBuildTime.Modified();
}

//-----------------------------------------------------------------------------
// Update the reduction factor of the render viewport (this->ReductionFactor)
// according to the time spent in seconds to render the previous frame
// (this->TimeToDraw) and a time in seconds allocated to render the next
// frame (allocatedTime).
// \pre valid_current_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
// \pre positive_TimeToDraw: this->TimeToDraw>=0.0
// \pre positive_time: allocatedTime>0.0
// \post valid_new_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ComputeReductionFactor(
  double allocatedTime)
{
  if ( !this->AutoAdjustSampleDistances )
    {
    this->ReductionFactor = 1.0 / this->ImageSampleDistance;
    return;
    }

  if ( this->TimeToDraw )
    {
    double oldFactor = this->ReductionFactor;

    double timeToDraw;
    if (allocatedTime < 1.0)
      {
      timeToDraw = this->SmallTimeToDraw;
      if ( timeToDraw == 0.0 )
        {
        timeToDraw = this->BigTimeToDraw/3.0;
        }
      }
    else
      {
      timeToDraw = this->BigTimeToDraw;
      }

    if ( timeToDraw == 0.0 )
      {
      timeToDraw = 10.0;
      }

    double fullTime = timeToDraw / this->ReductionFactor;
    double newFactor = allocatedTime / fullTime;

    if ( oldFactor == 1.0 ||
         newFactor / oldFactor > 1.3 ||
         newFactor / oldFactor < .95 )
      {
      this->ReductionFactor = (newFactor+oldFactor)/2.0;

      this->ReductionFactor = (this->ReductionFactor > 5.0) ? (1.00) :
                                (this->ReductionFactor);
      this->ReductionFactor = (this->ReductionFactor > 1.0) ? (0.99) :
                                (this->ReductionFactor);
      this->ReductionFactor = (this->ReductionFactor < 0.1) ? (0.10) :
                                (this->ReductionFactor);

      if ( 1.0/this->ReductionFactor > this->MaximumImageSampleDistance )
        {
        this->ReductionFactor = 1.0 / this->MaximumImageSampleDistance;
        }
      if ( 1.0/this->ReductionFactor < this->MinimumImageSampleDistance )
        {
        this->ReductionFactor = 1.0 / this->MinimumImageSampleDistance;
        }
      }
    }
  else
    {
    this->ReductionFactor = 1.0;
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GPURender(vtkRenderer* ren,
                                                vtkVolume* vol)
{
  vtkOpenGLClearErrorMacro();

  // Make sure the context is current
  ren->GetRenderWindow()->MakeCurrent();

  // Update in_volume first to make sure states are current
  vol->Update();

  // Get the input
  vtkImageData* input = this->GetTransformedInput();

  // Get the volume property (must have one)
  vtkVolumeProperty* volumeProperty = vol->GetProperty();

  // Check whether we have independent components or not
  int independentComponents = volumeProperty->GetIndependentComponents();

  vtkDataArray* scalars = this->GetScalars(input,
                          this->ScalarMode,
                          this->ArrayAccessMode,
                          this->ArrayId,
                          this->ArrayName,
                          this->CellFlag);

  // How many components are there?
  int noOfComponents = scalars->GetNumberOfComponents();

  // Set OpenGL states
  vtkVolumeStateRAII glState;

  if (volumeProperty->GetMTime() > this->Impl->InitializationTime.GetMTime())
    {
    this->Impl->Initialize(ren, vol, noOfComponents,
                           independentComponents);
    }

  // If it is just one, then get the range from the scalars
  if(noOfComponents == 1)
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
  bool volumeModified = false;
  if (input->GetMTime() > this->Impl->InputUpdateTime.GetMTime())
    {
    volumeModified = true;
    input->GetDimensions(this->Impl->Dimensions);

    // Update bounds, data, and geometry
    this->Impl->ComputeBounds(input);
    this->Impl->LoadVolume(ren, input, scalars);
    this->Impl->LoadMask(ren, input, this->MaskInput,
                         this->Impl->Extents, vol);
    }

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

  this->ComputeReductionFactor(vol->GetAllocatedRenderTime());
  this->Impl->UpdateSamplingDistance(input, ren, vol);

  // Build shader now
  // First get the shader cache from the render window. This is important
  // to make sure that shader cache knows the state of various shader programs
  // in use.
  vtkOpenGLRenderWindow* renWin =
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->Impl->ShaderCache = renWin->GetShaderCache();

  if (volumeProperty->GetMTime() >
      this->Impl->ShaderBuildTime.GetMTime() ||
      this->GetMTime() > this->Impl->ShaderBuildTime.GetMTime() ||
      ren->GetActiveCamera()->GetParallelProjection() !=
      this->Impl->LastProjectionParallel)
    {
    this->Impl->LastProjectionParallel =
      ren->GetActiveCamera()->GetParallelProjection();
    this->BuildShader(ren, vol, noOfComponents);
    }

  // Bind the shader
  this->Impl->ShaderCache->ReadyShader(
    this->Impl->ShaderProgram);

  // And now update the geometry that will be used
  // to render the 3D texture
  this->Impl->UpdateVolumeGeometry(ren, vol, input);

  // Update opacity transfer function
  for (int i = 0; i < (independentComponents ?
       noOfComponents : 1); ++i)
    {
    this->Impl->UpdateOpacityTransferFunction(ren, vol,
      scalars->GetNumberOfComponents(), i);

    this->Impl->UpdateGradientOpacityTransferFunction(ren, vol,
      scalars->GetNumberOfComponents(), i);

    // Update transfer color functions
    this->Impl->UpdateColorTransferFunction(ren, vol,
      scalars->GetNumberOfComponents(), i);
    }

  // Update noise sampler texture
  this->Impl->UpdateNoiseTexture(ren);

  // Grab depth sampler buffer (to handle cases when we are rendering geometry
  // and in_volume together
  this->Impl->UpdateDepthTexture(ren, vol);

  // Update lighting parameters
  this->Impl->UpdateLightingParameters(ren, vol);

  // Temporary variables
  float fvalue2[2];
  float fvalue3[3];
  float fvalue4[4];
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

  if (ren->GetActiveCamera()->GetParallelProjection())
    {
    double dir[4];
    ren->GetActiveCamera()->GetDirectionOfProjection(dir);
    vtkInternal::ToFloat(dir[0], dir[1], dir[2], fvalue3);
    this->Impl->ShaderProgram->SetUniform3fv(
      "in_projectionDirection", 1, &fvalue3);
    }

  // Pass constant uniforms at initialization
  this->Impl->ShaderProgram->SetUniformi("in_noOfComponents",
                                         noOfComponents);
  this->Impl->ShaderProgram->SetUniformi("in_independentComponents",
                                         independentComponents);


  // Step should be dependant on the bounds and not on the texture size
  // since we can have non uniform voxel size / spacing / aspect ratio
  vtkInternal::ToFloat(this->Impl->CellStep, fvalue3);
  this->Impl->ShaderProgram->SetUniform3fv("in_cellStep", 1, &fvalue3);

  vtkInternal::ToFloat(this->Impl->CellScale, fvalue3);
  this->Impl->ShaderProgram->SetUniform3fv("in_cellScale", 1, &fvalue3);

  vtkInternal::ToFloat(this->Impl->CellSpacing, fvalue3);
  this->Impl->ShaderProgram->SetUniform3fv("in_cellSpacing", 1, &fvalue3);

  this->Impl->ShaderProgram->SetUniformf("in_sampleDistance",
                                         this->Impl->ActualSampleDistance);

  vtkInternal::ToFloat(this->Impl->ScalarsRange, fvalue2);
  this->Impl->ShaderProgram->SetUniform2fv("in_scalarsRange", 1, &fvalue2);

  // Bind textures
  this->Impl->VolumeTextureObject->Activate();
  this->Impl->ShaderProgram->SetUniformi("in_volume",
    this->Impl->VolumeTextureObject->GetTextureUnit());

  // Opacity, color, and gradient opacity samplers / textures
  int numberOfSamplers = (independentComponents ? noOfComponents : 1);

  for (int i = 0; i < numberOfSamplers; ++i)
    {
    this->Impl->OpacityTables->GetTable(i)->Bind();
    this->Impl->ShaderProgram->SetUniformi(
      this->Impl->OpacityTablesMap[i].c_str(),
      this->Impl->OpacityTables->GetTable(i)->GetTextureUnit());

    if (this->BlendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
      {
      this->Impl->RGBTables->GetTable(i)->Bind();
      this->Impl->ShaderProgram->SetUniformi(
        this->Impl->RGBTablesMap[i].c_str(),
        this->Impl->RGBTables->GetTable(i)->GetTextureUnit());
      }

    if (this->Impl->GradientOpacityTables)
      {
      this->Impl->ShaderProgram->SetUniformi(
        this->Impl->GradientOpacityTablesMap[i].c_str(),
        this->Impl->GradientOpacityTables->GetTable(i)->GetTextureUnit());
      }
    }

  this->Impl->NoiseTextureObject->Activate();
  this->Impl->ShaderProgram->SetUniformi("in_noiseSampler",
    this->Impl->NoiseTextureObject->GetTextureUnit());

  this->Impl->DepthTextureObject->Activate();
  this->Impl->ShaderProgram->SetUniformi("in_depthSampler",
    this->Impl->DepthTextureObject->GetTextureUnit());

  if (this->Impl->CurrentMask)
    {
    this->Impl->CurrentMask->Bind();
    this->Impl->ShaderProgram->SetUniformi(
      "in_mask", this->Impl->CurrentMask->GetTextureUnit());
    }

  if(noOfComponents == 1 &&
     this->BlendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
    {
    if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
      {
      this->Impl->Mask1RGBTable->Bind();
      this->Impl->ShaderProgram->SetUniformi("in_mask1",
        this->Impl->Mask1RGBTable->GetTextureUnit());

      this->Impl->Mask2RGBTable->Bind();
      this->Impl->ShaderProgram->SetUniformi("in_mask2",
        this->Impl->Mask2RGBTable->GetTextureUnit());
      this->Impl->ShaderProgram->SetUniformf("in_maskBlendFactor",
                                             this->MaskBlendFactor);
      }
    }

  fvalue3[0] = fvalue3[1] = fvalue3[2] = volumeProperty->GetAmbient();
  this->Impl->ShaderProgram->SetUniform3f("in_ambient", fvalue3);

  fvalue3[0] = fvalue3[1] = fvalue3[2] = volumeProperty->GetDiffuse();
  this->Impl->ShaderProgram->SetUniform3f("in_diffuse", fvalue3);

  fvalue3[0] = fvalue3[1] = fvalue3[2] = volumeProperty->GetSpecular();
  this->Impl->ShaderProgram->SetUniform3f("in_specular", fvalue3);

  fvalue3[0] = volumeProperty->GetSpecularPower();
  this->Impl->ShaderProgram->SetUniformf("in_shininess", fvalue3[0]);

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
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
    "in_projectionMatrix", &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Impl->InverseProjectionMat.GetPointer(),
                             fvalue16);
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
    "in_inverseProjectionMatrix", &(fvalue16[0]));

  // Will require transpose of this matrix for OpenGL
  vtkMatrix4x4* modelviewMat4x4 =
    ren->GetActiveCamera()->GetModelViewTransformMatrix();
  this->Impl->InverseModelViewMat->DeepCopy(modelviewMat4x4);
  this->Impl->InverseModelViewMat->Invert();

  vtkInternal::VtkToGlMatrix(modelviewMat4x4, fvalue16);
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
    "in_modelViewMatrix", &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Impl->InverseModelViewMat.GetPointer(),
                             fvalue16);
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
    "in_inverseModelViewMatrix", &(fvalue16[0]));

  // Will require transpose of this matrix for OpenGL
  // Scene matrix
  vtkMatrix4x4* volumeMatrix4x4 = vol->GetMatrix();
  this->Impl->InverseVolumeMat->DeepCopy(volumeMatrix4x4);
  this->Impl->InverseVolumeMat->Invert();

  vtkInternal::VtkToGlMatrix(volumeMatrix4x4, fvalue16);
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
    "in_volumeMatrix", &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Impl->InverseVolumeMat.GetPointer(),
                             fvalue16);
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
    "in_inverseVolumeMatrix", &(fvalue16[0]));

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
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
  "in_textureDatasetMatrix", &(fvalue16[0]));

  // NOTE : VTK martices are row-major, and hence do pre-multiplication
  // of matrices
  vtkInternal::VtkToGlMatrix(
    this->Impl->InverseTextureToDataSetMat.GetPointer(), fvalue16);
  this->Impl->ShaderProgram->SetUniformMatrix4x4(
    "in_inverseTextureDatasetMatrix", &(fvalue16[0]));
  vtkMatrix4x4::Multiply4x4(volumeMatrix4x4,
                            modelviewMat4x4,
                            this->Impl->TextureToEyeTransposeInverse.GetPointer());
  vtkMatrix4x4::Multiply4x4(this->Impl->TextureToDataSetMat.GetPointer(),
                            this->Impl->TextureToEyeTransposeInverse.GetPointer(),
                            this->Impl->TextureToEyeTransposeInverse.GetPointer());
  this->Impl->TextureToEyeTransposeInverse->Invert();
  this->Impl->TextureToEyeTransposeInverse->Transpose();
  vtkInternal::VtkToGlMatrix(
    this->Impl->TextureToEyeTransposeInverse.GetPointer(), fvalue16, 3, 3);
  this->Impl->ShaderProgram->SetUniformMatrix3x3(
    "in_texureToEyeIt", &(fvalue16[0]));

  vtkInternal::ToFloat(ren->GetActiveCamera()->GetPosition(), fvalue3, 3);
  this->Impl->ShaderProgram->SetUniform3fv("in_cameraPos", 1, &fvalue3);

  vtkInternal::ToFloat(this->Impl->LoadedBounds[0],
                       this->Impl->LoadedBounds[2],
                       this->Impl->LoadedBounds[4], fvalue3);
  this->Impl->ShaderProgram->SetUniform3fv("in_volumeExtentsMin", 1, &fvalue3);

  vtkInternal::ToFloat(this->Impl->LoadedBounds[1],
                       this->Impl->LoadedBounds[3],
                       this->Impl->LoadedBounds[5], fvalue3);
  this->Impl->ShaderProgram->SetUniform3fv("in_volumeExtentsMax", 1, &fvalue3);

  vtkInternal::ToFloat(this->Impl->Extents[0],
                       this->Impl->Extents[2],
                       this->Impl->Extents[4], fvalue3);
  this->Impl->ShaderProgram->SetUniform3fv("in_textureExtentsMin", 1, &fvalue3);

  vtkInternal::ToFloat(this->Impl->Extents[1],
                       this->Impl->Extents[3],
                       this->Impl->Extents[5], fvalue3);
  this->Impl->ShaderProgram->SetUniform3fv("in_textureExtentsMax", 1, &fvalue3);

  // TODO Take consideration of reduction factor
  vtkInternal::ToFloat(this->Impl->WindowLowerLeft, fvalue2);
  this->Impl->ShaderProgram->SetUniform2fv("in_windowLowerLeftCorner", 1, &fvalue2);

  vtkInternal::ToFloat(1.0 / this->Impl->WindowSize[0],
                       1.0 / this->Impl->WindowSize[1], fvalue2);
  this->Impl->ShaderProgram->SetUniform2fv("in_inverseOriginalWindowSize", 1, &fvalue2);

  vtkInternal::ToFloat(1.0 / this->Impl->WindowSize[0],
                       1.0 / this->Impl->WindowSize[1], fvalue2);
  this->Impl->ShaderProgram->SetUniform2fv("in_inverseWindowSize", 1, &fvalue2);

  // Updating cropping if enabled
  this->Impl->UpdateCropping(ren, vol);

  // Updating clipping if enabled
  this->Impl->UpdateClipping(ren, vol);

  // Finally set the scale and bias for color correction
  this->Impl->ShaderProgram->SetUniformf("in_volumeScale", this->Impl->Scale);
  this->Impl->ShaderProgram->SetUniformf("in_volumeBias", this->Impl->Bias);

  // Finally set the scale and bias for color correction
  this->Impl->ShaderProgram->SetUniformf("in_scale",
    1.0 / this->FinalColorWindow);
  this->Impl->ShaderProgram->SetUniformf("in_bias",
    (0.5 - (this->FinalColorLevel/this->FinalColorWindow)));

  if (noOfComponents > 1 && independentComponents)
    {
    for (int i = 0; i < noOfComponents; ++i)
      {
      fvalue4[i] = static_cast<float>(volumeProperty->GetComponentWeight(i));
      }
    this->Impl->ShaderProgram->SetUniform4fv("in_componentWeight", 1, &fvalue4);
    }

#ifndef __APPLE__
  glBindVertexArray(this->Impl->CubeVAOId);
#endif
  glDrawElements(GL_TRIANGLES,
                 this->Impl->BBoxPolyData->GetNumberOfCells() * 3,
                 GL_UNSIGNED_INT, 0);

  if (volumeModified)
    {
    this->Impl->InputUpdateTime.Modified();
    }

  glFinish();

  vtkOpenGLCheckErrorMacro("failed after Render");
}
