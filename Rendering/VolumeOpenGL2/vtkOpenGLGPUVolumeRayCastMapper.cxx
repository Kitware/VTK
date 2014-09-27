/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLGPUVolumeRayCastMapper.h"

#include "vtkGLSLShader.h"
#include "vtkOpenGLOpacityTable.h"
#include "vtkOpenGLRGBTable.h"
#include "vtkOpenGLGradientOpacityTable.h"
#include "vtkVolumeShaderComposer.h"
#include "vtkVolumeStateRAII.h"

/// Include compiled shader code
#include <raycasterfs.h>
#include <raycastervs.h>

/// VTK includes
#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDensifyPolyData.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
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

/// GL includes
#include <GL/glew.h>
#include <vtkgl.h>

/// C/C++ includes
#include <cassert>
#include <string>
#include <sstream>


vtkStandardNewMacro(vtkOpenGLGPUVolumeRayCastMapper);

/// TODO Remove this afterwards
#define GL_CHECK_ERRORS \
  {\
  }

// Ref: http://stackoverflow.com/questions/22118518/use-of-undeclared-identifier-glgenvertexarrays-error-even-after-including-open
#ifdef __APPLE__
  #define glGenVertexArrays glGenVertexArraysAPPLE
  #define glBindVertexArrays glBindVertexArraysAPPLE
  #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#endif

///----------------------------------------------------------------------------
class vtkOpenGLGPUVolumeRayCastMapper::vtkInternal
{
public:
  ///
  /// \brief vtkInternal
  /// \param parent
  ///
  vtkInternal(vtkOpenGLGPUVolumeRayCastMapper* parent) :
    Initialized(false),
    ValidTransferFunction(false),
    LoadDepthTextureExtensionsSucceeded(false),
    CubeVBOId(0),
    CubeVAOId(0),
    CubeIndicesId(0),
    VolumeTextureId(0),
    NoiseTextureId(0),
    DepthTextureId(0),
    TextureWidth(1024),
    Parent(parent),
    RGBTable(0),
    Mask1RGBTable(0),
    Mask2RGBTable(0),
    OpacityTables(0),
    GradientOpacityTables(0)
    {
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
    }

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

  ///
  /// \brief Initialize
  ///
  void Initialize(vtkRenderer* ren, vtkVolume* vol);

  ///
  /// \brief vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadVolume
  /// \param imageData
  /// \param scalars
  /// \return
  ///
  bool LoadVolume(vtkImageData* imageData, vtkDataArray* scalars);

  ///
  /// \brief LoadMask
  /// \param input
  /// \param maskInput
  /// \param textureExtent
  /// \param volume
  /// \return
  ///
  bool LoadMask(vtkImageData* input,
                vtkImageData* maskInput,
                int textureExtent[6],
                vtkVolume* volume);

  ///
  /// \brief IsDataDirty
  /// \param imageData
  /// \return
  ///
  bool IsDataDirty(vtkImageData* imageData);

  ///
  /// \brief IsInitialized
  /// \return
  ///
  bool IsInitialized();


  ///
  /// \brief CompileAndLinkShader
  ///
  void CompileAndLinkShader(const string& vertexShader,
                            const string& fragmentShader);

  ///
  /// \brief ComputeBounds
  /// \param input
  ///
  void ComputeBounds(vtkImageData* input);

  ///
  /// \brief UpdateColorTransferFunction
  /// \param vol
  /// \param numberOfScalarComponents
  /// \return 0 (passed) or 1 (failed)
  ///
  /// Update transfer color function based on the incoming inputs and number of
  /// scalar components.
  ///
  /// TODO Deal with numberOfScalarComponents > 1
  int UpdateColorTransferFunction(vtkVolume* vol, int numberOfScalarComponents);

  ///
  /// \brief UpdateOpacityTransferFunction
  /// \param vol
  /// \param numberOfScalarComponents (1 or 4)
  /// \param level
  /// \return 0 (pass) or 1 (fail)
  ///
  int UpdateOpacityTransferFunction(vtkVolume* vol,
                                    int numberOfScalarComponents,
                                    unsigned int level);

  ///
  /// \brief UpdateGradientOpacityTransferFunction
  /// \param vol
  /// \param numberOfScalarComponents
  /// \param level
  /// \return 0 (pass) or 1 (fail)
  ///
  int UpdateGradientOpacityTransferFunction(vtkVolume* vol,
                                            int numberOfScalarComponents,
                                            unsigned int level);

  ///
  /// \brief UpdateNoiseTexture
  ///
  void UpdateNoiseTexture();

  ///
  /// \brief UpdateDepthTexture
  ///
  void UpdateDepthTexture(vtkRenderer* ren, vtkVolume* vol);

  ///
  /// \brief UpdateVolumeGeometry
  ///
  void UpdateVolumeGeometry();

  ///
  /// \brief Update cropping parameters to the shader
  ///
  void UpdateCropping(vtkRenderer* ren, vtkVolume* vol);

  ///
  /// \brief UpdateClipping Update clipping parameters to the shader
  /// \param ren
  /// \param vol
  ///
  void UpdateClipping(vtkRenderer* ren, vtkVolume* vol);

  ///
  /// \brief UpdateSamplingDistance Update sampling distance
  /// \param ren
  /// \param vol
  ///
  void UpdateSamplingDistance(vtkImageData *input,
                              vtkRenderer* ren, vtkVolume* vol);

  ///
  /// \brief Load OpenGL extensiosn required to grab depth sampler buffer
  ///
  void LoadRequireDepthTextureExtensions(vtkRenderWindow* renWin);

  ///
  /// Private member variables

  bool Initialized;
  bool ValidTransferFunction;
  bool LoadDepthTextureExtensionsSucceeded;

  GLuint CubeVBOId;
  GLuint CubeVAOId;
  GLuint CubeIndicesId;

  GLuint VolumeTextureId;
  GLuint NoiseTextureId;
  GLuint DepthTextureId;

  vtkGLSLShader Shader;

  int TextureWidth;

  double Scale;
  double Bias;

  float* NoiseTextureData;
  GLint NoiseTextureSize;

  float ActualSampleDistance;

  int Dimensions[3];
  int TextureSize[3];
  int WindowLowerLeft[2];
  int WindowSize[2];

  double ScalarsRange[2];
  double Bounds[6];
  int Extents[6];
  double DatasetStepSize[3];
  double CellScale[3];
  double CellStep[3];
  double CellSpacing[3];

  std::ostringstream ExtensionsStringStream;

  vtkOpenGLGPUVolumeRayCastMapper* Parent;
  vtkOpenGLRGBTable* RGBTable;
  vtkOpenGLOpacityTables* OpacityTables;
  vtkOpenGLRGBTable* Mask1RGBTable;
  vtkOpenGLRGBTable* Mask2RGBTable;
  vtkOpenGLGradientOpacityTables* GradientOpacityTables;

  vtkTimeStamp VolumeBuildTime;
  vtkTimeStamp ShaderBuildTime;

  vtkNew<vtkMatrix4x4> TextureToDataSetMat;
  vtkNew<vtkMatrix4x4> InverseTextureToDataSetMat;

  vtkNew<vtkMatrix4x4> InverseProjectionMat;
  vtkNew<vtkMatrix4x4> InverseModelViewMat;
  vtkNew<vtkMatrix4x4> InverseVolumeMat;

  vtkSmartPointer<vtkPolyData> BBoxPolyData;

  // Meed a list for AMR mode.
  vtkMapMaskTextureId* MaskTextures;
  vtkVolumeMask* CurrentMask;
};

///----------------------------------------------------------------------------
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

///----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T* in, float* out, int numberOfComponents)
{
  for (int i = 0; i < numberOfComponents; ++i)
    {
    out[i] = static_cast<float>(in[i]);
    }
}

///----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T (&in)[3], float (&out)[3])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
  out[2] = static_cast<float>(in[2]);
}

///----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T (&in)[2], float (&out)[2])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
}

///----------------------------------------------------------------------------
template<typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  T& in, float& out)
{
  out = static_cast<float>(in);
}

///----------------------------------------------------------------------------
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

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::Initialize(vtkRenderer* ren,
                                                              vtkVolume* vol)
{
  GLenum err = glewInit();
  if (GLEW_OK != err)
    {
    cerr <<"Error: "<< glewGetErrorString(err)<<endl;
    }
  else
    {
      if (GLEW_VERSION_3_3)
        {
        cout<<"Driver supports OpenGL 3.3\nDetails:"<<endl;
        }
    }
  /// This is to ignore INVALID ENUM error 1282
  err = glGetError();
  GL_CHECK_ERRORS

  // Output hardware information
  cout << "\tUsing GLEW "<< glewGetString(GLEW_VERSION) << endl;
  cout << "\tVendor: "<< glGetString (GL_VENDOR) << endl;
  cout << "\tRenderer: "<< glGetString (GL_RENDERER) << endl;
  cout << "\tVersion: "<< glGetString (GL_VERSION) << endl;
  cout << "\tGLSL: "<< glGetString (GL_SHADING_LANGUAGE_VERSION) << endl;

  /// Setup unit cube vertex array and vertex buffer objects
  glGenVertexArrays(1, &this->CubeVAOId);
  glGenBuffers(1, &this->CubeVBOId);
  glGenBuffers(1, &this->CubeIndicesId);

  /// Create RGB lookup table
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

  /// TODO Currently we are supporting only one level
  /// Create opacity lookup table
  this->OpacityTables = new vtkOpenGLOpacityTables(1);

  this->Initialized = true;
}

///----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadVolume(
  vtkImageData* imageData, vtkDataArray* scalars)
{
  GL_CHECK_ERRORS

  /// Generate OpenGL texture
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &this->VolumeTextureId);
  glBindTexture(GL_TEXTURE_3D, this->VolumeTextureId);

  /// Set the texture parameters
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  GLfloat borderColor[4]={0.0,0.0,0.0,0.0};
  glTexParameterfv(vtkgl::TEXTURE_3D,GL_TEXTURE_BORDER_COLOR, borderColor);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  GL_CHECK_ERRORS

  /// Allocate data with internal format and foramt as (GL_RED)
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
        internalFormat = vtkgl::INTENSITY16F_ARB;
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
          internalFormat=vtkgl::INTENSITY16F_ARB;
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

  /// Update scale and bias
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

    GL_CHECK_ERRORS

    /// Set scale and bias to their defaults
    glPixelTransferf(GL_RED_SCALE,1.0);
    glPixelTransferf(GL_RED_BIAS, 0.0);
    }
  else
    {
    /// Convert and send to the GPU, z-slice by z-slice so that we won't allocate
    /// memory at once.Allocate memory on the GPU (NULL data pointer with the
    /// right dimensions). Here we are assuming that GL_ARB_texture_non_power_of_two is
    /// available
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
                 this->TextureSize[0], this->TextureSize[1],
                 this->TextureSize[2], 0, format, type, 0);

    /// Send the slices one by one to the GPU. We are not sending all of them
    /// together so as to avoid allocating big data on the GPU which may not
    /// work if the original dataset is big as well.
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

  /// Update m_volume build time
  this->VolumeBuildTime.Modified();
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadMask(vtkImageData* input,
                                                           vtkImageData* maskInput,
                                                           int textureExtent[6],
                                                           vtkVolume* volume)
{
  int result = 1;

  // Mask
  if(maskInput != 0)
    {
    glActiveTexture(GL_TEXTURE7);

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
    glActiveTexture(GL_TEXTURE0);
    }

  return result;
}

///----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::IsInitialized()
{
  return this->Initialized;
}

///----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::IsDataDirty(
  vtkImageData* input)
{
  /// Check if the scalars modified time is higher than the last build time
  /// if yes, then mark the current referenced data as dirty.
  if (input->GetMTime() > this->VolumeBuildTime.GetMTime())
    {
    return true;
    }

  return false;
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CompileAndLinkShader(
  const std::string& vertexShader, const std::string& fragmentShader)
{
  this->Shader.LoadFromString(GL_VERTEX_SHADER, vertexShader);
  this->Shader.LoadFromString(GL_FRAGMENT_SHADER, fragmentShader);

  /// Compile and link the shader
  this->Shader.CreateAndLinkProgram();
}

///----------------------------------------------------------------------------
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

  /// Loaded data represents points
  if (!this->Parent->CellFlag)
    {
    // If spacing is negative, we may have to rethink the equation
    // between real point and texture coordinate...
    this->Bounds[0] = origin[0] +
      static_cast<double>(this->Extents[0 + swapBounds[0]]) * this->CellSpacing[0];
    this->Bounds[2] = origin[1] +
      static_cast<double>(this->Extents[2 + swapBounds[1]]) * this->CellSpacing[1];
    this->Bounds[4] = origin[2] +
      static_cast<double>(this->Extents[4 + swapBounds[2]]) * this->CellSpacing[2];
    this->Bounds[1] = origin[0] +
      static_cast<double>(this->Extents[1 - swapBounds[0]]) * this->CellSpacing[0];
    this->Bounds[3] = origin[1] +
      static_cast<double>(this->Extents[3 - swapBounds[1]]) * this->CellSpacing[1];
    this->Bounds[5] = origin[2] +
      static_cast<double>(this->Extents[5 - swapBounds[2]]) * this->CellSpacing[2];
    }
  /// Loaded extents represent cells
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
        this->Bounds[2 * i + swapBounds[i]] = origin[i];
        }
      else
        {
        this->Bounds[2 * i + swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i]) + 0.5) * this->CellSpacing[i];
        }

      if(this->Extents[2 * i + 1] == wholeTextureExtent[2 * i + 1])
        {
        this->Bounds[2 * i + 1 - swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i + 1]) + 1.0) * this->CellSpacing[i];
        }
      else
        {
        this->Bounds[2 * i + 1-swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i + 1]) + 0.5) * this->CellSpacing[i];
        }
      ++i;
      }
    }
}

///----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateColorTransferFunction(
  vtkVolume* vol, int numberOfScalarComponents)
{
  /// Build the colormap in a 1D texture.
  /// 1D RGB-texture=mapping from scalar values to color values
  /// build the table.
  if(numberOfScalarComponents == 1)
    {
    vtkVolumeProperty* volumeProperty = vol->GetProperty();
    vtkColorTransferFunction* colorTransferFunction =
      volumeProperty->GetRGBTransferFunction(0);

    /// Add points only if its not being added before
    if (colorTransferFunction->GetSize() < 1)
      {
      colorTransferFunction->AddRGBPoint(this->ScalarsRange[0], 0.0, 0.0, 0.0);
      colorTransferFunction->AddRGBPoint(this->ScalarsRange[1], 1.0, 1.0, 1.0);
      }

    this->RGBTable->Update(
      colorTransferFunction, this->ScalarsRange,
      volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION);

    glActiveTexture(GL_TEXTURE0);
    }

  if (this->Parent->MaskInput != 0 &&
      this->Parent->MaskType == LabelMapMaskType)
    {
    vtkVolumeProperty* volumeProperty = vol->GetProperty();

    vtkColorTransferFunction* colorTransferFunc =
      volumeProperty->GetRGBTransferFunction(1);
    this->Mask1RGBTable->Update(colorTransferFunc, this->ScalarsRange, false, 7);

    colorTransferFunc = volumeProperty->GetRGBTransferFunction(2);
    this->Mask2RGBTable->Update(colorTransferFunc, this->ScalarsRange, false, 8);

    glActiveTexture(GL_TEXTURE0);
    }

  return 0;
}

///----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateOpacityTransferFunction(
  vtkVolume* vol, int numberOfScalarComponents, unsigned int level)
{
  if (!vol)
    {
    std::cerr << "Invalid m_volume" << std::endl;
    return 1;
    }

  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  vtkPiecewiseFunction* scalarOpacity = volumeProperty->GetScalarOpacity();

  /// TODO: Do a better job to create the default opacity map
  /// Add points only if its not being added before
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

  /// Restore default active texture
  glActiveTexture(GL_TEXTURE0);

  return 0;
}

///----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::
  UpdateGradientOpacityTransferFunction(vtkVolume* vol,
    int numberOfScalarComponents, unsigned int level)
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

  /// TODO: Do a better job to create the default opacity map
  /// Add points only if its not being added before
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

  /// Restore default active texture
  glActiveTexture(GL_TEXTURE0);

  return 0;
}

///----------------------------------------------------------------------------
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
      /// -0.5 and 0.5 range
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

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateDepthTexture(
  vtkRenderer* ren, vtkVolume* vol)
{
  /// Make sure our render window is the current OpenGL context
  ren->GetRenderWindow()->MakeCurrent();

  /// Load required extensions for grabbing depth sampler buffer
  if (!this->LoadDepthTextureExtensionsSucceeded)
    {
    this->LoadRequireDepthTextureExtensions(ren->GetRenderWindow());
    }

  /// If we can't load the necessary extensions, provide
  /// feedback on why it failed.
  if(!this->LoadDepthTextureExtensionsSucceeded)
    {
    std::cerr << this->ExtensionsStringStream.str() << std::endl;
    return;
    }

  /// Now grab the depth sampler buffer as texture
  ren->GetTiledSizeAndOrigin(this->WindowSize, this->WindowSize + 1,
                             this->WindowLowerLeft, this->WindowLowerLeft + 1);

  glActiveTexture(GL_TEXTURE4);
  if (!this->DepthTextureId)
    {
    /// TODO Use framebuffer objects for best performance
    glGenTextures(1, &this->DepthTextureId);
    glBindTexture(GL_TEXTURE_2D, this->DepthTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
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
  GL_CHECK_ERRORS
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateVolumeGeometry()
{
  vtkNew<vtkTessellatedBoxSource> boxSource;
  vtkNew<vtkDensifyPolyData> densityPolyData;
  boxSource->SetBounds(this->Bounds);
  boxSource->QuadsOn();
  boxSource->SetLevel(0);

  densityPolyData->SetInputConnection(boxSource->GetOutputPort());
  densityPolyData->Update();
  densityPolyData->SetNumberOfSubdivisions(2);

  this->BBoxPolyData = densityPolyData->GetOutput();
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

  glBindVertexArray(this->CubeVAOId);

  /// Pass cube vertices to buffer object memory
  glBindBuffer (GL_ARRAY_BUFFER, this->CubeVBOId);
  glBufferData (GL_ARRAY_BUFFER, points->GetData()->GetDataSize() *
                points->GetData()->GetDataTypeSize(),
                points->GetData()->GetVoidPointer(0), GL_STATIC_DRAW);

  GL_CHECK_ERRORS

  /// Enable vertex attributre array for position
  /// and pass indices to element array  buffer
  glEnableVertexAttribArray(this->Shader["m_in_vertex_pos"]);
  glVertexAttribPointer(this->Shader["m_in_vertex_pos"], 3, GL_FLOAT, GL_FALSE, 0, 0);

  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, polys->GetDataSize() *
                polys->GetDataTypeSize(), polys->GetVoidPointer(0),
                GL_STATIC_DRAW);

  GL_CHECK_ERRORS

  glBindVertexArray(0);
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateCropping(
  vtkRenderer* ren, vtkVolume* vol)
{
  if (this->Parent->GetCropping())
    {
    int cropFlags = this->Parent->GetCroppingRegionFlags();
    double croppingRegionPlanes[6];
    this->Parent->GetCroppingRegionPlanes(croppingRegionPlanes);

    /// Clamp it
    croppingRegionPlanes[0] = croppingRegionPlanes[0] < this->Bounds[0] ?
                              this->Bounds[0] : croppingRegionPlanes[0];
    croppingRegionPlanes[0] = croppingRegionPlanes[0] > this->Bounds[1] ?
                              this->Bounds[1] : croppingRegionPlanes[0];
    croppingRegionPlanes[1] = croppingRegionPlanes[1] < this->Bounds[0] ?
                              this->Bounds[0] : croppingRegionPlanes[1];
    croppingRegionPlanes[1] = croppingRegionPlanes[1] > this->Bounds[1] ?
                              this->Bounds[1] : croppingRegionPlanes[1];

    croppingRegionPlanes[2] = croppingRegionPlanes[2] < this->Bounds[2] ?
                              this->Bounds[2] : croppingRegionPlanes[2];
    croppingRegionPlanes[2] = croppingRegionPlanes[2] > this->Bounds[3] ?
                              this->Bounds[3] : croppingRegionPlanes[2];
    croppingRegionPlanes[3] = croppingRegionPlanes[3] < this->Bounds[2] ?
                              this->Bounds[2] : croppingRegionPlanes[3];
    croppingRegionPlanes[3] = croppingRegionPlanes[3] > this->Bounds[3] ?
                              this->Bounds[3] : croppingRegionPlanes[3];

    croppingRegionPlanes[4] = croppingRegionPlanes[4] < this->Bounds[4] ?
                              this->Bounds[4] : croppingRegionPlanes[4];
    croppingRegionPlanes[4] = croppingRegionPlanes[4] > this->Bounds[5] ?
                              this->Bounds[5] : croppingRegionPlanes[4];
    croppingRegionPlanes[5] = croppingRegionPlanes[5] < this->Bounds[4] ?
                              this->Bounds[4] : croppingRegionPlanes[5];
    croppingRegionPlanes[5] = croppingRegionPlanes[5] > this->Bounds[5] ?
                              this->Bounds[5] : croppingRegionPlanes[5];

    float cropPlanes[6] = { static_cast<double>(croppingRegionPlanes[0]),
                            static_cast<double>(croppingRegionPlanes[1]),
                            static_cast<double>(croppingRegionPlanes[2]),
                            static_cast<double>(croppingRegionPlanes[3]),
                            static_cast<double>(croppingRegionPlanes[4]),
                            static_cast<double>(croppingRegionPlanes[5]) };

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

    glUniform1iv(this->Shader("cropping_flags"), numberOfRegions, cropFlagsArray);
    }
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateClipping(
  vtkRenderer* ren, vtkVolume* vol)
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

    clippingPlanes[0] = clippingPlanes.size() > 0 ? (clippingPlanes.size() - 1) :
                          0;

    glUniform1fv(this->Shader("m_clipping_planes"), clippingPlanes.size(),
                 &clippingPlanes[0]);
    }
}

///----------------------------------------------------------------------------
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
    //      this->ActualSampleDistance /= static_cast<GLfloat>(this->ReductionFactor*0.5);
    //      }
    //    }
    }
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::
  LoadRequireDepthTextureExtensions(vtkRenderWindow* renWin)
{
  /// Reset the message stream for extensions
  this->ExtensionsStringStream.str("");
  this->ExtensionsStringStream.clear();

  if (!GLEW_VERSION_2_0)
    {
    this->ExtensionsStringStream << "Requires OpenGL 2.0 or higher";
    return;
    }

  /// Check for npot even though it should be supported since
  /// it is in core since 2.0 as per specification
  if (!glewIsSupported("GL_ARB_texture_non_power_of_two"))
    {
    this->ExtensionsStringStream << "Required extension "
      << " GL_ARB_texture_non_power_of_two is not supported";
    return;
    }

  /// Check for float texture support. This extension became core
  /// in 3.0
  if (!glewIsSupported("GL_ARB_texture_float"))
    {
    this->ExtensionsStringStream << "Required extension "
      << " GL_ARB_texture_float is not supported";
    return;
    }

  /// Check for framebuffer objects. Framebuffer objects
  /// are core since version 3.0 only
  if (!glewIsSupported("GL_EXT_framebuffer_object"))
    {
    this->ExtensionsStringStream << "Required extension "
      << " GL_EXT_framebuffer_object is not supported";
    return;
    }

  /// NOTE: Support for depth sampler texture made into the core since version
  /// 1.4 and therefore we are no longer checking for it.
  this->LoadDepthTextureExtensionsSucceeded = true;
}

///----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::vtkOpenGLGPUVolumeRayCastMapper() :
  vtkGPUVolumeRayCastMapper()
{
  this->Implementation = new vtkInternal(this);
}

///
///----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::~vtkOpenGLGPUVolumeRayCastMapper()
{
  delete this->Implementation;
  this->Implementation = 0;
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  //this->Superclass->PrintSelf(os, indent);
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::BuildShader(vtkRenderer* ren,
                                                  vtkVolume* vol,
                                                  int noOfComponents)
{
  GL_CHECK_ERRORS

  vtkVolumeProperty* volProperty = vol->GetProperty();

  this->Implementation->Shader.DeleteShaderProgram();

  GL_CHECK_ERRORS

  /// Load the raycasting shader
  std::string vertexShader (raycastervs);
  std::string fragmentShader (raycasterfs);

  GL_CHECK_ERRORS

  vertexShader = vtkvolume::replace(vertexShader, "@COMPUTE_CLIP_POS@",
    vtkvolume::ComputeClip(ren, this, vol), true);
  vertexShader = vtkvolume::replace(vertexShader, "@COMPUTE_TEXTURE_COORDS@",
    vtkvolume::ComputeTextureCoords(ren, this, vol), true);

  GL_CHECK_ERRORS

  vertexShader = vtkvolume::replace(vertexShader, "@BASE_GLOBALS_VERT@",
    vtkvolume::BaseGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@BASE_GLOBALS_FRAG@",
    vtkvolume::BaseGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@BASE_INIT@",
    vtkvolume::BaseInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@BASE_INCREMENT@",
    vtkvolume::BaseIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@BASE_EXIT@",
    vtkvolume::BaseExit(ren, this, vol), true);

  fragmentShader = vtkvolume::replace(fragmentShader, "@COLOR_TRANSFER_FUNC@",
     vtkvolume::ColorTransferFunc(ren, this, vol, noOfComponents), true);

  GL_CHECK_ERRORS

  vertexShader = vtkvolume::replace(vertexShader, "@TERMINATION_GLOBALS_VERT@",
    vtkvolume::TerminationGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@TERMINATION_GLOBALS_FRAG@",
    vtkvolume::TerminationGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@TERMINATE_INIT@",
    vtkvolume::TerminationInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@TERMINATE_INCREMENT@",
    vtkvolume::TerminationIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@TERMINATE_EXIT@",
    vtkvolume::TerminationExit(ren, this, vol), true);

  vertexShader = vtkvolume::replace(vertexShader, "@SHADING_GLOBALS_VERT@",
    vtkvolume::ShadingGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@SHADING_GLOBALS_FRAG@",
    vtkvolume::ShadingGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@GRADIENT_OPACITY_GLOBALS_FRAG@",
    vtkvolume::GradientOpacityGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@SHADING_INIT@",
    vtkvolume::ShadingInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@SHADING_INCREMENT@",
    vtkvolume::ShadingIncrement(ren, this, vol, this->MaskInput,
                                this->Implementation->CurrentMask,
                                this->MaskType), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@GRADIENT_OPACITY_INCREMENT@",
    vtkvolume::GradientOpacityIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@SHADING_EXIT@",
    vtkvolume::ShadingExit(ren, this, vol), true);

  fragmentShader = vtkvolume::replace(fragmentShader, "@COMPUTE_OPACITY_FRAG@",
    vtkvolume::OpacityTransferFunc(ren, this, vol, noOfComponents), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@COMPUTE_GRADIENT_FRAG@",
    vtkvolume::GradientsComputeFunc(ren, this, vol, noOfComponents), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@COMPUTE_LIGHTING_FRAG@",
    vtkvolume::LightComputeFunc(ren, this, vol,noOfComponents), true);

  GL_CHECK_ERRORS

  vertexShader = vtkvolume::replace(vertexShader, "@CROPPING_GLOBALS_VERT@",
    vtkvolume::CroppingGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CROPPING_GLOBALS_FRAG@",
    vtkvolume::CroppingGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CROPPING_INIT@",
    vtkvolume::CroppingInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CROPPING_INCREMENT@",
    vtkvolume::CroppingIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CROPPING_EXIT@",
    vtkvolume::CroppingExit(ren, this, vol), true);

  vertexShader = vtkvolume::replace(vertexShader, "@CLIPPING_GLOBALS_VERT@",
    vtkvolume::ClippingGlobalsVert(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CLIPPING_GLOBALS_FRAG@",
    vtkvolume::ClippingGlobalsFrag(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CLIPPING_INIT@",
    vtkvolume::ClippingInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CLIPPING_INCREMENT@",
    vtkvolume::ClippingIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@CLIPPING_EXIT@",
    vtkvolume::ClippingExit(ren, this, vol), true);

  fragmentShader = vtkvolume::replace(fragmentShader, "@BINARY_MASK_GLOBALS_FRAG@",
    vtkvolume::BinaryMaskGlobalsFrag(ren, this, vol, this->MaskInput,
                                     this->Implementation->CurrentMask,
                                     this->MaskType), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@BINARY_MASK_INCREMENT@",
    vtkvolume::BinaryMaskIncrement(ren, this, vol, this->MaskInput,
                                   this->Implementation->CurrentMask,
                                   this->MaskType), true);

  fragmentShader = vtkvolume::replace(fragmentShader, "@COMPOSITE_MASK_GLOBALS_FRAG@",
    vtkvolume::CompositeMaskGlobalsFrag(ren, this, vol, this->MaskInput,
                                        this->Implementation->CurrentMask,
                                        this->MaskType), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@COMPOSITE_MASK_INCREMENT@",
    vtkvolume::CompositeMaskIncrement(ren, this, vol, this->MaskInput,
                                      this->Implementation->CurrentMask,
                                      this->MaskType), true);

  GL_CHECK_ERRORS

  /// Compile and link it
  this->Implementation->CompileAndLinkShader(vertexShader, fragmentShader);

  GL_CHECK_ERRORS

  /// Add attributes and uniforms
  this->Implementation->Shader.AddAttribute("m_in_vertex_pos");

  this->Implementation->Shader.AddUniform("m_volume_matrix");
  this->Implementation->Shader.AddUniform("m_inverse_volume_matrix");
  this->Implementation->Shader.AddUniform("m_modelview_matrix");
  this->Implementation->Shader.AddUniform("m_inverse_modelview_matrix");
  this->Implementation->Shader.AddUniform("m_projection_matrix");
  this->Implementation->Shader.AddUniform("m_inverse_projection_matrix");
  this->Implementation->Shader.AddUniform("m_texture_dataset_matrix");
  this->Implementation->Shader.AddUniform("m_inverse_texture_dataset_matrix");
  this->Implementation->Shader.AddUniform("m_volume");
  this->Implementation->Shader.AddUniform("m_camera_pos");
  this->Implementation->Shader.AddUniform("m_light_pos");
  this->Implementation->Shader.AddUniform("m_cell_step");
  this->Implementation->Shader.AddUniform("m_cell_scale");
  this->Implementation->Shader.AddUniform("m_cell_spacing");
  this->Implementation->Shader.AddUniform("m_sample_distance");
  this->Implementation->Shader.AddUniform("m_scalars_range");

  if (noOfComponents == 1 &&
      this->BlendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
    {
    this->Implementation->Shader.AddUniform("m_color_transfer_func");
    }

  this->Implementation->Shader.AddUniform("m_opacity_transfer_func");

  if (volProperty->GetGradientOpacity())
    {
    this->Implementation->Shader.AddUniform("m_gradient_transfer_func");
    }

  this->Implementation->Shader.AddUniform("m_noise_sampler");
  this->Implementation->Shader.AddUniform("m_depth_sampler");
  this->Implementation->Shader.AddUniform("m_vol_extents_min");
  this->Implementation->Shader.AddUniform("m_vol_extents_max");
  this->Implementation->Shader.AddUniform("m_texture_extents_min");
  this->Implementation->Shader.AddUniform("m_texture_extents_max");
  this->Implementation->Shader.AddUniform("m_ambient");
  this->Implementation->Shader.AddUniform("m_diffuse");
  this->Implementation->Shader.AddUniform("m_specular");
  this->Implementation->Shader.AddUniform("m_shininess");
  this->Implementation->Shader.AddUniform("m_window_lower_left_corner");
  this->Implementation->Shader.AddUniform("m_inv_original_window_size");
  this->Implementation->Shader.AddUniform("m_inv_window_size");

  GL_CHECK_ERRORS

  if (this->GetCropping())
    {
    this->Implementation->Shader.AddUniform("cropping_planes");
    this->Implementation->Shader.AddUniform("cropping_flags");
    }

  GL_CHECK_ERRORS

  if (this->GetClippingPlanes())
    {
    this->Implementation->Shader.AddUniform("m_clipping_planes");
    this->Implementation->Shader.AddUniform("m_clipping_planes_size");
    }

  if (this->Implementation->CurrentMask)
    {
    this->Implementation->Shader.AddUniform("m_mask");
    }

  if (this->MaskInput && this->MaskType == LabelMapMaskType)
    {
      this->Implementation->Shader.AddUniform("m_mask_1");
      this->Implementation->Shader.AddUniform("m_mask_2");
      this->Implementation->Shader.AddUniform("m_mask_blendfactor");
    }

  GL_CHECK_ERRORS

  std::cerr << "shader " << fragmentShader << std::endl;

  this->Implementation->ShaderBuildTime.Modified();
}

///----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GPURender(vtkRenderer* ren, vtkVolume* vol)
{
  /// Make sure the context is current
  ren->GetRenderWindow()->MakeCurrent();

  /// Update m_volume first to make sure states are current
  vol->Update();

  vtkImageData* input = this->GetInput();

  /// Set OpenGL states
  vtkVolumeStateRAII glState;

  if (!this->Implementation->IsInitialized())
    {
    this->Implementation->Initialize(ren, vol);
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
    scalars->GetRange(this->Implementation->ScalarsRange);
    }
  // If it is 3, then use the 4th component's range since that is
  // the component that will be passed through the scalar opacity
  // transfer function to look up opacity
  else
    {
    // Note that we've already checked data type and we know this is
    // unsigned char
    scalars->GetRange(this->Implementation->ScalarsRange, 3);
    }

  ///  volume if needed
  if (this->Implementation->IsDataDirty(input))
    {
    input->GetDimensions(this->Implementation->Dimensions);

    /// Update bounds, data, and geometry
    this->Implementation->ComputeBounds(input);
    this->Implementation->LoadVolume(input, scalars);
    this->Implementation->LoadMask(input, this->MaskInput,
                                   this->Implementation->Extents, vol);
    this->Implementation->UpdateVolumeGeometry();
    }

  /// Mask
  vtkVolumeMask* mask = 0;
  if(this->MaskInput != 0)
    {
    std::map<vtkImageData *,vtkVolumeMask*>::iterator it2 =
      this->Implementation->MaskTextures->Map.find(this->MaskInput);
    if(it2 == this->Implementation->MaskTextures->Map.end())
      {
      mask=0;
      }
    else
      {
      mask=(*it2).second;
      }
    }
  this->Implementation->CurrentMask = mask;
  if (this->Implementation->CurrentMask != 0)
   {
   this->Implementation->CurrentMask->Bind();
   }

  this->Implementation->UpdateSamplingDistance(input, ren, vol);

  /// Build shader
  if (vol->GetProperty()->GetMTime() >
      this->Implementation->ShaderBuildTime.GetMTime() ||
      this->GetMTime() > this->Implementation->ShaderBuildTime)
    {
    this->BuildShader(ren, vol, numberOfScalarComponents);
    }

  /// Update opacity transfer function
  /// TODO Passing level 0 for now
  this->Implementation->UpdateOpacityTransferFunction(vol,
    scalars->GetNumberOfComponents(), 0);

  this->Implementation->UpdateGradientOpacityTransferFunction(vol,
    scalars->GetNumberOfComponents(), 0);

  /// Update transfer color functions
  this->Implementation->UpdateColorTransferFunction(vol,
    scalars->GetNumberOfComponents());

  /// Update noise sampler texture
  this->Implementation->UpdateNoiseTexture();

  /// Grab depth sampler buffer (to handle cases when we are rendering geometry
  /// and m_volume together
  this->Implementation->UpdateDepthTexture(ren, vol);

  GL_CHECK_ERRORS

  // Temporary variables
  float fvalue;
  float fvalue2[2];
  float fvalue3[3];
  float fvalue16[16];

  /// Update sampling distance
  int* loadedExtent = input->GetExtent();

  this->Implementation->CellScale[0] =
    (static_cast<double>(loadedExtent[1] - loadedExtent[0]) * 0.5);
  this->Implementation->CellScale[1] =
    (static_cast<double>(loadedExtent[3] - loadedExtent[2])* 0.5);
  this->Implementation->CellScale[2] =
    (static_cast<double>(loadedExtent[5] - loadedExtent[4]) * 0.5);

  this->Implementation->CellStep[0] =
    (1.0/static_cast<double>(loadedExtent[1]-loadedExtent[0]));
  this->Implementation->CellStep[1] =
    (1.0/static_cast<double>(loadedExtent[3]-loadedExtent[2]));
  this->Implementation->CellStep[2] =
    (1.0/static_cast<double>(loadedExtent[5]-loadedExtent[4]));

  this->Implementation->DatasetStepSize[0] = 1.0 / (this->Bounds[1] - this->Bounds[0]);
  this->Implementation->DatasetStepSize[1] = 1.0 / (this->Bounds[3] - this->Bounds[2]);
  this->Implementation->DatasetStepSize[2] = 1.0 / (this->Bounds[5] - this->Bounds[4]);

  this->Implementation->CellScale[0] = (this->Bounds[1] - this->Bounds[0]) * 0.5;
  this->Implementation->CellScale[1] = (this->Bounds[3] - this->Bounds[2]) * 0.5;
  this->Implementation->CellScale[2] = (this->Bounds[5] - this->Bounds[4]) * 0.5;

  /// Now use the shader
  ///
  this->Implementation->Shader.Use();

  /// Pass constant uniforms at initialization
  /// Step should be dependant on the bounds and not on the texture size
  /// since we can have non uniform voxel size / spacing / aspect ratio
  vtkInternal::ToFloat(this->Implementation->CellStep, fvalue3);
  glUniform3f(this->Implementation->Shader("m_cell_step"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  vtkInternal::ToFloat(this->Implementation->CellScale, fvalue3);
  glUniform3f(this->Implementation->Shader("m_cell_scale"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  vtkInternal::ToFloat(this->Implementation->CellSpacing, fvalue3);
  glUniform3f(this->Implementation->Shader("m_cell_spacing"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  glUniform1f(this->Implementation->Shader("m_sample_distance"),
              this->Implementation->ActualSampleDistance);

  vtkInternal::ToFloat(this->Implementation->ScalarsRange, fvalue2);
  glUniform2f(this->Implementation->Shader("m_scalars_range"),
              fvalue2[0], fvalue2[1]);

  glUniform1i(this->Implementation->Shader("m_volume"), 0);
  glUniform1i(this->Implementation->Shader("m_opacity_transfer_func"), 2);
  glUniform1i(this->Implementation->Shader("m_noise_sampler"), 3);
  glUniform1i(this->Implementation->Shader("m_depth_sampler"), 4);
  glUniform1i(this->Implementation->Shader("m_gradient_transfer_func"), 5);

  if (this->Implementation->CurrentMask)
    {
    glUniform1i(this->Implementation->Shader("m_mask"), 6);
    }

  if(numberOfScalarComponents == 1 &&
     this->BlendMode!=vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
    {
    glUniform1i(this->Implementation->Shader("m_color_transfer_func"), 1);

    if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
      {
      glUniform1i(this->Implementation->Shader("m_mask_1"), 7);
      glUniform1i(this->Implementation->Shader("m_mask_2"), 8);
      glUniform1f(this->Implementation->Shader("m_mask_blendfactor"),
                  this->MaskBlendFactor);
      }
    }

  fvalue3[0] = fvalue3[1] = fvalue3[2] = vol->GetProperty()->GetAmbient();
  glUniform3f(this->Implementation->Shader("m_ambient"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  fvalue3[0] = fvalue3[1] = fvalue3[2] = vol->GetProperty()->GetDiffuse();
  glUniform3f(this->Implementation->Shader("m_diffuse"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  fvalue3[0] = fvalue3[1] = fvalue3[2] = vol->GetProperty()->GetSpecular();
  glUniform3f(this->Implementation->Shader("m_specular"),
              fvalue3[0], fvalue3[1], fvalue3[2]);

  fvalue3[0] = vol->GetProperty()->GetSpecularPower();
  glUniform1f(this->Implementation->Shader("m_shininess"), fvalue3[0]);

  /// Bind textures
  /// Volume texture is at unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, this->Implementation->VolumeTextureId);

  /// Color texture is at unit 1
  if (numberOfScalarComponents == 1)
    {
    this->Implementation->RGBTable->Bind();

    if (this->MaskInput != 0 && this->MaskType == LabelMapMaskType)
      {
      this->Implementation->Mask1RGBTable->Bind(7);
      this->Implementation->Mask2RGBTable->Bind(8);
      }
    }

  /// Opacity texture is at unit 2
  /// TODO Supports only one table for now
  this->Implementation->OpacityTables->GetTable(0)->Bind();

  /// Noise texture is at unit 3
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, this->Implementation->NoiseTextureId);

  /// Depth texture is at unit 4
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, this->Implementation->DepthTextureId);

  /// Look at the OpenGL Camera for the exact aspect computation
  double aspect[2];
  ren->ComputeAspect();
  ren->GetAspect(aspect);

  double clippingRange[2];
  ren->GetActiveCamera()->GetClippingRange(clippingRange);

  /// Will require transpose of this matrix for OpenGL
  vtkMatrix4x4* projectionMat4x4 = ren->GetActiveCamera()->
    GetProjectionTransformMatrix(aspect[0]/aspect[1], -1, 1);
  this->Implementation->InverseProjectionMat->DeepCopy(projectionMat4x4);
  this->Implementation->InverseProjectionMat->Invert();
  vtkInternal::VtkToGlMatrix(projectionMat4x4, fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_projection_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Implementation->InverseProjectionMat.GetPointer(), fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_inverse_projection_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  /// Will require transpose of this matrix for OpenGL
  vtkMatrix4x4* modelviewMat4x4 = ren->GetActiveCamera()->GetViewTransformMatrix();
  this->Implementation->InverseModelViewMat->DeepCopy(modelviewMat4x4);
  this->Implementation->InverseModelViewMat->Invert();

  vtkInternal::VtkToGlMatrix(modelviewMat4x4, fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_modelview_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Implementation->InverseModelViewMat.GetPointer(), fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_inverse_modelview_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  /// Will require transpose of this matrix for OpenGL
  /// Scene matrix
  float volumeMat[16];
  vtkMatrix4x4* volumeMatrix4x4 = vol->GetMatrix();
  this->Implementation->InverseVolumeMat->DeepCopy(volumeMatrix4x4);
  this->Implementation->InverseVolumeMat->Invert();

  vtkInternal::VtkToGlMatrix(volumeMatrix4x4, fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_volume_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::VtkToGlMatrix(this->Implementation->InverseVolumeMat.GetPointer(), fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_inverse_volume_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  /// Compute texture to dataset matrix
  this->Implementation->TextureToDataSetMat->Identity();
  this->Implementation->TextureToDataSetMat->SetElement(0, 0,
    (1.0 / this->Implementation->DatasetStepSize[0]));
  this->Implementation->TextureToDataSetMat->SetElement(1, 1,
    (1.0 / this->Implementation->DatasetStepSize[1]));
  this->Implementation->TextureToDataSetMat->SetElement(2, 2,
    (1.0 / this->Implementation->DatasetStepSize[2]));
  this->Implementation->TextureToDataSetMat->SetElement(3, 3,
    1.0);
  this->Implementation->TextureToDataSetMat->SetElement(0, 3,
    this->Implementation->Bounds[0]);
  this->Implementation->TextureToDataSetMat->SetElement(1, 3,
    this->Implementation->Bounds[2]);
  this->Implementation->TextureToDataSetMat->SetElement(2, 3,
    this->Implementation->Bounds[4]);

  this->Implementation->InverseTextureToDataSetMat->DeepCopy(
    this->Implementation->TextureToDataSetMat.GetPointer());
  this->Implementation->InverseTextureToDataSetMat->Invert();
  vtkInternal::VtkToGlMatrix(this->Implementation->TextureToDataSetMat.GetPointer(), fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_texture_dataset_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));
  vtkInternal::VtkToGlMatrix(this->Implementation->InverseTextureToDataSetMat.GetPointer(), fvalue16);
  glUniformMatrix4fv(this->Implementation->Shader("m_inverse_texture_dataset_matrix"), 1,
                     GL_FALSE, &(fvalue16[0]));

  vtkInternal::ToFloat(ren->GetActiveCamera()->GetPosition(), fvalue3, 3);
  glUniform3fv(this->Implementation->Shader("m_camera_pos"), 1, &(fvalue3[0]));

  /// NOTE Assuming that light is located on the camera
  glUniform3fv(this->Implementation->Shader("m_light_pos"), 1, &(fvalue3[0]));

  float volExtentsMin[3] = {this->Bounds[0], this->Bounds[2], this->Bounds[4]};
  float volExtentsMax[3] = {this->Bounds[1], this->Bounds[3], this->Bounds[5]};
  glUniform3fv(this->Implementation->Shader("m_vol_extents_min"), 1,
               &(volExtentsMin[0]));
  glUniform3fv(this->Implementation->Shader("m_vol_extents_max"), 1,
               &(volExtentsMax[0]));

  vtkInternal::ToFloat(this->Implementation->Extents[0],
                       this->Implementation->Extents[2],
                       this->Implementation->Extents[4], fvalue3);
  glUniform3fv(this->Implementation->Shader("m_texture_extents_min"), 1,
               &(fvalue3[0]));
  vtkInternal::ToFloat(this->Implementation->Extents[1],
                       this->Implementation->Extents[3],
                       this->Implementation->Extents[5], fvalue3);
  glUniform3fv(this->Implementation->Shader("m_texture_extents_max"), 1,
               &(fvalue3[0]));

  /// TODO Take consideration of reduction factor
  vtkInternal::ToFloat(this->Implementation->WindowLowerLeft, fvalue2);
  glUniform2fv(this->Implementation->Shader("m_window_lower_left_corner"),
               1, &fvalue2[0]);

  vtkInternal::ToFloat(1.0 / this->Implementation->WindowSize[0],
                       1.0 / this->Implementation->WindowSize[1], fvalue2);
  glUniform2fv(this->Implementation->Shader("m_inv_original_window_size"),
               1, &fvalue2[0]);

  vtkInternal::ToFloat(1.0 / this->Implementation->WindowSize[0],
                       1.0 / this->Implementation->WindowSize[1], fvalue2);
  glUniform2fv(this->Implementation->Shader("m_inv_window_size"), 1, &fvalue2[0]);

  /// Updating cropping if enabled
  this->Implementation->UpdateCropping(ren, vol);

  /// Updating clipping if enabled
  this->Implementation->UpdateClipping(ren, vol);

  glBindVertexArray(this->Implementation->CubeVAOId);
  glDrawElements(GL_TRIANGLES,
                 this->Implementation->BBoxPolyData->GetNumberOfCells() * 3,
                 GL_UNSIGNED_INT, 0);

  /// Undo binds and state changes
  /// TODO Provide a stack implementation
  this->Implementation->Shader.UnUse();
}
