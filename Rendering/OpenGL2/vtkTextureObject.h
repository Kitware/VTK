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
/**
 * @class   vtkTextureObject
 * @brief   abstracts an OpenGL texture object.
 *
 * vtkTextureObject represents an OpenGL texture object. It provides API to
 * create textures using data already loaded into pixel buffer objects. It can
 * also be used to create textures without uploading any data.
*/

#ifndef vtkTextureObject_h
#define vtkTextureObject_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"
#include "vtkWeakPointer.h" // for render context

class vtkOpenGLBufferObject;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;
class vtkWindow;
class vtkGenericOpenGLResourceFreeCallback;

#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
class vtkPixelBufferObject;
#endif

class VTKRENDERINGOPENGL2_EXPORT vtkTextureObject : public vtkObject
{
public:

  // DepthTextureCompareFunction values.
  enum
  {
    Lequal=0, // r=R<=Dt ? 1.0 : 0.0
    Gequal, // r=R>=Dt ? 1.0 : 0.0
    Less, // r=R<D_t ? 1.0 : 0.0
    Greater, // r=R>Dt ? 1.0 : 0.0
    Equal, // r=R==Dt ? 1.0 : 0.0
    NotEqual, // r=R!=Dt ? 1.0 : 0.0
    AlwaysTrue, //  r=1.0 // WARNING "Always" is macro defined in X11/X.h...
    Never, // r=0.0
    NumberOfDepthTextureCompareFunctions
  };

// ClampToBorder is not supported in ES 2.0
// Wrap values.
#if GL_ES_VERSION_2_0 != 1
  enum
  {
    ClampToEdge=0,
    Repeat,
    MirroredRepeat,
    ClampToBorder,
    NumberOfWrapModes
  };
#else
  enum
  {
    ClampToEdge=0,
    Repeat,
    MirroredRepeat,
    NumberOfWrapModes
  };
#endif

  // MinificationFilter values.
  enum
  {
    Nearest=0,
    Linear,
    NearestMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapNearest,
    LinearMipmapLinear,
    NumberOfMinificationModes
  };

  // Internal depth format
  enum
  {
    Native=0, // will try to match with the depth buffer format.
    Fixed16,
    Fixed24,
    Fixed32,
    Float32,
    NumberOfDepthFormats
  };

  // Internal alpha format
  enum
  {
    alpha=0,
    alpha8,
    alpha16,
    alpha16f,
    alpha32f,
    NumberOfAlphaFormats
  };

  // Depth mode formats
  enum
  {
    DepthAlpha=0,
    DepthLuminance,
    DepthIntensity,
    NumberOfDepthModeFormats
  };

  static vtkTextureObject* New();
  vtkTypeMacro(vtkTextureObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get/Set the context. This does not increase the reference count of the
   * context to avoid reference loops.

   * {
   * this->TextureObject = vtkTextureObject::New();
   * }SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(vtkOpenGLRenderWindow*);
  vtkOpenGLRenderWindow* GetContext();
  //@}

  //@{
  /**
   * Get the texture dimensions.
   * These are the properties of the OpenGL texture this instance represents.
   */
  vtkGetMacro(Width, unsigned int);
  vtkGetMacro(Height, unsigned int);
  vtkGetMacro(Depth, unsigned int);
  vtkGetMacro(Components, int);
  unsigned int GetTuples()
  { return this->Width*this->Height*this->Depth; }
  //@}

  vtkGetMacro(NumberOfDimensions, int);

  //@{
  /**
   * Returns OpenGL texture target to which the texture is/can be bound.
   */
  vtkGetMacro(Target, unsigned int);
  //@}

  //@{
  /**
   * Returns the OpenGL handle.
   */
  vtkGetMacro(Handle, unsigned int);
  //@}

  /**
   * Return the texture unit used for this texture
   */
  int GetTextureUnit();

  //@{
  /**
   * Bind UnBind The texture must have been created using Create().
   * A side affect is that tex paramteres are sent.
   * RenderWindow must be set before calling this.
   */
  void Bind();
  void UnBind();
  //@}

  /**
   * Activate and Bind the texture
   */
  void Activate();

  /**
   * Deactivate and UnBind the texture
   */
  void Deactivate();

  /**
   * Deactivate and UnBind the texture
   */
  void ReleaseGraphicsResources(vtkWindow *win);

  /**
   * Tells if the texture object is bound to the active texture image unit.
   * (a texture object can be bound to multiple texture image unit).
   */
  bool IsBound();

  //@{
  /**
   * Send all the texture object parameters to the hardware if not done yet.
   * Parameters are automatically sent as a side affect of Bind. Disable
   * this by setting AutoParameters 0.
   * \pre is_bound: IsBound()
   */
  void SendParameters();
  vtkSetMacro(AutoParameters, int);
  vtkGetMacro(AutoParameters, int);
  //@}

  /**
   * Create a 2D texture from client memory
   * numComps must be in [1-4].
   */
  bool Create2DFromRaw(unsigned int width, unsigned int height,
                       int numComps,  int dataType, void *data);

  /**
   * Create a 2D depth texture using a raw pointer.
   * This is a blocking call. If you can, use PBO instead.
   */
  bool CreateDepthFromRaw(unsigned int width, unsigned int height,
                          int internalFormat, int rawType,
                          void *raw);

  /**
   * Create a texture buffer basically a 1D texture that can be
   * very large for passing data into the fragment shader
   */
  bool CreateTextureBuffer(unsigned int numValues, int numComps,
                           int dataType, vtkOpenGLBufferObject *bo);

  /**
   * Create a cube texture from 6 buffers from client memory.
   * Image data must be provided in the following order: +X -X +Y -Y +Z -Z.
   * numComps must be in [1-4].
   */
  bool CreateCubeFromRaw(unsigned int width, unsigned int height,
                         int numComps, int dataType, void *data[6]);

// 1D  textures are not supported in ES 2.0 or 3.0
#if GL_ES_VERSION_2_0 != 1

  /**
   * Create a 1D texture using the PBO.
   * Eventually we may start supporting creating a texture from subset of data
   * in the PBO, but for simplicity we'll begin with entire PBO data.
   * numComps must be in [1-4].
   * shaderSupportsTextureInt is true if the shader has an alternate
   * implementation supporting sampler with integer values.
   * Even if the card supports texture int, it does not mean that
   * the implementor of the shader made a version that supports texture int.
   */
  bool Create1D(int numComps,
                vtkPixelBufferObject *pbo,
                bool shaderSupportsTextureInt);

  /**
   * Create 1D texture from client memory
   */
  bool Create1DFromRaw(unsigned int width, int numComps,
                       int dataType, void *data);
  /**
   * Create a 1D alpha texture using a raw pointer.
   * This is a blocking call. If you can, use PBO instead.
   */
  bool CreateAlphaFromRaw(unsigned int width,
                          int internalFormat,
                          int rawType,
                          void *raw);
#endif

// PBO's, and 3D textures are not supported in ES 2.0
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1

  /**
   * Create a 2D texture using the PBO.
   * Eventually we may start supporting creating a texture from subset of data
   * in the PBO, but for simplicity we'll begin with entire PBO data.
   * numComps must be in [1-4].
   */
  bool Create2D(unsigned int width, unsigned int height, int numComps,
                vtkPixelBufferObject *pbo,
                bool shaderSupportsTextureInt);

  /**
   * Create a 3D texture using the PBO.
   * Eventually we may start supporting creating a texture from subset of data
   * in the PBO, but for simplicity we'll begin with entire PBO data.
   * numComps must be in [1-4].
   */
  bool Create3D(unsigned int width, unsigned int height, unsigned int depth,
                int numComps, vtkPixelBufferObject *pbo,
                bool shaderSupportsTextureInt);

  /**
   * Create a 3D texture from client memory
   * numComps must be in [1-4].
   */
  bool Create3DFromRaw(unsigned int width, unsigned int height,
                       unsigned int depth, int numComps,
                       int dataType, void *data);

  /**
   * This is used to download raw data from the texture into a pixel bufer. The
   * pixel buffer API can then be used to download the pixel buffer data to CPU
   * arrays. The caller takes on the responsibility of deleting the returns
   * vtkPixelBufferObject once it done with it.
   */
  vtkPixelBufferObject* Download();

  /**
   * Create a 2D depth texture using a PBO.
   * \pre: valid_internalFormat: internalFormat>=0 && internalFormat<NumberOfDepthFormats
   */
  bool CreateDepth(unsigned int width,
                   unsigned int height,
                   int internalFormat,
                   vtkPixelBufferObject *pbo);

#endif

  /**
   * Create a 2D depth texture but does not initialize its values.
   */
  bool AllocateDepth(unsigned int width, unsigned int height,
                     int internalFormat);

  /**
   * Create a 1D color texture but does not initialize its values.
   * Internal format is deduced from numComps and vtkType.
   */
  bool Allocate1D(unsigned int width, int numComps, int vtkType);

  /**
   * Create a 2D color texture but does not initialize its values.
   * Internal format is deduced from numComps and vtkType.
   */
  bool Allocate2D(unsigned int width, unsigned int height, int numComps,
                  int vtkType);

  /**
   * Create a 3D color texture but does not initialize its values.
   * Internal format is deduced from numComps and vtkType.
   */
  bool Allocate3D(unsigned int width, unsigned int height,
                  unsigned int depth, int numComps,
                  int vtkType);


  //@{
  /**
   * Create texture without uploading any data.
   */
  bool Create2D(unsigned int width, unsigned int height, int numComps,
                int vtktype, bool shaderSupportsTextureInt);
  bool Create3D(unsigned int width, unsigned int height, unsigned int depth,
                int numComps, int vtktype, bool shaderSupportsTextureInt);
  //@}

  /**
   * Get the data type for the texture as a vtk type int i.e. VTK_INT etc.
   */
  int GetVTKDataType();

  //@{
  /**
   * Get the data type for the texture as GLenum type.
   */
  int GetDataType(int vtk_scalar_type);
  void SetDataType(unsigned int glType);
  int GetDefaultDataType(int vtk_scalar_type);
  //@}

  //@{
  /**
   * Get/Set internal format (OpenGL internal format) that should
   * be used.
   * (https://www.opengl.org/sdk/docs/man2/xhtml/glTexImage2D.xml)
   */
  unsigned int GetInternalFormat(int vtktype, int numComps,
                                 bool shaderSupportsTextureInt);
  void SetInternalFormat(unsigned int glInternalFormat);
  unsigned int GetDefaultInternalFormat(int vtktype, int numComps,
                                 bool shaderSupportsTextureInt);
  //@}

  //@{
  /**
   * Get/Set format (OpenGL internal format) that should
   * be used.
   * (https://www.opengl.org/sdk/docs/man2/xhtml/glTexImage2D.xml)
   */
  unsigned int GetFormat(int vtktype, int numComps,
                         bool shaderSupportsTextureInt);
  void SetFormat(unsigned int glFormat);
  unsigned int GetDefaultFormat(int vtktype, int numComps,
                         bool shaderSupportsTextureInt);
  //@}

  /**
   * Reset format, internal format, and type of the texture.

   * This method is useful when a texture is reused in a
   * context same as the previous render call. In such
   * cases, texture destruction does not happen and therefore
   * previous set values are used.
   */
  void ResetFormatAndType();

  unsigned int GetMinificationFilterMode(int vtktype);
  unsigned int GetMagnificationFilterMode(int vtktype);
  unsigned int GetWrapSMode(int vtktype);
  unsigned int GetWrapTMode(int vtktype);
  unsigned int GetWrapRMode(int vtktype);

  //@{
  /**
   * Optional, require support for floating point depth buffer
   * formats. If supported extensions will be loaded, however
   * loading will fail if the extension is required but not
   * available.
   */
  vtkSetMacro(RequireDepthBufferFloat, bool);
  vtkGetMacro(RequireDepthBufferFloat, bool);
  vtkGetMacro(SupportsDepthBufferFloat, bool);
  //@}

  //@{
  /**
   * Optional, require support for floating point texture
   * formats. If supported extensions will be loaded, however
   * loading will fail if the extension is required but not
   * available.
   */
  vtkSetMacro(RequireTextureFloat, bool);
  vtkGetMacro(RequireTextureFloat, bool);
  vtkGetMacro(SupportsTextureFloat, bool);
  //@}

  //@{
  /**
   * Optional, require support for integer texture
   * formats. If supported extensions will be loaded, however
   * loading will fail if the extension is required but not
   * available.
   */
  vtkSetMacro(RequireTextureInteger, bool);
  vtkGetMacro(RequireTextureInteger, bool);
  vtkGetMacro(SupportsTextureInteger, bool);
  //@}

  //@{
  /**
   * Wrap mode for the first texture coordinate "s"
   * Valid values are:
   * - Clamp
   * - ClampToEdge
   * - Repeat
   * - ClampToBorder
   * - MirroredRepeat
   * Initial value is Repeat (as in OpenGL spec)
   */
  vtkGetMacro(WrapS, int);
  vtkSetMacro(WrapS, int);
  //@}

  //@{
  /**
   * Wrap mode for the first texture coordinate "t"
   * Valid values are:
   * - Clamp
   * - ClampToEdge
   * - Repeat
   * - ClampToBorder
   * - MirroredRepeat
   * Initial value is Repeat (as in OpenGL spec)
   */
  vtkGetMacro(WrapT, int);
  vtkSetMacro(WrapT, int);
  //@}

  //@{
  /**
   * Wrap mode for the first texture coordinate "r"
   * Valid values are:
   * - Clamp
   * - ClampToEdge
   * - Repeat
   * - ClampToBorder
   * - MirroredRepeat
   * Initial value is Repeat (as in OpenGL spec)
   */
  vtkGetMacro(WrapR, int);
  vtkSetMacro(WrapR, int);
  //@}

  //@{
  /**
   * Minification filter mode.
   * Valid values are:
   * - Nearest
   * - Linear
   * - NearestMipmapNearest
   * - NearestMipmapLinear
   * - LinearMipmapNearest
   * - LinearMipmapLinear
   * Initial value is Nearest (note initial value in OpenGL spec
   * is NearestMipMapLinear but this is error-prone because it makes the
   * texture object incomplete. ).
   */
  vtkGetMacro(MinificationFilter, int);
  vtkSetMacro(MinificationFilter, int);
  //@}

  //@{
  /**
   * Magnification filter mode.
   * Valid values are:
   * - Nearest
   * - Linear
   * Initial value is Nearest
   */
  vtkGetMacro(MagnificationFilter, int);
  vtkSetMacro(MagnificationFilter, int);
  //@}

  /**
   * Tells if the magnification mode is linear (true) or nearest (false).
   * Initial value is false (initial value in OpenGL spec is true).
   */
  void SetLinearMagnification(bool val)
  { this->SetMagnificationFilter(val?Linear:Nearest); }

  bool GetLinearMagnification()
  { return this->MagnificationFilter==Linear; }

  //@{
  /**
   * Border Color (RGBA). The values can be any valid float value,
   * if the gpu supports it. Initial value is (0.0f, 0.0f, 0.0f, 0.0f),
   * as in the OpenGL spec.
   */
  vtkSetVector4Macro(BorderColor, float);
  vtkGetVector4Macro(BorderColor, float);
  //@}

  //@{
  /**
   * Lower-clamp the computed LOD against this value. Any float value is valid.
   * Initial value is -1000.0f, as in OpenGL spec.
   */
  vtkSetMacro(MinLOD, float);
  vtkGetMacro(MinLOD, float);
  //@}

  //@{
  /**
   * Upper-clamp the computed LOD against this value. Any float value is valid.
   * Initial value is 1000.0f, as in OpenGL spec.
   */
  vtkSetMacro(MaxLOD, float);
  vtkGetMacro(MaxLOD, float);
  //@}

  //@{
  /**
   * Level of detail of the first texture image. A texture object is a list of
   * texture images. It is a non-negative integer value.
   * Initial value is 0, as in OpenGL spec.
   */
  vtkSetMacro(BaseLevel, int);
  vtkGetMacro(BaseLevel, int);
  //@}

  //@{
  /**
   * Level of detail of the first texture image. A texture object is a list of
   * texture images. It is a non-negative integer value.
   * Initial value is 1000, as in OpenGL spec.
   */
  vtkSetMacro(MaxLevel, int);
  vtkGetMacro(MaxLevel, int);
  //@}

  //@{
  /**
   * Tells if the output of a texture unit with a depth texture uses
   * comparison or not.
   * Comparison happens between D_t the depth texture value in the range [0,1]
   * and with R the interpolated third texture coordinate clamped to range
   * [0,1]. The result of the comparison is noted `r'. If this flag is false,
   * r=D_t.
   * Initial value is false, as in OpenGL spec.
   * Ignored if the texture object is not a depth texture.
   */
  vtkGetMacro(DepthTextureCompare, bool);
  vtkSetMacro(DepthTextureCompare, bool);
  //@}

  //@{
  /**
   * In case DepthTextureCompare is true, specify the comparison function in
   * use. The result of the comparison is noted `r'.
   * Valid values are:
   * - Value
   * - Lequal: r=R<=Dt ? 1.0 : 0.0
   * - Gequal: r=R>=Dt ? 1.0 : 0.0
   * - Less: r=R<D_t ? 1.0 : 0.0
   * - Greater: r=R>Dt ? 1.0 : 0.0
   * - Equal: r=R==Dt ? 1.0 : 0.0
   * - NotEqual: r=R!=Dt ? 1.0 : 0.0
   * - AlwaysTrue: r=1.0
   * - Never: r=0.0
   * If the magnification of minification factor are not nearest, percentage
   * closer filtering (PCF) is used: R is compared to several D_t and r is
   * the average of the comparisons (it is NOT the average of D_t compared
   * once to R).
   * Initial value is Lequal, as in OpenGL spec.
   * Ignored if the texture object is not a depth texture.
   */
  vtkGetMacro(DepthTextureCompareFunction, int);
  vtkSetMacro(DepthTextureCompareFunction, int);
  //@}

  //@{
  /**
   * Tells the hardware to generate mipmap textures from the first texture
   * image at BaseLevel.
   * Initial value is false, as in OpenGL spec.
   */
  vtkGetMacro(GenerateMipmap, bool);
  vtkSetMacro(GenerateMipmap, bool);
  //@}

  /**
   * Query and return maximum texture size (dimension) supported by the
   * OpenGL driver for a particular context. It should be noted that this
   * size does not consider the internal format of the texture and therefore
   * there is no guarentee that a texture of this size will be allocated by
   * the driver. Also, the method does not make the context current so
   * if the passed context is not valid or current, a value of -1 will
   * be returned.
   */
  static int GetMaximumTextureSize(vtkOpenGLRenderWindow* context);

  /**
   * Returns if the context supports the required extensions. If flags
   * for optional extenisons are set then the test fails when support
   * for them is not found.
   */
  static bool IsSupported(
        vtkOpenGLRenderWindow* renWin,
        bool requireTexFloat,
        bool requireDepthFloat,
        bool requireTexInt);

  /**
   * Check for feature support, without any optional features.
   */
  static bool IsSupported(vtkOpenGLRenderWindow* renWin)
    { return vtkTextureObject::IsSupported(renWin, false, false, false); }

  //@{
  /**
   * Copy the texture (src) in the current framebuffer.  A variety of
   * signatures based on what you want to do
   * Copy the entire texture to the entire current viewport
   */
  void CopyToFrameBuffer(vtkShaderProgram *program,
                         vtkOpenGLVertexArrayObject *vao);
  // part of a texture to part of a viewport, scaling as needed
  void CopyToFrameBuffer(int srcXmin, int srcYmin,
                         int srcXmax, int srcYmax,
                         int dstXmin, int dstYmin,
                         int dstXmax, int dstYmax,
                         int dstSizeX, int dstSizeY,
                         vtkShaderProgram *program,
                         vtkOpenGLVertexArrayObject *vao
                         );
  // copy part of a texure to part of a viewport, no scalaing
  void CopyToFrameBuffer(int srcXmin, int srcYmin,
                         int srcXmax, int srcYmax,
                         int dstXmin, int dstYmin,
                         int dstSizeX, int dstSizeY,
                         vtkShaderProgram *program,
                         vtkOpenGLVertexArrayObject *vao
                         );
  // copy a texture to a quad using the provided tcoords and verts
  void CopyToFrameBuffer(float *tcoords, float *verts,
                         vtkShaderProgram *program,
                         vtkOpenGLVertexArrayObject *vao
                         );
  //@}


  /**
   * Copy a sub-part of a logical buffer of the framebuffer (color or depth)
   * to the texture object. src is the framebuffer, dst is the texture.
   * (srcXmin,srcYmin) is the location of the lower left corner of the
   * rectangle in the framebuffer. (dstXmin,dstYmin) is the location of the
   * lower left corner of the rectangle in the texture. width and height
   * specifies the size of the rectangle in pixels.
   * If the logical buffer is a color buffer, it has to be selected first with
   * glReadBuffer().
   * \pre is2D: GetNumberOfDimensions()==2
   */
  void CopyFromFrameBuffer(int srcXmin,
                           int srcYmin,
                           int dstXmin,
                           int dstYmin,
                           int width,
                           int height);

  /**
   * Get the shift and scale required in the shader to
   * return the texture values to their original range.
   * Thsi is useful when for example you have unsigned char
   * data and it is being accessed using the floating point
   * texture calls. In that case OpenGL maps the uchar
   * range to a different floating point range under the hood.
   * Applying the shift and scale will return the data to
   * its original values in the shader. The texture's
   * internal format must be set before calling these
   * routines. Creating the texture does set it.
   */
  void GetShiftAndScale(float &shift, float &scale);

protected:
  vtkTextureObject();
  ~vtkTextureObject();

  vtkGenericOpenGLResourceFreeCallback *ResourceCallback;

  /**
   * Load all necessary extensions.
   */
  bool LoadRequiredExtensions(vtkOpenGLRenderWindow *renWin);

  /**
   * Creates a texture handle if not already created.
   */
  void CreateTexture();

  /**
   * Destroy the texture.
   */
  void DestroyTexture();

  int NumberOfDimensions;
  unsigned int Width;
  unsigned int Height;
  unsigned int Depth;

  unsigned int Target; // GLenum
  unsigned int Format; // GLenum
  unsigned int InternalFormat; // GLenum
  unsigned int Type; // GLenum
  int Components;

  vtkWeakPointer<vtkOpenGLRenderWindow> Context;
  unsigned int Handle;
  bool RequireTextureInteger;
  bool SupportsTextureInteger;
  bool RequireTextureFloat;
  bool SupportsTextureFloat;
  bool RequireDepthBufferFloat;
  bool SupportsDepthBufferFloat;

  int WrapS;
  int WrapT;
  int WrapR;
  int MinificationFilter;
  int MagnificationFilter;

  float MinLOD;
  float MaxLOD;
  int BaseLevel;
  int MaxLevel;
  float BorderColor[4];

  bool DepthTextureCompare;
  int DepthTextureCompareFunction;

  bool GenerateMipmap;

  int AutoParameters;
  vtkTimeStamp SendParametersTime;

  // used for copying to framebuffer
  vtkOpenGLHelper *ShaderProgram;

  // for texturebuffers we hold on to the Buffer
  vtkOpenGLBufferObject *BufferObject;

private:
  vtkTextureObject(const vtkTextureObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTextureObject&) VTK_DELETE_FUNCTION;
};

#endif
