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
#include "vtkOpenGLVolumeRGBTable.h"

#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPerlinNoise.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVolumeProperty.h>

#include <GL/glew.h>
#include <vtkgl.h>

#include <cassert>

vtkStandardNewMacro(vtkSinglePassVolumeMapper);

/// TODO Remove this afterwards
#define GL_CHECK_ERRORS \
  {\
  assert(glGetError()== GL_NO_ERROR); \
  }

//--------------------------------------------------------------------------
///
/// \brief The vtkSinglePassVolumeMapper::vtkInternal class
///
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
    CubeVBOId(0),
    CubeVAOId(0),
    CubeIndicesId(0),
    VolumeTextureId(0),
    TransferFuncId(0),
    NoiseTextureId(0),
    CellFlag(-1),
    TextureWidth(1024),
    Parent(parent),
    RGBTable(0)
    {
    this->TextureSize[0] = this->TextureSize[1] = this->TextureSize[2] = -1;
    this->SampleDistance[0] = this->SampleDistance[1] =
      this->SampleDistance[2] = 0.0;
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
  void Initialize();

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
  /// \brief UpdateVolumeGeometry
  ///
  void UpdateVolumeGeometry();

  ///
  /// Private member variables

  bool Initialized;
  bool ValidTransferFunction;

  GLuint CubeVBOId;
  GLuint CubeVAOId;
  GLuint CubeIndicesId;

  GLuint VolumeTextureId;
  GLuint TransferFuncId;
  GLuint NoiseTextureId;

  vtkGLSLShader Shader;

  int CellFlag;
  int TextureSize[3];
  int TextureExtents[6];
  int TextureWidth;
  int BlendMode;

  double ScalarsRange[2];
  double Bounds[6];
  int Extents[6];
  double SampleDistance[3];
  double CellScale[3];
  double Scale;

  float* NoiseTextureData;
  GLint NoiseTextureSize;

  vtkSinglePassVolumeMapper* Parent;
  vtkOpenGLVolumeRGBTable* RGBTable;
  vtkOpenGLOpacityTables* OpacityTables;

  vtkTimeStamp VolumeBuildTime;
};

//--------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::Initialize()
{
  GLenum err = glewInit();
  if (GLEW_OK != err)	{
      cerr <<"Error: "<< glewGetErrorString(err)<<endl;
  } else {
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

  /// Load the raycasting shader
  this->Shader.LoadFromFile(GL_VERTEX_SHADER, "shaders/raycaster.vert");
  this->Shader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/raycaster.frag");

  /// Compile and link the shader
  this->Shader.CreateAndLinkProgram();
  this->Shader.Use();

  /// Add attributes and uniforms
  this->Shader.AddAttribute("in_vertex_pos");
  this->Shader.AddUniform("scene_matrix");
  this->Shader.AddUniform("modelview_matrix");
  this->Shader.AddUniform("projection_matrix");
  this->Shader.AddUniform("volume");
  this->Shader.AddUniform("camera_pos");
  this->Shader.AddUniform("light_pos");
  this->Shader.AddUniform("step_size");
  this->Shader.AddUniform("scale");
  this->Shader.AddUniform("cell_scale");
  this->Shader.AddUniform("color_transfer_func");
  this->Shader.AddUniform("opacity_transfer_func");
  this->Shader.AddUniform("noise");
  this->Shader.AddUniform("vol_extents_min");
  this->Shader.AddUniform("vol_extents_max");
  this->Shader.AddUniform("texture_extents_min");
  this->Shader.AddUniform("texture_extents_max");
  this->Shader.AddUniform("texture_coord_offset");
  this->Shader.AddUniform("enable_shading");
  this->Shader.AddUniform("ambient");
  this->Shader.AddUniform("diffuse");
  this->Shader.AddUniform("specular");
  this->Shader.AddUniform("shininess");

  // Setup unit cube vertex array and vertex buffer objects
  glGenVertexArrays(1, &this->CubeVAOId);
  glGenBuffers(1, &this->CubeVBOId);
  glGenBuffers(1, &this->CubeIndicesId);

  this->RGBTable = new vtkOpenGLVolumeRGBTable();

  /// TODO Currently we are supporting only one level
  this->OpacityTables = new vtkOpenGLOpacityTables(1);

  this->Shader.UnUse();

  this->Initialized = true;
}

//--------------------------------------------------------------------------
bool vtkSinglePassVolumeMapper::vtkInternal::LoadVolume(vtkImageData* imageData,
                                                        vtkDataArray* scalars)
{
  /// Generate OpenGL texture
  glEnable(GL_TEXTURE_3D);
  glGenTextures(1, &this->VolumeTextureId);
  glBindTexture(GL_TEXTURE_3D, this->VolumeTextureId);

  /// Set the texture parameters
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // TODO Make it configurable
  // Set the mipmap levels (base and max)
  //  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
  //  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

  GL_CHECK_ERRORS

  /// Allocate data with internal format and foramt as (GL_RED)
  GLint internalFormat = 0;
  GLenum format = 0;
  GLenum type = 0;

  double shift = 0.0;
  double scale = 1.0;
  int needTypeConversion = 0;

  int scalarType = scalars->GetDataType();
  if(scalars->GetNumberOfComponents()==4)
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
        shift=-(2*this->ScalarsRange[0]+1)/VTK_UNSIGNED_CHAR_MAX;
        scale = VTK_SIGNED_CHAR_MAX/(this->ScalarsRange[1]-this->ScalarsRange[0]);
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
        /// TODO Implement support for this
        std::cerr << "Scalar type VTK_UNSIGNED_LONG_LONG not supported" << std::endl;
        break;
      case VTK_SHORT:
        internalFormat = GL_INTENSITY16;
        format = GL_RED;
        type = GL_SHORT;
        shift=-(2*this->ScalarsRange[0]+1)/VTK_UNSIGNED_SHORT_MAX;
        scale = VTK_SHORT_MAX/(this->ScalarsRange[1]-this->ScalarsRange[0]);
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

  /// Update scale
  this->Scale = scale;
  imageData->GetExtent(this->Extents);

  void* dataPtr = scalars->GetVoidPointer(0);
  int i = 0;
  while(i < 3)
    {
    this->TextureSize[i] = this->Extents[2*i+1] - this->Extents[2*i] + 1;
    ++i;
    }

  glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
               this->TextureSize[0],this->TextureSize[1],this->TextureSize[2], 0,
               format, type, dataPtr);

  GL_CHECK_ERRORS


  /// TODO Enable mipmapping later
  // Generate mipmaps
  //glGenerateMipmap(GL_TEXTURE_3D);

  /// Update volume build time
  this->VolumeBuildTime.Modified();
  return 1;
}

//--------------------------------------------------------------------------
///
/// \brief vtkSinglePassVolumeMapper::vtkInternal::IsInitialized
/// \return
///
bool vtkSinglePassVolumeMapper::vtkInternal::IsInitialized()
{
  return this->Initialized;
}

//--------------------------------------------------------------------------
///
/// \brief vtkSinglePassVolumeMapper::vtkInternal::IsDataDirty
/// \param input
/// \return
///
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

//--------------------------------------------------------------------------
///
/// \brief vtkSinglePassVolumeMapper::vtkInternal::ComputeBounds
/// \param bounds
///
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
    this->Bounds[0] = origin[0]+
      static_cast<double>(this->Extents[0+swapBounds[0]]) * spacing[0];
    this->Bounds[2] = origin[1]+
      static_cast<double>(this->Extents[2+swapBounds[1]]) * spacing[1];
    this->Bounds[4] = origin[2]+
      static_cast<double>(this->Extents[4+swapBounds[2]]) * spacing[2];
    this->Bounds[1] = origin[0]+
      static_cast<double>(this->Extents[1-swapBounds[0]]) * spacing[0];
    this->Bounds[3] = origin[1]+
      static_cast<double>(this->Extents[3-swapBounds[1]]) * spacing[1];
    this->Bounds[5] = origin[2]+
      static_cast<double>(this->Extents[5-swapBounds[2]]) * spacing[2];
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
        this->Bounds[ 2 * i + swapBounds[i]] = origin[i];
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

//--------------------------------------------------------------------------
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

    /// Activate texture 1
    glActiveTexture(GL_TEXTURE1);

    this->RGBTable->Update(
      colorTransferFunction, this->ScalarsRange,
      volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION);

    glActiveTexture(GL_TEXTURE0);
    }
  else
    {
    std::cerr << "SinglePass volume mapper does not handle multi-component scalars";
    return 1;
    }

  return 0;
}

//--------------------------------------------------------------------------
int vtkSinglePassVolumeMapper::vtkInternal::UpdateOpacityTransferFunction(
  vtkVolume* vol, int numberOfScalarComponents, unsigned int level)
{
  if (!vol)
    {
    std::cerr << "Invalid volume" << std::endl;
    return 1;
    }

  if (numberOfScalarComponents != 1)
    {
    std::cerr << "SinglePass volume mapper does not handle multi-component scalars";
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

  /// Activate texture 2
  glActiveTexture(GL_TEXTURE2);

  this->OpacityTables->GetTable(level)->Update(
    scalarOpacity,this->BlendMode,
    this->SampleDistance,
    this->ScalarsRange,
    volumeProperty->GetScalarOpacityUnitDistance(),
    volumeProperty->GetInterpolationType() == VTK_LINEAR_INTERPOLATION);

  /// Restore default active texture
  glActiveTexture(GL_TEXTURE0);

  return 0;
}

//--------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::UpdateNoiseTexture()
{
  if (this->NoiseTextureData == 0)
    {
    glGenTextures(1, &this->NoiseTextureId);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, this->NoiseTextureId);

    GLsizei size = 128;
    GLint maxSize;
    const float factor = 0.1f;
    const float amplitude = 0.5f * factor;

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
      noiseGenerator->SetAmplitude(amplitude);
      int j = 0;
      while(j < size)
        {
        int i = 0;
        while(i < size)
          {
          this->NoiseTextureData[j * size + i] =
            amplitude + static_cast<float>(noiseGenerator->EvaluateFunction(i, j, 0.0));
          ++i;
          }
        ++j;
        }
      }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, size, size, 0,
                 GL_RED, GL_FLOAT, this->NoiseTextureData);

    GLfloat borderColor[4]={0.0, 0.0, 0.0, 0.0};
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glActiveTexture(GL_TEXTURE0);
    }
}

//--------------------------------------------------------------------------
void vtkSinglePassVolumeMapper::vtkInternal::UpdateVolumeGeometry()
{
  /// Cube vertices
  double vertices[8][3] =
    {
    {this->Bounds[0], this->Bounds[2], this->Bounds[4]}, // 0
    {this->Bounds[1], this->Bounds[2], this->Bounds[4]}, // 1
    {this->Bounds[1], this->Bounds[3], this->Bounds[4]}, // 2
    {this->Bounds[0], this->Bounds[3], this->Bounds[4]}, // 3
    {this->Bounds[0], this->Bounds[2], this->Bounds[5]}, // 4
    {this->Bounds[1], this->Bounds[2], this->Bounds[5]}, // 5
    {this->Bounds[1], this->Bounds[3], this->Bounds[5]}, // 6
    {this->Bounds[0], this->Bounds[3], this->Bounds[5]}  // 7
    };

  /// Cube indices
  GLushort cubeIndices[36]=
    {
    0,5,4, // bottom
    5,0,1, // bottom
    3,7,6, // top
    3,6,2, // op
    7,4,6, // front
    6,4,5, // front
    2,1,3, // left side
    3,1,0, // left side
    3,0,7, // right side
    7,0,4, // right side
    6,5,2, // back
    2,5,1  // back
    };

  glBindVertexArray(this->CubeVAOId);

  /// Pass cube vertices to buffer object memory
  glBindBuffer (GL_ARRAY_BUFFER, this->CubeVBOId);
  glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), &(vertices[0][0]), GL_STATIC_DRAW);

  GL_CHECK_ERRORS

  /// Enable vertex attributre array for position
  /// and pass indices to element array  buffer
  glEnableVertexAttribArray(this->Shader["in_vertex_pos"]);
  glVertexAttribPointer(this->Shader["in_vertex_pos"], 3, GL_DOUBLE, GL_FALSE, 0, 0);

  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), &cubeIndices[0], GL_STATIC_DRAW);

  GL_CHECK_ERRORS

  glBindVertexArray(0);
}

//--------------------------------------------------------------------------
///
/// \brief vtkSinglePassVolumeMapper::vtkSinglePassVolumeMapper
///
vtkSinglePassVolumeMapper::vtkSinglePassVolumeMapper() : vtkVolumeMapper()
{
  this->Implementation = new vtkInternal(this);
}

//--------------------------------------------------------------------------
///
/// \brief vtkSinglePassVolumeMapper::~vtkSinglePassVolumeMapper
///
vtkSinglePassVolumeMapper::~vtkSinglePassVolumeMapper()
{
  delete this->Implementation;
  this->Implementation = 0;
}

//--------------------------------------------------------------------------
///
/// \brief vtkSinglePassVolumeMapper::PrintSelf
/// \param os
/// \param indent
///
void vtkSinglePassVolumeMapper::PrintSelf(ostream& vtkNotUsed(os),
                                          vtkIndent indent)
{
  // TODO Implement this method
}

///---------------------------------------------------------------------------
///
/// \brief vtkSinglePassVolumeMapper::Render
/// \param ren
/// \param vol
///
void vtkSinglePassVolumeMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  /// Make sure the context is current
  ren->GetRenderWindow()->MakeCurrent();

  /// Update volume first to make sure states are current
  vol->Update();

  vtkImageData* input = this->GetInput();

  /// Enable texture 1D and 3D as we are using it
  /// for transfer functions and volume data
  glEnable(GL_TEXTURE_1D);
  glEnable(GL_TEXTURE_3D);

  if (!this->Implementation->IsInitialized())
    {
    this->Implementation->Initialize();
    }

  vtkDataArray* scalars = this->GetScalars(input,
                          this->ScalarMode,
                          this->ArrayAccessMode,
                          this->ArrayId,
                          this->ArrayName,
                          this->Implementation->CellFlag);

  scalars->GetRange(this->Implementation->ScalarsRange);

  /// Load volume data if needed
  if (this->Implementation->IsDataDirty(input))
    {
    /// Update bounds
    this->Implementation->ComputeBounds(input);
    this->Implementation->LoadVolume(input, scalars);
    this->Implementation->UpdateVolumeGeometry();
    }

  /// Use the shader
  this->Implementation->Shader.Use();

  /// Update opacity transfer function
  /// TODO Passing level 0 for now
  this->Implementation->UpdateOpacityTransferFunction(vol,
    scalars->GetNumberOfComponents(), 0);

  /// Update transfer color functions
  this->Implementation->UpdateColorTransferFunction(vol,
    scalars->GetNumberOfComponents());

  /// Update noise texture
  this->Implementation->UpdateNoiseTexture();

  GL_CHECK_ERRORS

  /// Enable depth test
  glEnable(GL_DEPTH_TEST);

  /// Set the over blending function
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /// Enable blending
  glEnable(GL_BLEND);

  GL_CHECK_ERRORS

  /// Update sampling distance
  this->Implementation->SampleDistance[0] = 1.0 / (this->Bounds[1] - this->Bounds[0]);
  this->Implementation->SampleDistance[1] = 1.0 / (this->Bounds[3] - this->Bounds[2]);
  this->Implementation->SampleDistance[2] = 1.0 / (this->Bounds[5] - this->Bounds[4]);

  this->Implementation->CellScale[0] = (this->Bounds[1] - this->Bounds[0]) * 0.5;
  this->Implementation->CellScale[1] = (this->Bounds[3] - this->Bounds[2]) * 0.5;
  this->Implementation->CellScale[2] = (this->Bounds[5] - this->Bounds[4]) * 0.5;

  /// Pass constant uniforms at initialization
  /// Step should be dependant on the bounds and not on the texture size
  /// since we can have non uniform voxel size / spacing / aspect ratio
  glUniform3f(this->Implementation->Shader("step_size"),
              this->Implementation->SampleDistance[0],
              this->Implementation->SampleDistance[1],
              this->Implementation->SampleDistance[2]);

  glUniform3f(this->Implementation->Shader("cell_scale"),
              this->Implementation->CellScale[0],
              this->Implementation->CellScale[1],
              this->Implementation->CellScale[2]);

  glUniform1f(this->Implementation->Shader("scale"), this->Implementation->Scale);

  glUniform1i(this->Implementation->Shader("volume"), 0);
  glUniform1i(this->Implementation->Shader("color_transfer_func"), 1);
  glUniform1i(this->Implementation->Shader("opacity_transfer_func"), 2);
  glUniform1i(this->Implementation->Shader("noise"), 3);

  /// Shading is ON by default
  /// TODO Add an API to enable / disable shading if not present
  glUniform1i(this->Implementation->Shader("enable_shading"),
              vol->GetProperty()->GetShade(0));
  glUniform3f(this->Implementation->Shader("ambient"),
              0.0, 0.0, 0.0);
  glUniform3f(this->Implementation->Shader("diffuse"),
              0.2, 0.2, 0.2);
  glUniform3f(this->Implementation->Shader("specular"),
              0.2, 0.2, 0.2);
  glUniform1f(this->Implementation->Shader("shininess"), 10.0);

  /// Bind textures
  /// Volume texture is at unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, this->Implementation->VolumeTextureId);

  /// Color texture is at unit 1
  glActiveTexture(GL_TEXTURE1);
  this->Implementation->RGBTable->Bind();

  /// Opacity texture is at unit 2
  /// TODO Supports only one table for now
  glActiveTexture(GL_TEXTURE2);
  this->Implementation->OpacityTables->GetTable(0)->Bind();

  /// Noise texture is at unit 3
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, this->Implementation->NoiseTextureId);

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

  glUniformMatrix4fv(this->Implementation->Shader("projection_matrix"), 1,
                     GL_FALSE, &(projectionMat[0]));
  glUniformMatrix4fv(this->Implementation->Shader("modelview_matrix"), 1,
                     GL_FALSE, &(modelviewMat[0]));
  glUniformMatrix4fv(this->Implementation->Shader("scene_matrix"), 1,
                     GL_FALSE, &(sceneMat[0]));

  /// We are using float for now
  double* cameraPos = ren->GetActiveCamera()->GetPosition();
  float pos[3] = {static_cast<float>(cameraPos[0]),
                  static_cast<float>(cameraPos[1]),
                  static_cast<float>(cameraPos[2])};

  glUniform3fv(this->Implementation->Shader("camera_pos"), 1, &(pos[0]));

  /// NOTE Assuming that light is located on the camera
  glUniform3fv(this->Implementation->Shader("light_pos"), 1, &(pos[0]));

  float volExtentsMin[3] = {this->Bounds[0], this->Bounds[2], this->Bounds[4]};
  float volExtentsMax[3] = {this->Bounds[1], this->Bounds[3], this->Bounds[5]};

  glUniform3fv(this->Implementation->Shader("vol_extents_min"), 1,
               &(volExtentsMin[0]));
  glUniform3fv(this->Implementation->Shader("vol_extents_max"), 1,
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

  glUniform3fv(this->Implementation->Shader("texture_extents_min"), 1,
               &(textureExtentsMin[0]));
  glUniform3fv(this->Implementation->Shader("texture_extents_max"), 1,
               &(textureExtentsMax[0]));

  glBindVertexArray(this->Implementation->CubeVAOId);
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

  /// Undo binds and state changes
  /// TODO Provide a stack implementation
  this->Implementation->Shader.UnUse();

  glBindVertexArray(0);
  glDisable(GL_BLEND);

  glActiveTexture(GL_TEXTURE0);

  glDisable(GL_TEXTURE_3D);
  glDisable(GL_TEXTURE_1D);
}
