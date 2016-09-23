/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineIntegralConvolution2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLineIntegralConvolution2D
 * @brief   GPU-based implementation of Line
 *  Integral Convolution (LIC)
 *
 *
 *  This class resorts to GLSL to implement GPU-based Line Integral Convolution
 *  (LIC) for visualizing a 2D vector field that may be obtained by projecting
 *  an original 3D vector field onto a surface (such that the resulting 2D
 *  vector at each grid point on the surface is tangential to the local normal,
 *  as done in vtkSurfaceLICPainter).
 *
 *  As an image-based technique, 2D LIC works by (1) integrating a bidirectional
 *  streamline from the center of each pixel (of the LIC output image), (2)
 *  locating the pixels along / hit by this streamline as the correlated pixels
 *  of the starting pixel (seed point / pixel), (3) indexing a (usually white)
 *  noise texture (another input to LIC, in addition to the 2D vector field,
 *  usually with the same size as that of the 2D vetor field) to determine the
 *  values (colors) of these pixels (the starting and the correlated pixels),
 *  typically through bi-linear interpolation, and (4) performing convolution
 *  (weighted averaging) on these values, by adopting a low-pass filter (such
 *  as box, ramp, and Hanning kernels), to obtain the result value (color) that
 *  is then assigned to the seed pixel.
 *
 *  The GLSL-based GPU implementation herein maps the aforementioned pipeline to
 *  fragment shaders and a box kernel is employed. Both the white noise and the
 *  vector field are provided to the GPU as texture objects (supported by the
 *  multi-texturing capability). In addition, there are four texture objects
 *  (color buffers) allocated to constitute two pairs that work in a ping-pong
 *  fashion, with one as the read buffers and the other as the write / render
 *  targets. Maintained by a frame buffer object (GL_EXT_framebuffer_object),
 *  each pair employs one buffer to store the current (dynamically updated)
 *  position (by means of the texture coordinate that keeps being warped by the
 *  underlying vector) of the (virtual) particle initially released from each
 *  fragment while using the bother buffer to store the current (dynamically
 *  updated too) accumulated texture value that each seed fragment (before the
 *  'mesh' is warped) collects. Given NumberOfSteps integration steps in each
 *  direction, there are a total of (2 * NumberOfSteps + 1) fragments (including
 *  the seed fragment) are convolved and each contributes 1 / (2 * NumberOfSteps
 *  + 1) of the associated texture value to fulfill the box filter.
 *
 *  One pass of LIC (basic LIC) tends to produce low-contrast / blurred images and
 *  vtkLineIntegralConvolution2D provides an option for creating enhanced LIC
 *  images. Enhanced LIC improves image quality by increasing inter-streamline
 *  contrast while suppressing artifacts. It performs two passes of LIC, with a
 *  3x3 Laplacian high-pass filter in between that processes the output of pass
 *  #1 LIC and forwards the result as the input 'noise' to pass #2 LIC.
 *
 *  vtkLineIntegralConvolution2D applies masking to zero-vector fragments so
 *  that un-filtered white noise areas are made totally transparent by class
 *  vtkSurfaceLICPainter to show the underlying geometry surface.
 *
 *  The convolution process tends to decrease both contrast and dynamic range,
 *  sometimes leading to dull dark images. In order to counteract this, optional
 *  contrast ehnancement stages have been added. These increase the dynamic range and
 *  contrast and sharpen streaking patterns that emerge from the LIC process.
 *
 *  Under some circumstances, typically depending on the contrast and dynamic
 *  range and graininess of the noise texture, jagged or pixelated patterns emerge
 *  in the LIC. These can be reduced by enabling the optional anti-aliasing pass.
 *
 *  The internal pipeline is as follows, with optional stages denoted by ()
 *  nested optional stages depend on their parent stage.
 *  <pre>
 *   noise texture
 *           |
 *           [ LIC ((CE) HPF LIC) (AA) (CE) ]
 *           |                              |
 *  vector field                       LIC'd image
 * </pre>
 *  where LIC is the LIC stage, HPF is the high-pass filter stage, CE is the
 *  contrast ehnacement stage, and AA is the antialias stage.
 *
 * @sa
 *  vtkImageDataLIC2D vtkStructuredGridLIC2D
*/

#ifndef vtkLineIntegralConvolution2D_h
#define vtkLineIntegralConvolution2D_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // for ren context
#include "vtkRenderingLICOpenGL2Module.h" // for export macro
#include <deque> // for deque

class vtkFrameBufferObject2;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkPainterCommunicator;
class vtkPixelExtent;
class vtkRenderWindow;
class vtkShaderProgram;
class vtkTextureObject;

class VTKRENDERINGLICOPENGL2_EXPORT vtkLineIntegralConvolution2D : public vtkObject
{
public:
  static vtkLineIntegralConvolution2D *New();
  vtkTypeMacro(vtkLineIntegralConvolution2D, vtkObject);
  void PrintSelf(ostream & os, vtkIndent indent);

  /**
   * Returns if the context supports the required extensions.
   */
  static bool IsSupported(vtkRenderWindow * renWin);

  //@{
  /**
   * Set/Get the rendering context. A reference is not explicity held,
   * thus refernce to the context must be held externally.
   */
  void SetContext(vtkOpenGLRenderWindow *context);
  vtkOpenGLRenderWindow *GetContext();
  //@}

  //@{
  /**
   * EnhancedLIC mean compute the LIC twice with the second pass using
   * the edge-enhanced result of the first pass as a noise texture. Edge
   * enhancedment is made by a simple Laplace convolution.
   */
  vtkSetClampMacro(EnhancedLIC, int, 0, 1);
  vtkGetMacro(EnhancedLIC, int);
  vtkBooleanMacro(EnhancedLIC, int);
  //@}

  //@{
  /**
   * Enable/Disable contrast and dynamic range correction stages. Stage 1 is applied
   * on the input to the high-pass filter when the high-pass filter is enabled and
   * skipped otherwise. Stage 2, when enabled is the final stage in the internal
   * pipeline. Both stages are implemented by a histogram stretching of the gray scale
   * colors in the LIC'd image as follows:

   * c = (c-m)/(M-m)

   * where, c is the fragment color, m is the color value to map to 0, M is the
   * color value to map to 1. The default values of m and M are the min and max
   * over all fragments.

   * This increase the dynamic range and contrast in the LIC'd image, both of which
   * are natuarly attenuated by the LI conovlution proccess.

   * ENHANCE_CONTRAST_OFF  -- don't enhance contrast
   * ENHANCE_CONTRAST_ON   -- enhance high-pass input and final stage output

   * This feature is disabled by default.
   */
  enum {
    ENHANCE_CONTRAST_OFF=0,
    ENHANCE_CONTRAST_ON=1};
  vtkSetClampMacro(EnhanceContrast, int, 0, 2);
  vtkGetMacro(EnhanceContrast, int);
  vtkBooleanMacro(EnhanceContrast, int);
  //@}

  //@{
  /**
   * This feature is used to fine tune the contrast enhancement. Values are provided
   * indicating the fraction of the range to adjust m and M by during contrast enahncement
   * histogram stretching.  M and m are the intensity/lightness values that map to 1 and 0.
   * (see EnhanceContrast for an explanation of the mapping procedure). m and M are computed
   * using the factors as follows:

   * m = min(C) - mFactor * (max(C) - min(C))
   * M = max(C) - MFactor * (max(C) - min(C))

   * the default values for mFactor and MFactor are 0 which result in
   * m = min(C), M = max(C), where C is all of the colors in the image. Adjusting
   * mFactor and MFactor above zero provide a means to control the saturation of
   * normalization. These settings only affect the final normalization, the
   * normalization that occurs on the input to the high-pass filter always uses
   * the min and max.
   */
  vtkSetClampMacro(LowContrastEnhancementFactor, double, 0.0, 1.0);
  vtkGetMacro(LowContrastEnhancementFactor, double);
  vtkSetClampMacro(HighContrastEnhancementFactor, double, 0.0, 1.0);
  vtkGetMacro(HighContrastEnhancementFactor, double);
  //@}

  //@{
  /**
   * Enable/Disable the anti-aliasing pass. This optional pass (disabled by
   * default) can be enabled to reduce jagged patterns in the final LIC image.
   * Values greater than 0 control the number of iterations, one is typically
   * sufficient.
   */
  vtkSetClampMacro(AntiAlias, int, 0, VTK_INT_MAX);
  vtkGetMacro(AntiAlias, int);
  vtkBooleanMacro(AntiAlias, int);
  //@}

  //@{
  /**
   * Number of streamline integration steps (initial value is 1).
   * In term of visual quality, the greater (within some range) the better.
   */
  vtkSetClampMacro(NumberOfSteps, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfSteps, int);
  //@}

  //@{
  /**
   * Get/Set the streamline integration step size (0.01 by default). This is
   * the length of each step in normalized image space i.e. in range [0, FLOAT_MAX].
   * In term of visual quality, the smaller the better. The type for the
   * interface is double as VTK interface is, but GPU only supports float.
   * Thus it will be converted to float in the execution of the algorithm.
   */
  vtkSetClampMacro(StepSize, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(StepSize, double);
  //@}

  //@{
  /**
   * If VectorField has >= 3 components, we must choose which 2 components
   * form the (X, Y) components for the vector field. Must be in the range
   * [0, 3].
   */
  void SetComponentIds(int c0, int c1);
  void SetComponentIds(int c[2]){ this->SetComponentIds(c[0], c[1]); }
  vtkGetVector2Macro(ComponentIds, int);
  //@}

  //@{
  /**
   * Set the max noise value for use during LIC integration normalization.
   * The integration normalization factor is the max noise value times the
   * number of steps taken. The default value is 1.
   */
  vtkSetClampMacro(MaxNoiseValue, double, 0.0, 1.0);
  vtkGetMacro(MaxNoiseValue, double);
  //@}

  //@{
  /**
   * This class performs LIC in the normalized image space. Hence, by default
   * it transforms the input vectors to the normalized image space (using the
   * GridSpacings and input vector field dimensions). Set this to 0 to disable
   * tranformation if the vectors are already transformed.
   */
  void SetTransformVectors(int val);
  vtkGetMacro(TransformVectors, int);
  //@}

  /**
   * Set/Get the spacing in each dimension of the plane on which the vector
   * field is defined. This class performs LIC in the normalized image space
   * and hence generally it needs to transform the input vector field (given
   * in physical space) to the normalized image space. The Spacing is needed
   * to determine the transform. Default is (1.0, 1.0). It is possible to
   * disable vector transformation by setting TransformVectors to 0.
   * vtkSetVector2Macro(GridSpacings, double);
   * vtkGetVector2Macro(GridSpacings, double);
   */

  //@{
  /**
   * Normalize vectors during integration. When set(the default) the input vector field
   * is normalized during integration, and each integration occurs over the same arclength.
   * When not set each integration occurs over an arc length proportional to the field
   * magnitude as is customary in traditional numerical methods. See, "Imaging Vector
   * Fields Using Line Integral Convolution" for an axample where normalization is used.
   * See, "Image Space Based Visualization of Unsteady Flow on Surfaces" for an example
   * of where no normalization is used.
   */
  void SetNormalizeVectors(int val);
  vtkGetMacro(NormalizeVectors, int);
  //@}

  //@{
  /**
   * The MaskThreshold controls blanking of the LIC texture. For fragments with
   * |V|<threhold the LIC fragment is not rendered. The default value is 0.0.

   * For surface LIC MaskThreshold units are in the original vector space. For image LIC
   * be aware that while the vector field is transformed to image space while the mask
   * threshold is not. Therefore the mask threshold must be specified in image space
   * units.
   */
  vtkSetClampMacro(MaskThreshold, double, -1.0, VTK_FLOAT_MAX);
  vtkGetMacro(MaskThreshold, double);
  //@}


  /**
   * Compute the lic on the entire vector field texture.
   */
  vtkTextureObject *Execute(
        vtkTextureObject *vectorTex,
        vtkTextureObject *noiseTex);

  /**
   * Compute the lic on the indicated subset of the vector field
   * texture.
   */
  vtkTextureObject *Execute(
        const int extent[4],
        vtkTextureObject *vectorTex,
        vtkTextureObject *noiseTex);

  /**
   * Compute LIC over the desired subset of the input texture. The
   * result is copied into the desired subset of the provided output
   * texture.

   * inputTexExtent  : screen space extent of the input texture
   * vectorExtent    : part of the inpute extent that has valid vectors
   * licExtent       : part of the inpute extent to compute on
   * outputTexExtent : screen space extent of the output texture
   * outputExtent    : part of the output texture to store the result
   */
  vtkTextureObject *Execute(
        const vtkPixelExtent &inputTexExtent,
        const std::deque<vtkPixelExtent> &vectorExtent,
        const std::deque<vtkPixelExtent> &licExtent,
        vtkTextureObject *vectorTex,
        vtkTextureObject *maskVectorTex,
        vtkTextureObject *noiseTex);

  /**
   * Convenience functions to ensure that the input textures are
   * configured correctly.
   */
  static
  void SetVectorTexParameters(vtkTextureObject *vectors);

  static
  void SetNoiseTexParameters(vtkTextureObject *noise);

  /**
   * Set the communicator to use during parallel operation
   * The communicator will not be duplicated or reference
   * counted for performance reasons thus caller should
   * hold/manage reference to the communicator during use
   * of the LIC object.
   */
  virtual void SetCommunicator(vtkPainterCommunicator *){}
  virtual vtkPainterCommunicator *GetCommunicator();

  /**
   * For parallel operation, find global min/max
   * min/max are in/out.
   */
 virtual void GetGlobalMinMax(
       vtkPainterCommunicator*,
       float&,
       float&) {}

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * vtkLineIntegralConviolution2DTIME to enable benchmarks.
   * During each update timing information is stored, it can
   * be written to disk by calling WriteLog.
   */
  virtual void WriteTimerLog(const char *){}

protected:
  vtkLineIntegralConvolution2D();
  virtual ~vtkLineIntegralConvolution2D();

  vtkPainterCommunicator *Comm;

  void SetVTShader(vtkShaderProgram *prog);
  void SetLIC0Shader(vtkShaderProgram *prog);
  void SetLICIShader(vtkShaderProgram *prog);
  void SetLICNShader(vtkShaderProgram *prog);
  void SetEEShader(vtkShaderProgram *prog);
  void SetCEShader(vtkShaderProgram *prog);
  void SetAAHShader(vtkShaderProgram *prog);
  void SetAAVShader(vtkShaderProgram *prog);

  void BuildShaders();

  void RenderQuad(
        float computeBounds[4],
        vtkPixelExtent computeExtent);

  vtkTextureObject *AllocateBuffer(unsigned int texSize[2]);

  /**
   * Convenience functions to ensure that the input textures are
   * configured correctly.
   */
  void SetNoise2TexParameters(vtkTextureObject *noise);

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * vtkSurfaceLICPainterTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog (defined in vtkSurfaceLICPainter).
   */
  virtual void StartTimerEvent(const char *){}
  virtual void EndTimerEvent(const char *){}

protected:
  vtkWeakPointer<vtkOpenGLRenderWindow> Context;
  vtkFrameBufferObject2 *FBO;

  int ShadersNeedBuild;
  vtkOpenGLHelper *FinalBlendProgram;
  vtkOpenGLHelper *IntermediateBlendProgram;
  vtkOpenGLHelper *VTShader;
  vtkOpenGLHelper *LIC0Shader;
  vtkOpenGLHelper *LICIShader;
  vtkOpenGLHelper *LICNShader;
  vtkOpenGLHelper *EEShader;
  vtkOpenGLHelper *CEShader;
  vtkOpenGLHelper *AAHShader;
  vtkOpenGLHelper *AAVShader;

  int     NumberOfSteps;
  double  StepSize;
  int     EnhancedLIC;
  int     EnhanceContrast;
  double  LowContrastEnhancementFactor;
  double  HighContrastEnhancementFactor;
  int     AntiAlias;
  int     NoiseTextureLookupCompatibilityMode;
  double  MaskThreshold;
  int     TransformVectors;
  int     NormalizeVectors;
  int     ComponentIds[2];
  double  MaxNoiseValue;

private:
  vtkLineIntegralConvolution2D(const vtkLineIntegralConvolution2D &) VTK_DELETE_FUNCTION;
  void operator = (const vtkLineIntegralConvolution2D &) VTK_DELETE_FUNCTION;
};

#endif
