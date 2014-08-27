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

#include "vtkSinglePassVolumeMapper.h"

#include "vtkGLSLShader.h"
#include "vtkOpenGLOpacityTable.h"
#include "vtkOpenGLRGBTable.h"
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
#include <vtkVolumeProperty.h>

/// GL includes
#include <GL/glew.h>
#include <vtkgl.h>

/// C/C++ includes
#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkSinglePassVolumeMapper);

/// TODO Remove this afterwards
#define GL_CHECK_ERRORS \
  {\
  assert(glGetError()== GL_NO_ERROR); \
  }

///
/// \brief The vtkSinglePassVolumeMapper::vtkInternal class
///
///----------------------------------------------------------------------------
class vtkSinglePassVolumeMapper::vtkInternal
{
public:
  ///
  /// \brief vtkInternal
  /// \param parent
  ///
  vtkInternal(vtkSinglePassVolumeMapper* parent) :
    Initialized(false),
    ValidTransferFunction(false),
    LoadDepthTextureExtensionsSucceeded(false),
    CubeVBOId(0),
    CubeVAOId(0),
    CubeIndicesId(0),
    VolumeTextureId(0),
    NoiseTextureId(0),
    DepthTextureId(0),
    CellFlag(0),
    TextureWidth(1024),
    Parent(parent),
    RGBTable(0)
    {
    this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = -1;
    this->TextureSize[0] = this->TextureSize[1] = this->TextureSize[2] = -1;
    this->CellScale[0] = this->CellScale[1] = this->CellScale[2] = 0.0;
    this->NoiseTextureData = 0;

    this->Extents[0]=VTK_INT_MAX;
    this->Extents[1]=VTK_INT_MIN;
    this->Extents[2]=VTK_INT_MAX;
    this->Extents[3]=VTK_INT_MIN;
    this->Extents[4]=VTK_INT_MAX;
    this->Extents[5]=VTK_INT_MIN;
    }

  ~vtkInternal()
    {
    delete this->RGBTable;
    this->RGBTable = 0;

    delete this->OpacityTables;
    this->OpacityTables = 0;

    delete this->NoiseTextureData;
    this->NoiseTextureData = 0;
    }

  ///
  /// \brief Initialize
  ///
  void Initialize(vtkRenderer* ren, vtkVolume* vol);

  ///
  /// \brief vtkSinglePassVolumeMapper::vtkInternal::LoadVolume
  /// \param imageData
  /// \param scalars
  /// \return
  ///
  bool LoadVolume(vtkImageData* imageData, vtkDataArray* scalars);

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
  /// \return 0 or 1 (fail)
  ///
  int UpdateOpacityTransferFunction(vtkVolume* vol, int numberOfScalarComponents,
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

  int CellFlag;
  int Dimensions[3];
  int TextureSize[3];
  int TextureExtents[6];
  int WindowLowerLeft[2];
  int WindowSize[2];
  int TextureWidth;

  double ScalarsRange[2];
  double Bounds[6];
  int Extents[6];
  double StepSize[3];
  double CellScale[3];
  double Scale;
  double Bias;
  double ElapsedDrawTime;

  float* NoiseTextureData;
  GLint NoiseTextureSize;

  std::ostringstream ExtensionsStringStream;

  vtkSinglePassVolumeMapper* Parent;
  vtkOpenGLRGBTable* RGBTable;
  vtkOpenGLOpacityTables* OpacityTables;

  vtkTimeStamp VolumeBuildTime;
  vtkTimeStamp ShaderBuildTime;

  vtkNew<vtkTimerLog> Timer;

  vtkNew<vtkMatrix4x4> TextureToDataSetMat;

  vtkSmartPointer<vtkPolyData> BBoxPolyData;
};

///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::Initialize(vtkRenderer* ren,
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

  //output hardware information
  cout<<"\tUsing GLEW "<< glewGetString(GLEW_VERSION)<<endl;
  cout<<"\tVendor: "<< glGetString (GL_VENDOR)<<endl;
  cout<<"\tRenderer: "<< glGetString (GL_RENDERER)<<endl;
  cout<<"\tVersion: "<< glGetString (GL_VERSION)<<endl;
  cout<<"\tGLSL: "<< glGetString (GL_SHADING_LANGUAGE_VERSION)<<endl;

  /// Setup unit cube vertex array and vertex buffer objects
  glGenVertexArrays(1, &this->CubeVAOId);
  glGenBuffers(1, &this->CubeVBOId);
  glGenBuffers(1, &this->CubeIndicesId);

  /// Create RGB lookup table
  this->RGBTable = new vtkOpenGLRGBTable();

  /// TODO Currently we are supporting only one level
  /// Create opacity lookup table
  this->OpacityTables = new vtkOpenGLOpacityTables(1);

  this->Initialized = true;
}

///----------------------------------------------------------------------------
bool vtkSinglePassVolumeMapper::vtkInternal::LoadVolume(vtkImageData* imageData,
                                                        vtkDataArray* scalars)
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
        scale = VTK_SIGNED_CHAR_MAX/(this->ScalarsRange[1] - this->ScalarsRange[0]);
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
        scale = VTK_SHORT_MAX / (this->ScalarsRange[1] - this->ScalarsRange[0]);
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
          VTK_UNSIGNED_SHORT_MAX/(this->ScalarsRange[1]-this->ScalarsRange[0]);
        break;
      case VTK_UNSIGNED_INT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_UNSIGNED_INT;
        shift=-this->ScalarsRange[0]/VTK_UNSIGNED_INT_MAX;
        scale = VTK_UNSIGNED_INT_MAX/(this->ScalarsRange[1]-this->ScalarsRange[0]);
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
                 this->TextureSize[0],this->TextureSize[1],this->TextureSize[2], 0,
                 format, type, dataPtr);

    GL_CHECK_ERRORS

    /// Set scale and bias to their defaults
    glPixelTransferf(GL_RED_SCALE,1.0);
    glPixelTransferf(GL_RED_BIAS,0.0);
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
    int kInc = (this->Dimensions[0] - this->CellFlag) *
               (this->Dimensions[1] - this->CellFlag);
    int kOffset = (this->TextureExtents[4] *  (this->Dimensions[1] - this->CellFlag) +
                   this->TextureExtents[2]) * (this->Dimensions[0] - this->CellFlag) +
                   this->TextureExtents[0];
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
        jOffset += this->Dimensions[0] - this->CellFlag;
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

///
/// \brief vtkSinglePassVolumeMapper::vtkInternal::IsInitialized
/// \return
///----------------------------------------------------------------------------
bool vtkSinglePassVolumeMapper::vtkInternal::IsInitialized()
{
  return this->Initialized;
}

///
/// \brief vtkSinglePassVolumeMapper::vtkInternal::IsDataDirty
/// \param input
/// \return
///----------------------------------------------------------------------------
bool vtkSinglePassVolumeMapper::vtkInternal::IsDataDirty(vtkImageData* input)
{
  /// Check if the scalars modified time is higher than the last build time
  /// if yes, then mark the current referenced data as dirty.
  if (input->GetMTime() > this->VolumeBuildTime.GetMTime())
    {
    return true;
    }

  return false;
}

///
/// \brief vtkSinglePassVolumeMapper::vtkInternal::CompileAndLinkShader
///
///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::CompileAndLinkShader(
  const std::string& vertexShader, const std::string& fragmentShader)
{
  this->Shader.LoadFromString(GL_VERTEX_SHADER, vertexShader);
  this->Shader.LoadFromString(GL_FRAGMENT_SHADER, fragmentShader);

  /// Compile and link the shader
  this->Shader.CreateAndLinkProgram();
}

///
/// \brief vtkSinglePassVolumeMapper::vtkInternal::ComputeBounds
/// \param bounds
///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::ComputeBounds(vtkImageData* input)
{
  double spacing[3];
  double origin[3];

  input->GetSpacing(spacing);
  input->GetOrigin(origin);
  input->GetExtent(this->Extents);

  int swapBounds[3];
  swapBounds[0] = (spacing[0] < 0);
  swapBounds[1] = (spacing[1] < 0);
  swapBounds[2] = (spacing[2] < 0);

  /// Loaded data represents points
  if (!this->CellFlag)
    {
    // If spacing is negative, we may have to rethink the equation
    // between real point and texture coordinate...
    this->Bounds[0] = origin[0] +
      static_cast<double>(this->Extents[0 + swapBounds[0]]) * spacing[0];
    this->Bounds[2] = origin[1] +
      static_cast<double>(this->Extents[2 + swapBounds[1]]) * spacing[1];
    this->Bounds[4] = origin[2] +
      static_cast<double>(this->Extents[4 + swapBounds[2]]) * spacing[2];
    this->Bounds[1] = origin[0] +
      static_cast<double>(this->Extents[1 - swapBounds[0]]) * spacing[0];
    this->Bounds[3] = origin[1] +
      static_cast<double>(this->Extents[3 - swapBounds[1]]) * spacing[1];
    this->Bounds[5] = origin[2] +
      static_cast<double>(this->Extents[5 - swapBounds[2]]) * spacing[2];
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
          (static_cast<double>(this->Extents[2 * i]) + 0.5) * spacing[i];
        }

      if(this->Extents[2 * i + 1] == wholeTextureExtent[2 * i + 1])
        {
        this->Bounds[2 * i + 1 - swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i + 1]) + 1.0) * spacing[i];
        }
      else
        {
        this->Bounds[2 * i + 1-swapBounds[i]] = origin[i] +
          (static_cast<double>(this->Extents[2 * i + 1]) + 0.5) * spacing[i];
        }
      ++i;
      }
    }
}

///----------------------------------------------------------------------------
int vtkSinglePassVolumeMapper::vtkInternal::UpdateColorTransferFunction(
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
  else
    {
    std::cerr << "SinglePass m_volume mapper does not handle multi-component scalars";
    return 1;
    }

  return 0;
}

///----------------------------------------------------------------------------
int vtkSinglePassVolumeMapper::vtkInternal::UpdateOpacityTransferFunction(
  vtkVolume* vol, int numberOfScalarComponents, unsigned int level)
{
  if (!vol)
    {
    std::cerr << "Invalid m_volume" << std::endl;
    return 1;
    }

  if (numberOfScalarComponents != 1)
    {
    std::cerr << "SinglePass m_volume mapper does not handle multi-component scalars";
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
    this->Parent->SampleDistance,
    this->ScalarsRange,
    volumeProperty->GetScalarOpacityUnitDistance(),
    volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION);

  /// Restore default active texture
  glActiveTexture(GL_TEXTURE0);

  return 0;
}

///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::UpdateNoiseTexture()
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
void vtkSinglePassVolumeMapper::vtkInternal::UpdateDepthTexture(
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
void vtkSinglePassVolumeMapper::vtkInternal::UpdateVolumeGeometry()
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
/// \brief vtkSinglePassVolumeMapper::UpdateCropping
void vtkSinglePassVolumeMapper::vtkInternal::UpdateCropping(vtkRenderer* ren,
                                                            vtkVolume* vol)
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
    glUniform1i(this->Shader("cropping_flags"), cropFlags);
    }
}

///----------------------------------------------------------------------------
/// \brief vtkSinglePassVolumeMapper::UpdateClipping
void vtkSinglePassVolumeMapper::vtkInternal::UpdateClipping(vtkRenderer* ren,
                                                            vtkVolume* vol)
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

///
/// \brief vtkSinglePassVolumeMapper::vtkSinglePassVolumeMapper
///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::LoadRequireDepthTextureExtensions(
  vtkRenderWindow* renWin)
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

///
/// \brief vtkSinglePassVolumeMapper::vtkSinglePassVolumeMapper
///----------------------------------------------------------------------------
vtkSinglePassVolumeMapper::vtkSinglePassVolumeMapper() : vtkVolumeMapper()
{
  this->SampleDistance = 1.0;

  this->Implementation = new vtkInternal(this);
}

///
/// \brief vtkSinglePassVolumeMapper::~vtkSinglePassVolumeMapper
///----------------------------------------------------------------------------
vtkSinglePassVolumeMapper::~vtkSinglePassVolumeMapper()
{
  delete this->Implementation;
  this->Implementation = 0;
}

///
/// \brief vtkSinglePassVolumeMapper::PrintSelf
/// \param os
/// \param indent
///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::PrintSelf(ostream& vtkNotUsed(os),
                                          vtkIndent indent)
{
  // TODO Implement this method
}

///
/// \brief vtkSinglePassVolumeMapper::BuildShader
/// \param vertexShader
/// \param fragmentShader
/// \param ren
/// \param vol
///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::BuildShader(vtkRenderer* ren, vtkVolume* vol)
{
  GL_CHECK_ERRORS

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
  fragmentShader = vtkvolume::replace(fragmentShader, "@SHADING_INIT@",
    vtkvolume::ShadingInit(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@SHADING_INCREMENT@",
    vtkvolume::ShadingIncrement(ren, this, vol), true);
  fragmentShader = vtkvolume::replace(fragmentShader, "@SHADING_EXIT@",
    vtkvolume::ShadingExit(ren, this, vol), true);

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

  GL_CHECK_ERRORS

  /// Compile and link it
  this->Implementation->CompileAndLinkShader(vertexShader, fragmentShader);

  GL_CHECK_ERRORS

  /// Add attributes and uniforms
  this->Implementation->Shader.AddAttribute("m_in_vertex_pos");

  this->Implementation->Shader.AddUniform("m_scene_matrix");
  this->Implementation->Shader.AddUniform("m_modelview_matrix");
  this->Implementation->Shader.AddUniform("m_projection_matrix");
  this->Implementation->Shader.AddUniform("m_texture_dataset_matrix");
  this->Implementation->Shader.AddUniform("m_volume");
  this->Implementation->Shader.AddUniform("m_camera_pos");
  this->Implementation->Shader.AddUniform("m_light_pos");
  this->Implementation->Shader.AddUniform("m_step_size");
  this->Implementation->Shader.AddUniform("m_sample_distance");
  this->Implementation->Shader.AddUniform("m_scale");
  this->Implementation->Shader.AddUniform("m_bias");
  this->Implementation->Shader.AddUniform("m_cell_scale");
  this->Implementation->Shader.AddUniform("m_color_transfer_func");
  this->Implementation->Shader.AddUniform("m_opacity_transfer_func");
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

  GL_CHECK_ERRORS

  this->Implementation->ShaderBuildTime.Modified();
}

///
/// \brief vtkSinglePassVolumeMapper::ValidateRender
/// \param ren
/// \param vol
/// \return
///----------------------------------------------------------------------------
int vtkSinglePassVolumeMapper::ValidateRender(vtkRenderer* ren, vtkVolume* vol)
{
  /// Check that we have everything we need to render.
  int goodSoFar = 1;

  /// Check for a renderer - we MUST have one
  if (!ren)
    {
    goodSoFar = 0;
    vtkErrorMacro("Renderer cannot be null.");
    }

  /// Check for the m_volume - we MUST have one
  if (goodSoFar && !vol)
    {
    goodSoFar = 0;
    vtkErrorMacro("Volume cannot be null.");
    }

  /// Don't need to check if we have a m_volume property
  /// since the m_volume will create one if we don't. Also
  /// don't need to check for the scalar opacity function
  /// or the RGB transfer function since the property will
  /// create them if they do not yet exist.

  /// TODO: Enable cropping planes
  /// \see vtkGPUVolumeRayCastMapper

  /// Check that we have input data
  vtkImageData* input = this->GetInput();
  if (goodSoFar && input == 0)
    {
    vtkErrorMacro("Input is NULL but is required");
    goodSoFar = 0;
    }

  if (goodSoFar)
    {
    this->GetInputAlgorithm()->Update();
    }

  /// TODO:
  /// Check if we need to do workaround to handle extents starting from non-zero
  /// values.
  /// \see vtkGPUVolumeRayCastMapper

  /// Update the date then make sure we have scalars. Note
  /// that we must have point or cell scalars because field
  /// scalars are not supported.
  vtkDataArray* scalars = NULL;
  if (goodSoFar)
    {
    /// Now make sure we can find scalars
    scalars=this->GetScalars(input,this->ScalarMode,
                             this->ArrayAccessMode,
                             this->ArrayId,
                             this->ArrayName,
                             this->Implementation->CellFlag);

    /// We couldn't find scalars
    if (!scalars)
      {
      vtkErrorMacro("No scalars found on input.");
      goodSoFar = 0;
      }
    /// Even if we found scalars, if they are field data scalars that isn't good
    else if (this->Implementation->CellFlag == 2)
      {
      vtkErrorMacro("Only point or cell scalar support - found field scalars instead.");
      goodSoFar = 0;
      }
    }

  /// Make sure the scalar type is actually supported. This mappers supports
  /// almost all standard scalar types.
  if (goodSoFar)
    {
    switch(scalars->GetDataType())
      {
      case VTK_CHAR:
        vtkErrorMacro(<< "scalar of type VTK_CHAR is not supported "
                      << "because this type is platform dependent. "
                      << "Use VTK_SIGNED_CHAR or VTK_UNSIGNED_CHAR instead.");
        goodSoFar = 0;
        break;
      case VTK_BIT:
        vtkErrorMacro("scalar of type VTK_BIT is not supported by this mapper.");
        goodSoFar = 0;
        break;
      case VTK_ID_TYPE:
        vtkErrorMacro("scalar of type VTK_ID_TYPE is not supported by this mapper.");
        goodSoFar = 0;
        break;
      case VTK_STRING:
        vtkErrorMacro("scalar of type VTK_STRING is not supported by this mapper.");
        goodSoFar = 0;
        break;
      default:
        /// Don't need to do anything here
        break;
      }
    }

  /// Check on the blending type - we support composite and min / max intensity
  if (goodSoFar)
    {
    if (this->BlendMode != vtkVolumeMapper::COMPOSITE_BLEND &&
        this->BlendMode != vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND &&
        this->BlendMode != vtkVolumeMapper::MINIMUM_INTENSITY_BLEND &&
        this->BlendMode != vtkVolumeMapper::ADDITIVE_BLEND)
      {
      goodSoFar = 0;
      vtkErrorMacro(<< "Selected blend mode not supported. "
                    << "Only Composite, MIP, MinIP and additive modes "
                    << "are supported by the current implementation.");
      }
    }

  /// This mapper supports 1 component data, or 4 component if it is not independent
  /// component (i.e. the four components define RGBA)
  int numberOfComponents = 0;
  if (goodSoFar)
    {
    numberOfComponents=scalars->GetNumberOfComponents();
    if (!(numberOfComponents==1 ||
          (numberOfComponents==4 &&
           vol->GetProperty()->GetIndependentComponents()==0)))
      {
      goodSoFar = 0;
      vtkErrorMacro(<< "Only one component scalars, or four "
                    << "component with non-independent components, "
                    << "are supported by this mapper.");
      }
    }

  /// If this is four component data, then it better be unsigned char (RGBA).
  /// TODO: Check on this condition
  if (goodSoFar &&
      numberOfComponents == 4 &&
      scalars->GetDataType() != VTK_UNSIGNED_CHAR)
    {
    goodSoFar = 0;
    vtkErrorMacro("Only unsigned char is supported for 4-component scalars!");
    }

  if (goodSoFar && numberOfComponents!=1 &&
     this->BlendMode==vtkVolumeMapper::ADDITIVE_BLEND)
    {
    goodSoFar=0;
    vtkErrorMacro("Additive mode only works with 1-component scalars!");
    }

  /// return our status
  return goodSoFar;
}

///
/// \brief vtkSinglePassVolumeMapper::Render
/// \param ren
/// \param vol
///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  /// Invoke a VolumeMapperRenderStartEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderStartEvent,0);

  /// Start the timer to time the length of this render
  this->Implementation->Timer->StartTimer();

  /// Make sure everything about this render is OK.
  /// This is where the input is updated.
  if (this->ValidateRender(ren, vol ))
    {
    /// Everything is OK - so go ahead and really do the render
    this->GPURender(ren, vol);
    }

  /// Stop the timer
  this->Implementation->Timer->StopTimer();
  this->Implementation->ElapsedDrawTime =
    this->Implementation->Timer->GetElapsedTime();

  // Invoke a VolumeMapperRenderEndEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderEndEvent,0);
}

///
/// \brief vtkSinglePassVolumeMapper::GPURender
/// \param ren
/// \param vol
///----------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::GPURender(vtkRenderer* ren, vtkVolume* vol)
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

  if (vol->GetProperty()->GetMTime() >
      this->Implementation->ShaderBuildTime.GetMTime() ||
      this->GetMTime() > this->Implementation->ShaderBuildTime)
    {
    this->BuildShader(ren, vol);
    }

  vtkDataArray* scalars = this->GetScalars(input,
                          this->ScalarMode,
                          this->ArrayAccessMode,
                          this->ArrayId,
                          this->ArrayName,
                          this->Implementation->CellFlag);

  scalars->GetRange(this->Implementation->ScalarsRange);

  /// Load m_volume data if needed
  if (this->Implementation->IsDataDirty(input))
    {
    input->GetDimensions(this->Implementation->Dimensions);

    /// Update bounds, data, and geometry
    this->Implementation->ComputeBounds(input);
    this->Implementation->LoadVolume(input, scalars);
    this->Implementation->UpdateVolumeGeometry();
    }

  /// Update opacity transfer function
  /// TODO Passing level 0 for now
  this->Implementation->UpdateOpacityTransferFunction(vol,
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

  /// Update sampling distance
  this->Implementation->StepSize[0] = 1.0 / (this->Bounds[1] - this->Bounds[0]);
  this->Implementation->StepSize[1] = 1.0 / (this->Bounds[3] - this->Bounds[2]);
  this->Implementation->StepSize[2] = 1.0 / (this->Bounds[5] - this->Bounds[4]);

  this->Implementation->CellScale[0] = (this->Bounds[1] - this->Bounds[0]) * 0.5;
  this->Implementation->CellScale[1] = (this->Bounds[3] - this->Bounds[2]) * 0.5;
  this->Implementation->CellScale[2] = (this->Bounds[5] - this->Bounds[4]) * 0.5;

  /// Now use the shader
  this->Implementation->Shader.Use();

  /// Pass constant uniforms at initialization
  /// Step should be dependant on the bounds and not on the texture size
  /// since we can have non uniform voxel size / spacing / aspect ratio
  glUniform3f(this->Implementation->Shader("m_step_size"),
              this->Implementation->StepSize[0],
              this->Implementation->StepSize[1],
              this->Implementation->StepSize[2]);

  glUniform1f(this->Implementation->Shader("m_sample_distance"),
              this->SampleDistance);

  glUniform3f(this->Implementation->Shader("m_cell_scale"),
              this->Implementation->CellScale[0],
              this->Implementation->CellScale[1],
              this->Implementation->CellScale[2]);

  glUniform1f(this->Implementation->Shader("m_scale"),
              this->Implementation->Scale);

  glUniform1f(this->Implementation->Shader("m_bias"),
              this->Implementation->Bias);

  glUniform1i(this->Implementation->Shader("m_volume"), 0);
  glUniform1i(this->Implementation->Shader("m_color_transfer_func"), 1);
  glUniform1i(this->Implementation->Shader("m_opacity_transfer_func"), 2);
  glUniform1i(this->Implementation->Shader("m_noise_sampler"), 3);
  glUniform1i(this->Implementation->Shader("m_depth_sampler"), 4);

  glUniform3f(this->Implementation->Shader("m_ambient"),
              0.0, 0.0, 0.0);
  glUniform3f(this->Implementation->Shader("m_diffuse"),
              0.2, 0.2, 0.2);
  glUniform3f(this->Implementation->Shader("m_specular"),
              0.2, 0.2, 0.2);
  glUniform1f(this->Implementation->Shader("m_shininess"), 10.0);

  /// Bind textures
  /// Volume texture is at unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, this->Implementation->VolumeTextureId);

  /// Color texture is at unit 1
  this->Implementation->RGBTable->Bind();

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
  vtkMatrix4x4* projMat = ren->GetActiveCamera()->
    GetProjectionTransformMatrix(aspect[0]/aspect[1], -1, 1);
  float projectionMat[16];
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      projectionMat[i * 4 + j] = projMat->Element[i][j];
      }
    }

  /// Will require transpose of this matrix for OpenGL
  vtkMatrix4x4* mvMat = ren->GetActiveCamera()->GetViewTransformMatrix();
  float modelviewMat[16];
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      modelviewMat[i * 4 + j] = mvMat->Element[i][j];
      }
    }

  /// Will require transpose of this matrix for OpenGL
  /// Scene matrix
  float sceneMat[16];
  vtkMatrix4x4* scMat = vol->GetMatrix();
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      sceneMat[i * 4 + j] = scMat->Element[i][j];
      }
    }

  /// Compute texture to dataset matrix
  this->Implementation->TextureToDataSetMat->Identity();
  this->Implementation->TextureToDataSetMat->SetElement(0, 0,
    (1.0 / this->Implementation->StepSize[0]));
  this->Implementation->TextureToDataSetMat->SetElement(1, 1,
    (1.0 / this->Implementation->StepSize[1]));
  this->Implementation->TextureToDataSetMat->SetElement(2, 2,
    (1.0 / this->Implementation->StepSize[2]));
  this->Implementation->TextureToDataSetMat->SetElement(3, 3,
    1.0);
  this->Implementation->TextureToDataSetMat->SetElement(0, 3,
    this->Implementation->Bounds[0]);
  this->Implementation->TextureToDataSetMat->SetElement(1, 3,
    this->Implementation->Bounds[2]);
  this->Implementation->TextureToDataSetMat->SetElement(2, 3,
    this->Implementation->Bounds[4]);

  float textureDataSetMat[16];
  for (int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      textureDataSetMat[i * 4 + j] =
        this->Implementation->TextureToDataSetMat->Element[i][j];
      }
    }

  glUniformMatrix4fv(this->Implementation->Shader("m_projection_matrix"), 1,
                     GL_FALSE, &(projectionMat[0]));
  glUniformMatrix4fv(this->Implementation->Shader("m_modelview_matrix"), 1,
                     GL_FALSE, &(modelviewMat[0]));
  glUniformMatrix4fv(this->Implementation->Shader("m_scene_matrix"), 1,
                     GL_FALSE, &(sceneMat[0]));
  glUniformMatrix4fv(this->Implementation->Shader("m_texture_dataset_matrix"), 1,
                     GL_FALSE, &(textureDataSetMat[0]));

  /// We are using float for now
  double* cameraPos = ren->GetActiveCamera()->GetPosition();
  float pos[3] = {static_cast<float>(cameraPos[0]),
                  static_cast<float>(cameraPos[1]),
                  static_cast<float>(cameraPos[2])};

  glUniform3fv(this->Implementation->Shader("m_camera_pos"), 1, &(pos[0]));

  /// NOTE Assuming that light is located on the camera
  glUniform3fv(this->Implementation->Shader("m_light_pos"), 1, &(pos[0]));

  float volExtentsMin[3] = {this->Bounds[0], this->Bounds[2], this->Bounds[4]};
  float volExtentsMax[3] = {this->Bounds[1], this->Bounds[3], this->Bounds[5]};

  glUniform3fv(this->Implementation->Shader("m_vol_extents_min"), 1,
               &(volExtentsMin[0]));
  glUniform3fv(this->Implementation->Shader("m_vol_extents_max"), 1,
               &(volExtentsMax[0]));

  float textureExtentsMin[3] =
    {
    static_cast<float>(this->Implementation->Extents[0]),
    static_cast<float>(this->Implementation->Extents[2]),
    static_cast<float>(this->Implementation->Extents[4])
    };

  float textureExtentsMax[3] =
    {
    static_cast<float>(this->Implementation->Extents[1]),
    static_cast<float>(this->Implementation->Extents[3]),
    static_cast<float>(this->Implementation->Extents[5])
    };

  glUniform3fv(this->Implementation->Shader("m_texture_extents_min"), 1,
               &(textureExtentsMin[0]));
  glUniform3fv(this->Implementation->Shader("m_texture_extents_max"), 1,
               &(textureExtentsMax[0]));

  /// TODO Take consideration of reduction factor
  float fvalue[2];
  fvalue[0] = static_cast<float>(this->Implementation->WindowLowerLeft[0]);
  fvalue[1] = static_cast<float>(this->Implementation->WindowLowerLeft[1]);
  glUniform2fv(this->Implementation->Shader("m_window_lower_left_corner"), 1, &fvalue[0]);

  fvalue[0] = static_cast<float>(1.0 / this->Implementation->WindowSize[0]);
  fvalue[1] = static_cast<float>(1.0 / this->Implementation->WindowSize[1]);
  glUniform2fv(this->Implementation->Shader("m_inv_original_window_size"), 1, &fvalue[0]);

  fvalue[0] = static_cast<float>(1.0 / this->Implementation->WindowSize[0]);
  fvalue[1] = static_cast<float>(1.0 / this->Implementation->WindowSize[1]);
  glUniform2fv(this->Implementation->Shader("m_inv_window_size"), 1, &fvalue[0]);

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
