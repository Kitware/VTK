/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLState.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLState - Raw OpenGL State.
// .SECTION Description
// A vtkOpenGLState object can record the OpenGL state from OpenGL query calls.
// The only purpose of this class is debugging. It is useful when there is no
// available OpenGL debugging tool of if the existing OpenGL debugging tools
// cannot work in special configurations.
//
// The typical usage, is to dump the state in a file at different points of
// an algorithm. To use it, create an instance of vtkOpenGLState by passing it
// a valid OpenGL context (a vtkOpenGLRenderWindow), call Update(), call
// PrintSelf() on a ofstream. Debugging consists then to perform a diff between
// output text files.

// .SECTION Implementation
// Not all the OpenGL state is covered yet.

#ifndef __vtkOpenGLState_h
#define __vtkOpenGLState_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkgl.h"
#include <vector>

class vtkOpenGLRenderWindow;

class vtkOpenGLMaterialState
{
public:
  GLfloat Ambient[4];
  GLfloat Diffuse[4];
  GLfloat Specular[4];
  GLfloat Shininess;

  GLfloat AmbientIndex;
  GLfloat DiffuseIndex;
  GLfloat SpecularIndex;
};

class vtkOpenGLLightState
{
public:
  void Update();

  GLfloat Ambient[4];
  GLfloat Diffuse[4];
  GLfloat Specular[4];
  GLfloat Position[4];
  GLfloat ConstantAttenuation;
  GLfloat LinearAttenuation;
  GLfloat QuadraticAttenuation;
  GLfloat SpotDirection[4];
  GLfloat SpotExponent;
  GLfloat SpotCutoff;
};

class vtkOpenGLTextureImageState
{
public:
  void Update();

  GLvoid *Image;
  GLint Width;
  GLint Height;
  GLint Border;
  GLint InternalFormat;
  GLint RedSize;
  GLint GreenSize;
  GLint BlueSize;
  GLint AlphaSize;
  GLint LuminanceSize;
  GLint IntensitySize;

  // 1.2.1
  GLint Depth;

  // 1.3
  GLboolean Compressed;
  GLint CompressedImageSize;

  // 1.4
  GLint DepthSize;

};

#if 0
class vtkOpenGLTextureObjectState
{
public:
  void Update();

  static const int n=10;
  vtkOpenGLTextureImageState Texture1D[n];
  vtkOpenGLTextureImageState Texture2D[n];

  GLfloat BorderColor[4];
  GLint MinFilter;
  GLint MagFilter;
  GLint WrapS;
  GLint WrapT;
  GLfloat TexturePriority;
  GLboolean TextureResident;

  // 1.2.1
  vtkOpenGLTextureImageState Texture3D[n];

  GLint WrapR;
  GLint MinLOD;
  GLint MaxLOD;
  GLint BaseLevel;
  GLint MaxLevel;

  // 1.3
  vtkOpenGLTextureImageState TextureCubeMapPositiveX[n];
  vtkOpenGLTextureImageState TextureCubeMapNegativeX[n];
  vtkOpenGLTextureImageState TextureCubeMapPositiveY[n];
  vtkOpenGLTextureImageState TextureCubeMapNegativeY[n];
  vtkOpenGLTextureImageState TextureCubeMapPositiveZ[n];
  vtkOpenGLTextureImageState TextureCubeMapNegativeZ[n];

  // 1.4
  GLfloat LODBias;
  GLint DepthTextureMode;
  GLint TextureCompareMode;
  GLint TextureCompareFunc;
  GLboolean GenerateMipmap;
};
#endif

class vtkOpenGLTexGenState
{
public:
  void Update();

  GLboolean Enabled;
  GLfloat EyePlane[4];
  GLfloat ObjectPlane[4];
  GLint Mode;
};

class vtkOpenGLPixelControl
{
public:
  void Update();
  void PrintSelf(ostream &os,
                 vtkIndent indent);
  GLboolean SwapBytes;
  GLboolean LsbFirst; // Warning: cannot be LSBFirst, as it is defined in X.h
  GLint RowLength;
  GLint SkipRows;
  GLint SkipPixels;
  GLint Alignment;

  // 1.2.1
  GLint ImageHeight;
  GLint SkipImages;
};

class vtkOpenGLComponentTransform
{
public:
  void Update();
  void PrintSelf(ostream &os,
                 vtkIndent indent);
  GLfloat Scale;
  GLfloat Bias;
};

class vtkOpenGLRGBAPixelMapState
{
public:
  void Update();

  GLint Size;
  GLfloat *Map; // values
};

class vtkOpenGLIndexPixelMapState
{
public:
  void Update();

  GLint Size;
  GLint *Map; // values
};

class vtkOpenGLTextureCoordinateProcessingUnit
{
public:
  GLfloat CurrentMatrix[16];
  GLint MatrixStackDepth;
  vtkOpenGLTexGenState TextureGenS;
  vtkOpenGLTexGenState TextureGenT;
  vtkOpenGLTexGenState TextureGenR;
  vtkOpenGLTexGenState TextureGenQ;
  // Texture environment generation, 2.0
  GLboolean CoordReplace;

  GLfloat CurrentTextureCoords[4];
  GLfloat CurrentRasterTextureCoords[4];
};

class vtkOpenGLTextureImageUnit
{
public:
  // Texture object bound to 1D target/sampler
  GLint TextureBinding1D;
  // Texture object bound to 2D target/sampler
  GLint TextureBinding2D;
  // 1.2.1
  // Texture object bound to 3D target/sampler
  GLint TextureBinding3D;
  // 1.3
  // Texture object bound to cubemap target/sampler
  GLint TextureBindingCubeMap;
};

class vtkOpenGLTextureImageUnitFixedPipelineState
{
public:
  GLboolean Texture1DEnabled;
  GLboolean Texture2DEnabled;
  // 1.2.1
  GLboolean Texture3DEnabled;
  // 1.3
  GLboolean TextureCubeMapEnabled;
};

class vtkOpenGLClipPlaneState
{
public:
  GLfloat Equation[4];
  GLboolean Enabled;
};


// Replaced by shader program
class vtkOpenGLFixePipelineState
{
public:
  // Replaced by verter shader

  // Transformation state
  GLboolean Normalize;

  // Transformation state: 1.2.1
  GLboolean RescaleNormal;

  // Lighting
  GLboolean LightingEnabled;

  std::vector<GLboolean> *LightEnabled; // MaxLights (ex:8)


  // Replaced by fragment shader:

  // 1.3
  GLint CombineRGB;
  GLint CombineAlpha;
  GLint Source0RGB;
  GLint Source1RGB;
  GLint Source2RGB;
  GLint Source0Alpha;
  GLint Source1Alpha;
  GLint Source2Alpha;
  GLint Operand0RGB;
  GLint Operand1RGB;
  GLint Operand2RGB;
  GLint Operand0Alpha;
  GLint Operand1Alpha;
  GLint Operand2Alpha;
  GLint RGBScale;
  GLint AlphaScale;

  std::vector<vtkOpenGLTextureImageUnitFixedPipelineState> *TextureImageUnitEnabled; // MaxTextureUnits (ex: 4)

    // Coloring
  GLboolean FogEnabled;
  // Coloring, 1.4
  GLboolean ColorSumEnabled;
};

class vtkOpenGLBufferObjectState
{
public:
  void PrintSelf(ostream &os,
                 vtkIndent indent);

  GLuint Id;
  GLint Size;
  GLenum Usage;
  GLenum Access;
  GLenum AccessFlags; // in GL 3.1 spec but missing in header files
  GLboolean Mapped;
  GLvoid *MapPointer;
  GLint MapOffset; // in GL 3.1 spec but missing in header files
  GLint MapLength; // in GL 3.1 spec but missing in header files
protected:
  void BufferAccessFlagsToStream(ostream &os);
  const char *BufferUsageToString();
  const char *BufferAccessToString();
};

class vtkOpenGLShaderState
{
public:
  void PrintSelf(ostream &os,
                 vtkIndent indent);
  GLuint Id;
  GLenum Type;
  GLboolean DeleteStatus;
  GLboolean CompileStatus;
  vtkgl::GLchar *InfoLog;
  GLint InfoLogLength;
  vtkgl::GLchar *Source;
  GLint SourceLength;
protected:
  const char *ShaderTypeToString();
};

class vtkOpenGLProgramState
{
public:
  void PrintSelf(ostream &os,
                 vtkIndent indent);
  GLuint Id;
  GLboolean DeleteStatus;
  GLboolean LinkStatus;
  GLboolean ValidateStatus;
  GLint NumberOfAttachedShaders;
  GLint InfoLogLength;
  GLint ActiveUniforms;
  GLint ActiveUniformMaxLength;
  GLint ActiveAttributes;
  GLint ActiveAttributeMaxLength;
  std::vector<vtkOpenGLShaderState> *AttachedShaders;
  vtkgl::GLchar *InfoLog;


  bool HasVertexShader;
  bool HasFragmentShader;
};

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLState
{
public:
  vtkOpenGLState(vtkOpenGLRenderWindow *context);
  ~vtkOpenGLState();

  // Save current OpenGL state in the object.
  void Update();

  void PrintSelf(ostream &os,
                 vtkIndent indent);

  // There are too many variables to make them protected and implement
  // a public Get method for each of them. We keep them public.
  // I wish C++ would syntaxically forbid construction like c->value=x,
  // like Eiffel does...


  vtkOpenGLFixePipelineState FixedPipeline;

  // OpenGL 1.1 state

  // Current values and associated data

  GLfloat CurrentColor[4];
  GLfloat CurrentIndex;
  //  CurrentTextureCoords: see vtkOpenGLTextureCoordinateProcessingUnit
  GLfloat CurrentNormal[3];

  GLfloat CurrentRasterPosition[4];
  GLfloat CurrentRasterDistance;
  GLfloat CurrentRasterColor[4];
  GLfloat CurrentRasterIndex;
  //  CurrentRasterTextureCoords:  see vtkOpenGLTextureCoordinateProcessingUnit
  GLboolean CurrentRasterPositionValid;
  GLboolean EdgeFlag;

  // Current values and associated data, 1.4
  GLfloat CurrentSecondaryColor[4];
  GLfloat CurrentFogCoordinate[3]; // or scalar?

  // Current values and associated data, 2.1
  GLfloat CurrentRasterSecondaryColor[4];


  // Vertex Array Data

  GLboolean VertexArrayEnabled;
  GLint VertexArraySize;
  GLint VertexArrayType;
  GLint VertexArrayStride;
  GLvoid *VertexArrayPointer;

  GLboolean NormalArrayEnabled;
  GLint NormalArrayType;
  GLint NormalArrayStride;
  GLvoid *NormalArrayPointer;

  GLboolean ColorArrayEnabled;
  GLint ColorArraySize;
  GLint ColorArrayType;
  GLint ColorArrayStride;
  GLvoid *ColorArrayPointer;

  GLboolean IndexArrayEnabled;
  GLint IndexArrayType;
  GLint IndexArrayStride;
  GLvoid *IndexArrayPointer;

  GLboolean TextureCoordArrayEnabled;
  GLint TextureCoordArraySize;
  GLint TextureCoordArrayType;
  GLint TextureCoordArrayStride;
  GLvoid *TextureCoordArrayPointer;

  GLboolean EdgeFlagArray;
  GLint EdgeFlagArrayStride;
  GLvoid *EdgeFlagArrayPointer;

  // Vertex Array Data, 1.3
  GLenum ClientActiveTexture;

  // Vertex Array Data, 1.4
  GLboolean FogCoordinateArrayEnabled;
  GLint FogCoordinateArrayType;
  GLint FogCoordinateArrayStride;
  GLvoid *FogCoordinateArrayPointer;

  GLboolean SecondaryColorArrayEnabled;
  GLint SecondaryColorArraySize;
  GLint SecondaryColorArrayType;
  GLint SecondaryColorArrayStride;
  GLvoid *SecondaryColorArrayPointer;

  // Vertex Array Data, 1.5, VBO
  GLint ArrayBufferBinding;
  GLint VertexArrayBufferBinding;
  GLint NormalArrayBufferBinding;
  GLint ColorArrayBufferBinding;
  GLint IndexArrayBufferBinding;
  GLint TextureCoordArrayBufferBinding;
  GLint EdgeFlagArrayBufferBinding;
  GLint SecondaryColorArrayBufferBinding;
  GLint FogCoordArrayBufferBinding;
  GLint ElementArrayBufferBinding;

  // Vertex Array Data, 2.0
  GLboolean VertexAttribArrayEnabled;
  GLint VertexAttribArraySize;
  GLint VertexAttribArrayStride;
  GLint VertexAttribArrayType;
  GLboolean VertexAttribArrayNormalized;
  GLvoid *VertexAttribArrayPointer;

  // Vertex Array Data, 2.1
  GLint VertexAttribArrayBufferBinding;

  // Buffer Object State, 1.5
  GLint BufferSize;
  GLint BufferUsage;
  GLint BufferAccess;
  GLboolean BufferMapped;
  GLvoid *BufferMapPointer;

  // Transformation state

  GLfloat ModelViewMatrix[16];
  GLint ModelViewStackDepth;

  GLfloat ProjectionMatrix[16];
  GLint ProjectionStackDepth;

  // 2.0
  GLint MaxTextureCoords; // 8
  std::vector<vtkOpenGLTextureCoordinateProcessingUnit> *TCPU;

  // 2.0
  GLint MaxCombinedTextureImageUnits; // 16
  std::vector<vtkOpenGLTextureImageUnit> *TIU;

  GLint Viewport[4];
  GLfloat DepthRange[2];

  GLint MatrixMode;
  GLboolean Normalize;

  std::vector<vtkOpenGLClipPlaneState> *ClipPlanes; // MaxClipPlanes (ex: 6)

  // Transformation state: 1.2.1, optional
  GLfloat ColorMatrix[16];
  GLint ColorMatrixStackDepth;

  // Coloring

  GLfloat FogColor[4];
  GLfloat FogIndex;
  GLfloat FogDensity;
  GLfloat FogStart;
  GLfloat FogEnd;
  GLint FogMode;

  GLint ShadeModel; // fixed-pipeline and GLSL

  // Coloring, 1.4
  GLint FogCoordinateSource; // renamed FogCoordSrc in 1.5

  // Lighting
  GLboolean ColorMaterialEnabled;
  GLint ColorMaterialParameter;
  GLint ColorMaterialFace;
  vtkOpenGLMaterialState FrontMaterial;
  vtkOpenGLMaterialState BackMaterial;

  GLfloat LightModelAmbient[4];
  GLboolean LightModelLocalViewer;
  GLboolean LightModelTwoSide;

  std::vector<vtkOpenGLLightState> *Lights; // MaxLights (ex: 8)

  // Lighting: 1.2.1
  GLint lightModelColorControl;

  // Rasterization
  GLfloat PointSize;
  GLboolean PointSmoothEnabled;

  GLfloat LineWidth;
  GLboolean LineSmoothEnabled;
  GLint LineStipplePattern;
  GLint LineStippleRepeat;
  GLboolean LineStippleEnabled;

  GLboolean CullFaceEnabled;
  GLint CullFaceMode;
  GLint FrontFace;
  GLboolean PolygonSmoothEnabled;
  GLint PolygonMode[2]; //0=front, 1=back
  GLfloat PolygonOffsetFactor;
  GLfloat PolygonOffsetUnits;
  GLboolean PolygonOffsetPointEnabled;
  GLboolean PolygonOffsetLineEnabled;
  GLboolean PolygonOffsetFillEnabled;
  GLint PolygonStipple;
  GLboolean PolygonStippleEnabled;

  // Rasterization, 1.4
  GLfloat PointSizeMin;
  GLfloat PointSizeMax;
  GLfloat PointFadeThresholdSize;
  GLfloat PointDistanceAttenuation;

  // Rasterization, 2.0
  GLboolean PointSpriteEnabled;
  GLint PointSpriteCoordOrigin;

  // Multisampling (1.2.1)
  GLboolean MultiSampleEnabled;
  GLboolean SampleAlphaToCoverageEnabled;
  GLboolean SampleAlphaToOneEnabled;
  GLboolean SampleCoverageEnabled;
  GLfloat SampleCoverageValue;
  GLboolean SampleCoverageInvert;



//  vtkOpenGLTextureObjectState TextureObjects;

  // Texture environment generation
  GLint TextureEnvMode;
  GLfloat TextureEnvColor;

  // Texture environment generation, 1.3
  GLenum ActiveTexture;

  // Texture environment generation, 1.4
  GLfloat TextureLODBias;

  // Pixel operations
  // - Scissor
  GLboolean ScissorTestEnabled;
  GLint ScissorBox[4];

  GLboolean AlphaTestEnabled;
  GLint AlphaTestFunc;
  GLfloat AlphaTestRef;

  GLboolean StencilTestEnabled;
  GLint StencilFunc;
  GLint StencilValueMask;
  GLint StencilRef;
  GLint StencilFail;
  GLint StencilPassDepthFail;
  GLint StencilPassDepthPass;

  // - Stencil, 2.0
  GLint StencilBackFunc;
  GLint StencilBackValueMask;
  GLint StencilBackRef;
  GLint StencilBackFail;
  GLint StencilBackPassDepthFail;
  GLint StencilBackPassDepthPass;

  GLboolean DepthTestEnabled;
  GLint DepthFunc;

  GLboolean BlendEnabled;
  GLint BlendSrc; // <=1.3
  GLint BlendDst; // <=1.3

  // Optional in 1.2.1, mandatory in 1.4
  GLint BlendEquation; // renamed BlendEquationRGB in 2.0
  GLfloat BlendColor[4];

  // 1.4
  GLint BlendSrcRGB;
  GLint BlendSrcAlpha;
  GLint BlendDstRGB;
  GLint BlendDstAlpha;

  // 2.0
  GLint BlendEquationRGB;
  GLint BlendEquationAlpha;

  GLboolean DitherEnabled;

  GLboolean IndexLogicOpEnabled;
  GLboolean ColorLogicOpEnabled;
  GLint LogicOpMode;

  // Framebuffer control (drawing)

  // 2.0
  GLint MaxDrawBuffers;
  std::vector<GLint> *DrawBuffers;

  GLint IndexWriteMask;
  GLboolean ColorWriteMask[4];
  GLboolean DepthWriteMask;
  GLuint StencilWriteMask;
  GLfloat ColorClearValue[4];
  GLfloat IndexClearValue;
  GLfloat DepthClearValue;
  GLint StencilClearValue;
  GLfloat AccumClearValue[4];

  // Framebuffer control (drawing) 2.0
  GLuint StencilBackWriteMask;

  // Framebuffer control (drawing) 2.1
  GLint DrawBuffer0;
  GLint DrawBuffer1;
  GLint DrawBuffer3; // <max

  // Pixels

  vtkOpenGLPixelControl Unpack;
  vtkOpenGLPixelControl Pack;

  GLboolean MapColor; // see PixelMap
  GLboolean MapStencil; // see PixelMap
  GLint IndexShift;
  GLint IndexOffset;

  vtkOpenGLComponentTransform RedTransform;
  vtkOpenGLComponentTransform GreenTransform;
  vtkOpenGLComponentTransform BlueTransform;
  vtkOpenGLComponentTransform AlphaTransform;
  vtkOpenGLComponentTransform DepthTransform;

  GLfloat ZoomX;
  GLfloat ZoomY;

  // Size==1 <=> not used.
  vtkOpenGLIndexPixelMapState PixelMapColorIndexToColorIndex;
  vtkOpenGLIndexPixelMapState PixelMapStencilIndexToStencilIndex;
  vtkOpenGLRGBAPixelMapState PixelMapColorIndexToRed;
  vtkOpenGLRGBAPixelMapState PixelMapColorIndexToGreen;
  vtkOpenGLRGBAPixelMapState PixelMapColorIndexToBlue;
  vtkOpenGLRGBAPixelMapState PixelMapColorIndexToAlpha;
  vtkOpenGLRGBAPixelMapState PixelMapRedToRed;
  vtkOpenGLRGBAPixelMapState PixelMapGreenToGreen;
  vtkOpenGLRGBAPixelMapState PixelMapBlueToBlue;
  vtkOpenGLRGBAPixelMapState PixelMapAlphaToAlpha;

  // Pixels, 2.1
  GLenum PixelPackBufferBinding;
  GLenum PixelUnpackBufferBinding;

  // Relevant only if PixelPackBufferBinding>0
  vtkOpenGLBufferObjectState PixelPackBufferObject;
  // Relevant only if PixelUnPackBufferBinding>0
  vtkOpenGLBufferObjectState PixelUnpackBufferObject;

  // 1.2.1, optional
  GLboolean ColorTableEnabled;
  GLboolean PostConvolutionColorTableEnabled;
  GLboolean PostColorMatricColorTableEnabled;
  // TODO ...

  // Framebuffer control (reading)

  GLint ReadBuffer;

  // Evaluators
  // TODO

  // Shader Object State 2.0
  vtkOpenGLProgramState *CurrentProgramState;

#if 0
  GLint ShaderType;
  GLboolean DeleteStatus;
  GLboolean CompileStatus;
  vtkgl::GLchar *ShaderLogInfo;
  GLint InfoLogLength;
  vtkgl::GLchar *ShaderSource;
  GLint ShaderSourceLength;
#endif
  // Program Object State 2.0
  GLuint CurrentProgram;

#if 0
  GLboolean DeleteStatus;
  GLboolean LinkStatus;
  GLboolean ValidateStatus;
  GLint *AttachedShaders;
  GLint InfoLogLength;
  GLint ActiveUniforms;
  GLint ActiveUniformMaxLength;
  GLint ActiveAttributes;
  GLint ActiveAttributesMaxLength;
#endif

  // Vertex Shader State 2.0
  GLboolean VertexProgramTwoSideEnabled;
  GLfloat CurrentVertexAttrib[16][4];
  GLboolean VertexProgramPointSizeEnabled;

  // Hints
  GLint PerpectiveCorrectionHint;
  GLint PointSmoothHint;
  GLint LineSmoothHint;
  GLint PolygonSmoothHint;
  GLint FogHint;
  // Hints: 1.3
  GLint TextureCompressionHint;
  // Hints: 1.4
  GLint GenerateMipMapHint;
  // Hints: 2.0
  GLint FragmentShaderDerivativeHint;

  // Implementation dependent values

  // per OpenGL implementation

  GLint MaxLights;
  GLint MaxClipPlanes;
  GLint MaxModelViewStackDepth;
  GLint MaxProjectionStackDepth;
  GLint MaxTextureStackDepth;
  GLint SubpixelBits;
  GLint MaxTextureSize;
  GLint MaxPixelMapTable;
  GLint MaxNameStackDepth;
  GLint MaxListNesting;
  GLint MaxEvalOrder;
  GLint MaxViewportDims;
  GLint MaxAttribStackDepth;
  GLint MaxClientAttribStackDepth;

  // per framebuffer
  GLint AuxBuffers;
  GLboolean RGBAMode;
  GLboolean IndexMode;
  GLboolean DoubleBuffer;
  GLboolean Stereo;

  // per framebuffer object
  GLint MaxColorAttachments;

  // per OpenGL implementation

  GLfloat PointSizeRange[2]; // 1.2.1: renamed SmoothPointSizeRange
  GLfloat PointSizeGranularity; // 1.2.1: renamed SmoothPointSizeGranularity
  GLfloat LineWidthRange[2]; // 1.2.1: renamed SmoothLineWidthRange
  GLfloat LineWidthGranularity; // 1.2.1: renamed SmoothLineWidthGranularity

  // per framebuffer (for each color buffer)
  GLint RedBits;
  GLint GreenBits;
  GLint BlueBits;
  GLint AlphaBits;
  GLint IndexBits;

  GLint DepthBits; // depth buffer
  GLint StencilBits; // stencil buffer

  GLint AccumRedBits;
  GLint AccumGreenBits;
  GLint AccumBlueBits;
  GLint AccumAlphaBits;

  // per OpenGL implementation, 1.2.1
  GLint Max3DTextureSize;
  GLfloat AliasedPointSizeRange[2];
  GLfloat AliasedLineWidthRange[2];
  GLint MaxElementsIndices;
  GLint MaxElementsVertices;


  // per OpenGL implementation, 1.2.1, optional
  GLint MaxColorMatrixStackDepth;
  GLint MaxConvolutionWidth[3];
  GLint MaxConvolutionHeight[2];

  // per OpenGL implementation, 1.3
  GLint MaxCubeMapTextureSize;
  GLint MaxTextureUnits;
  GLint CompressedTextureFormats;
  GLint NumCompressedTextureFormats;

  // per framebuffer, 1.3
  GLint SampleBuffers;
  GLint Samples;

  // per OpenGL implementation, 1.4
  GLfloat MaxTextureLODBias;

  // per OpenGL implementation, 1.5
  GLint QueryCounterBits;

  // per OpenGL implementations, 2.0
  GLubyte *Extensions;
  GLubyte *Renderer;
  GLubyte *ShadingLanguageVersion;
  GLubyte *Vendor;
  GLubyte *Version;
  GLint MaxVertexAttribs;
  GLint MaxVertexUniformComponents;
  GLint MaxVaryingFloats;
  GLint MaxVertexTextureImageUnits;
  GLint MaxTextureImageUnits;
  GLint MaxFragmentUniformComponents;

  // Misc.

  // - Display lists
  GLint ListBase;
  GLint ListIndex;
  GLint ListMode;


  // - Current depth of stacks
  GLint AttribStackDepth;
  GLint ClientAtribStackDepth;
  GLint NameStackDepth;

  GLint RenderMode;

  // - Selection buffer
  GLvoid *SelectionBufferPointer;
  GLint SelectionBufferSize;

  // - Feedback buffer
  GLvoid *FeedbackBufferPointer;
  GLint FeedbackBufferSize;
  GLint FeedbackBufferType;

  // - error code
  GLenum ErrorCode;

  // Misc, 1.5
  GLint CurrentQuery;

  // Framebuffer, GL_EXT_framebuffer_object
  GLint FrameBufferBinding;
  GLint Read;

protected:
  void UpdateCurrentProgram();
  void UpdateShader(size_t i);

  void ColorBufferToStream(ostream &os,GLint colorBuffer);
  const char *ErrorCodeToString();
  const char *BlendFuncToString(GLint blendFunc);
  const char *BlendEquationToString(GLint blendEquation);
  const char *LogicOpModeToString();
  const char *ListModeToString();
  const char *BooleanToString(GLint booleanValue);
  const char *ShadeModelToString();
  const char *CullFaceModeToString();
  const char *FrontFaceToString();
  const char *PolygonModeToString(GLint polygonMode);
  const char *AlphaTestFuncToString();
  const char *DepthFuncToString();
  const char *RenderModeToString();
  const char *MatrixModeToString();

  const char *ValueToString(GLint value,
                            int valueTable[],
                            const char *stringTable[],
                            int tableSize);

  void PrintMatrix(ostream &os,
                   vtkIndent indent,
                   GLfloat matrix[16]);

  vtkOpenGLRenderWindow *Context;
};



#endif
// VTK-HeaderTest-Exclude: vtkOpenGLState.h
