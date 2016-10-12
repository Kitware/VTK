/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSurfaceLICPainter
 * @brief   painter that performs LIC on the surface of
 *  arbitrary geometry.
 *
 *
 *  vtkSurfaceLICPainter painter performs LIC on the surface of arbitrary
 *  geometry. Point vectors are used as the vector field for generating the LIC.
 *  The implementation was originallu  based on "Image Space Based Visualization
 *  on Unsteady Flow on Surfaces" by Laramee, Jobard and Hauser appeared in
 *  proceedings of IEEE Visualization '03, pages 131-138.
 *
 *  Internal pipeline:
 * <pre>
 * noise
 *     |
 *     [ PROJ (GAT) (COMP) LIC2D (SCAT) SHADE (CCE) DEP]
 *     |                                               |
 * vectors                                         surface LIC
 * </pre>
 * PROj  - prject vectors onto surface
 * GAT   - gather data for compositing and guard pixel generation  (parallel only)
 * COMP  - composite gathered data
 * LIC2D - line intengral convolution, see vtkLineIntegralConvolution2D.
 * SCAT  - scatter result (parallel only, not all compositors use it)
 * SHADE - combine LIC and scalar colors
 * CCE   - color contrast enhancement (optional)
 * DEP   - depth test and copy to back buffer
 *
 * The result of each stage is cached in a texture so that during interaction
 * a stage may be skipped if the user has not modified its paramters or input
 * data.
 *
 * The parallel parts of algorithm are implemented in vtkPSurfaceLICPainter.
 * Note that for MPI enabled builds this class will be automatically created
 * by the object factory.
 *
 * @sa
 * vtkSurfaceLICDefaultPainter vtkLineIntegralConvolution2D
*/

#ifndef vtkSurfaceLICPainter_h
#define vtkSurfaceLICPainter_h

#include "vtkRenderingLICModule.h" // For export macro
#include "vtkPainter.h"

class vtkRenderWindow;
class vtkRenderer;
class vtkActor;
class vtkImageData;
class vtkDataObject;
class vtkDataArray;
class vtkPainterCommunicator;

class VTKRENDERINGLIC_EXPORT vtkSurfaceLICPainter : public vtkPainter
{
public:
  static vtkSurfaceLICPainter* New();
  vtkTypeMacro(vtkSurfaceLICPainter, vtkPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release. In this case, releases the display lists.
   */
  virtual void ReleaseGraphicsResources(vtkWindow * win);

  /**
   * Get the output data object from this painter.
   * Overridden to pass the input points (or cells) vectors as the tcoords to
   * the deletage painters. This is required by the internal GLSL shader
   * programs used for generating LIC.
   */
  virtual vtkDataObject* GetOutput();

  //@{
  /**
   * Enable/Disable this painter.
   */
  void SetEnable(int val);
  vtkGetMacro(Enable, int);
  void SetEnableOn(){ this->SetEnable(1); }
  void SetEnableOff(){ this->SetEnable(0); }
  //@}

  //@{
  /**
   * Set the vectors to used for applying LIC. By default point vectors are
   * used. Arguments are same as those passed to
   * vtkAlgorithm::SetInputArrayToProcess except the first 3 arguments i.e. idx,
   * port, connection.
   */
  void SetInputArrayToProcess(int fieldAssociation, const char *name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType);
  //@}

  //@{
  /**
   * Get/Set the number of integration steps in each direction.
   */
  void SetNumberOfSteps(int val);
  vtkGetMacro(NumberOfSteps, int);
  //@}

  //@{
  /**
   * Get/Set the step size (in pixels).
   */
  void SetStepSize(double val);
  vtkGetMacro(StepSize, double);
  //@}

  //@{
  /**
   * Normalize vectors during integration. When set(the default) the
   * input vector field is normalized during integration, and each
   * integration occurs over the same arclength. When not set each
   * integration occurs over an arc length proportional to the field
   * magnitude as is customary in traditional numerical methods. See,
   * "Imaging Vector Fields Using Line Integral Convolution" for an
   * axample where normalization is used. See, "Image Space Based
   * Visualization of Unsteady Flow on Surfaces" for an example
   * of where no normalization is used.
   */
  void SetNormalizeVectors(int val);
  vtkBooleanMacro(NormalizeVectors, int);
  vtkGetMacro(NormalizeVectors, int);
  //@}

  //@{
  /**
   * When set MaskOnSurface computes |V| for use in the fragment masking
   * tests on the surface. When not set the original un-projected
   * un-transformed |V| is used.
   */
  void SetMaskOnSurface(int val);
  vtkBooleanMacro(MaskOnSurface, int);
  vtkGetMacro(MaskOnSurface, int);
  //@}

  //@{
  /**
   * The MaskThreshold controls the rendering of fragments in stagnant
   * regions of flow.  // In these regions LIC noise texture will be masked,
   * where |V| < MaskThreshold is satisifed. The masking process blends a
   * the MaskColor with the scalar color of the surface proportional to
   * MaskIntesnsity. See MaskIntensity for more information on the blending
   * algorithm. This blending allows one control over the masking process
   * so that masked fragments may be: highlighted (by setting a unique
   * mask color and mask intensity > 0), made invisible with and without
   * passing the un-convolved noise texture (by setting mask intensity 0),
   * made to blend into the LIC.

   * MaskThreshold units are in the original vector space. Note that the
   * threshold can be applied to the original vector field or to the surface
   * projected vector field. See MaskOnSurface.
   */
  void SetMaskThreshold(double val);
  vtkGetMacro(MaskThreshold, double);
  //@}

  //@{
  /**
   * The MaskColor is used on masked fragments. The default of (0.5, 0.5, 0.5)
   * makes the masked fragments look similar to the LIC'd fragments. The mask
   * color is applied only when MaskIntensity > 0.
   */
  void SetMaskColor(double *val);
  void SetMaskColor(double r, double g, double b)
    { double rgb[3]={r,g,b}; this->SetMaskColor(rgb); }
  vtkGetVector3Macro(MaskColor, double);
  //@}

  //@{
  /**
   * The MaskIntensity controls the blending of the mask color and the geometry
   * color. The color of masked fragments is given by:

   * c = maskColor * maskIntensity + geomColor * (1 - maskIntensity)

   * The default value of 0.0 results in the geometry color being used.
   */
  void SetMaskIntensity(double val);
  vtkGetMacro(MaskIntensity, double);
  //@}

  //@{
  /**
   * EnhancedLIC mean compute the LIC twice with the second pass using
   * the edge-enhanced result of the first pass as a noise texture. Edge
   * enhancedment is made by a simple Laplace convolution.
   */
  void SetEnhancedLIC(int val);
  vtkGetMacro(EnhancedLIC, int);
  vtkBooleanMacro(EnhancedLIC, int);
  //@}

  //@{
  /**
   * Enable/Disable contrast and dynamic range correction stages. Contrast
   * enhancement can be enabled during LIC computations (See
   * vtkLineINtegralComvolution2D) and after the scalar colors have been
   * combined with the LIC.

   * The best appraoch for using this feature is to enable LIC enhancement,
   * and only if the image is to dark or dull enable COLOR enhancement.

   * Both stages are implemented by a histogram stretching algorithm. During
   * LIC stages the contrast enhancement is applied to gray scale LIC image.
   * During the scalar coloring stage the contrast enhancement is applied to
   * the lightness channel of the color image in HSL color space. The
   * histogram stretching is implemented as follows:

   * L = (L-m)/(M-m)

   * where, L is the fragment intensity/lightness, m is the intensity/lightness
   * to map to 0, M is the intensity/lightness to map to 1. The default values
   * of m and M are the min and max taken over all fragments.

   * This increase the dynamic range and contrast in the LIC'd image, both of
   * which are natuarly attenuated by the convolution proccess.

   * Values

   * ENHANCE_CONTRAST_OFF   -- don't enhance LIC or scalar colors
   * ENHANCE_CONTRAST_LIC   -- enhance in LIC high-pass input and output
   * ENHANCE_CONTRAST_COLOR -- enhance after scalars are combined with LIC
   * ENHANCE_CONTRAST_BOTH  -- enhance in LIC stages and after scalar colors

   * This feature is disabled by default.
   */
  enum {
    ENHANCE_CONTRAST_OFF=0,
    ENHANCE_CONTRAST_LIC=1,
    ENHANCE_CONTRAST_COLOR=3,
    ENHANCE_CONTRAST_BOTH=4
  };
  void SetEnhanceContrast(int val);
  vtkGetMacro(EnhanceContrast, int);
  //@}

  //@{
  /**
   * This feature is used to fine tune the contrast enhancement. There are two
   * modes AUTOMATIC and MANUAL.In AUTOMATIC mode values are provided indicating
   * the fraction of the range to adjust M and m by, during contrast enahncement
   * histogram stretching. M and m are the intensity/lightness values that map
   * to 1 and 0. (see EnhanceContrast for an explanation of the mapping
   * procedure). m and M are computed using the factors as follows:

   * m = min(C) + mFactor * (max(C) - min(C))
   * M = max(C) - MFactor * (max(C) - min(C))

   * the default values for mFactor and MFactor are 0 which result in
   * m = min(C), M = max(C), taken over the entire image. Modifying mFactor and
   * MFactor above or below zero provide control over the saturation/
   * de-saturation during contrast enhancement.
   */
  vtkGetMacro(LowLICContrastEnhancementFactor, double);
  vtkGetMacro(HighLICContrastEnhancementFactor, double);
  void SetLowLICContrastEnhancementFactor(double val);
  void SetHighLICContrastEnhancementFactor(double val);
  //
  vtkGetMacro(LowColorContrastEnhancementFactor, double);
  vtkGetMacro(HighColorContrastEnhancementFactor, double);
  void SetLowColorContrastEnhancementFactor(double val);
  void SetHighColorContrastEnhancementFactor(double val);
  //@}

  //@{
  /**
   * Enable/Disable the anti-aliasing pass. This optional pass (disabled by
   * default) can be enabled to reduce jagged patterns in the final LIC image.
   * Values greater than 0 control the number of iterations, 1 is typically
   * sufficient.
   */
  void SetAntiAlias(int val);
  vtkBooleanMacro(AntiAlias, int);
  vtkGetMacro(AntiAlias, int);
  //@}

  //@{
  /**
   * Set/Get the color mode. The color mode controls how scalar colors are
   * combined with the LIC in the final image. The BLEND mode combines scalar
   * colors with LIC intensities with proportional blending controled by the
   * LICIntensity parameter. The MAP mode combines scalar colors with LIC,
   * by multiplication the HSL represntation of color's lightness.

   * The default is COLOR_MODE_BLEND.
   */
  enum {
    COLOR_MODE_BLEND=0,
    COLOR_MODE_MAP
  };
  void SetColorMode(int val);
  vtkGetMacro(ColorMode, int);
  //@}

  //@{
  /**
   * Factor used when blend mode is set to COLOR_MODE_BLEND. This controls the
   * contribution of the LIC in the final output image as follows:

   * c = LIC * LICIntensity + scalar * (1 - LICIntensity);

   * 0.0 produces same result as disabling LIC altogether, while 1.0 implies
   * show LIC result alone.
   */
  void SetLICIntensity(double val);
  vtkGetMacro(LICIntensity, double);
  //@}

  //@{
  /**
   * Factor used when blend mode is set to COLOR_MODE_MAP. This adds a bias to
   * the LIC image. The purpose of this is to adjust the brightness when a
   * brighter image is desired. The default of 0.0 results in no change. Values
   * gretaer than 0.0 will brighten the image while values less than 0.0 darken
   * the image.
   */
  void SetMapModeBias(double val);
  vtkGetMacro(MapModeBias, double);
  //@}

  //@{
  /**
   * Set the data containing a noise array as active scalars. Active scalars
   * array will be converted into a texture for use as noise in the LIC process.
   * Noise datasets are expected to be gray scale.
   */
  void SetNoiseDataSet(vtkImageData *data);
  vtkImageData *GetNoiseDataSet();
  //@}

  //@{
  /**
   * Set/Get the noise texture source. When not set the default 200x200 white
   * noise texture is used (see VTKData/Data/Data/noise.png). When set a noise
   * texture is generated based on the following parameters:

   * NoiseType               - select noise type. Gaussian, Uniform, etc
   * NoiseTextureSize        - number of pixels in square noise texture(side)
   * NoiseGrainSize          - number of pixels each noise value spans(side)
   * MinNoiseValue           - minimum noise color >=0 && < MaxNoiseValue
   * MaxNoiseValue           - maximum noise color <=1 ** > MinNoiseValue
   * NumberOfNoiseLevels     - number of discrete noise colors
   * ImpulseNoiseProbability - impulse noise is generated when < 1
   * ImpulseNoiseBackgroundValue  - the background color for untouched pixels
   * NoiseGeneratorSeed      - seed the random number generators

   * Changing the noise texture gives one greater control over the look of the
   * final image. The default is 0 which results in the use of a static 200x200
   * Gaussian noise texture. See VTKData/Data/Data/noise.png.
   */
  void SetGenerateNoiseTexture(int shouldGenerate);
  vtkGetMacro(GenerateNoiseTexture, int);
  //@}

  //@{
  /**
   * Select the statistical distribution of randomly generated noise values.
   * With uniform noise there is greater control over the range of values
   * in the noise texture. The Default is NOISE_TYPE_GAUSSIAN.
   */
  enum {
    NOISE_TYPE_UNIFORM=0,
    NOISE_TYPE_GAUSSIAN=1,
    NOISE_TYPE_PERLIN=2
  };
  void SetNoiseType(int type);
  vtkGetMacro(NoiseType, int);
  //@}

  //@{
  /**
   * Set/Get the side length in pixels of the noise texture. The texture will
   * be length^2 pixels in area.
   */
  void SetNoiseTextureSize(int length);
  vtkGetMacro(NoiseTextureSize, int);
  //@}

  //@{
  /**
   * Set/Get the side length in pixels of the noise values in the noise texture.
   * Each noise value will be length^2 pixels in area.
   */
  void SetNoiseGrainSize(int val);
  vtkGetMacro(NoiseGrainSize, int);
  //@}

  //@{
  /**
   * Set/Get the minimum and mximum  gray scale values that the generated noise
   * can take on. The generated noise will be in the range of MinNoiseValue to
   * MaxNoiseValue. Values are clamped within 0 to 1. MinNoiseValue must be
   * less than MaxNoiseValue.
   */
  void SetMinNoiseValue(double val);
  void SetMaxNoiseValue(double val);
  vtkGetMacro(MinNoiseValue, double);
  vtkGetMacro(MaxNoiseValue, double);
  //@}

  //@{
  /**
   * Set/Get the number of discrete values a noise pixel may take on. Default
   * 1024.
   */
  void SetNumberOfNoiseLevels(int val);
  vtkGetMacro(NumberOfNoiseLevels, int);
  //@}

  //@{
  /**
   * Control the density of of the noise. A value of 1.0 produces uniform random
   * noise while values < 1.0 produce impulse noise with the given probabilty.
   */
  void SetImpulseNoiseProbability(double val);
  vtkGetMacro(ImpulseNoiseProbability, double);
  //@}

  //@{
  /**
   * The color to use for untouched pixels when impulse noise probability < 1.
   */
  void SetImpulseNoiseBackgroundValue(double val);
  vtkGetMacro(ImpulseNoiseBackgroundValue, double);
  //@}

  //@{
  /**
   * Set/Get the seed value used by the random number generator.
   */
  void SetNoiseGeneratorSeed(int val);
  vtkGetMacro(NoiseGeneratorSeed, int);
  //@}

  //@{
  /**
   * Control the screen space decomposition where LIC is computed.
   */
  enum {
    COMPOSITE_INPLACE=0,
    COMPOSITE_INPLACE_DISJOINT=1,
    COMPOSITE_BALANCED=2,
    COMPOSITE_AUTO=3
  };
  void SetCompositeStrategy(int val);
  vtkGetMacro(CompositeStrategy, int);
  //@}

  /**
   * Returns true if the rendering context supports extensions needed by this
   * painter.
   */
  static bool IsSupported(vtkRenderWindow *context);

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * vtkSurfaceLICPainterTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog.
   */
  virtual void WriteTimerLog(const char *){}

protected:
  vtkSurfaceLICPainter();
  ~vtkSurfaceLICPainter();

  /**
   * Called before RenderInternal() if the Information has been changed
   * since the last time this method was called. We use this to detect
   * when LUT has changed.
   */
  virtual void ProcessInformation(vtkInformation* info);

  /**
   * Get the min/max across all ranks. min/max are in/out.
   * In serial operation this is a no-op, in parallel it
   * is a global collective reduction.
   */
  virtual void GetGlobalMinMax(vtkPainterCommunicator*, float&, float&){}

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * vtkSurfaceLICPainterTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog.
   */
  virtual void StartTimerEvent(const char *){}
  virtual void EndTimerEvent(const char *){}

  /**
   * Creates a new communicator with/without the calling processes
   * as indicated by the passed in flag, if not 0 the calling process
   * is included in the new communicator. In parallel this call is mpi
   * collective on the world communicator. In serial this is a no-op.
   */
  virtual vtkPainterCommunicator *CreateCommunicator(int);

  /**
   * Creates a new communicator for internal use based on this
   * rank's visible data.
   */
  void CreateCommunicator();

  /**
   * Computes data bounds.
   */
  void GetBounds(vtkDataObject* data, double bounds[6]);

  /**
   * Take part in garbage collection.
   */
  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

  /**
   * Updates the noise texture, downsampling by the requested sample rate.
   */
  void UpdateNoiseImage(vtkRenderWindow *renWin);

  /**
   * Performs the actual rendering. Subclasses may override this method.
   * default implementation merely call a Render on the DelegatePainter,
   * if any. When RenderInternal() is called, it is assured that the
   * DelegatePainter is in sync with this painter i.e. UpdateDelegatePainter()
   * has been called.
   */
  virtual void RenderInternal(
        vtkRenderer* renderer,
        vtkActor* actor,
        unsigned long typeflags,
        bool forceCompileOnly);


  /**
   * Look for changes that would trigger stage updates
   */
  void ValidateContext(vtkRenderer *renderer);

  //@{
  /**
   * Return false if stage can be skipped
   */
  bool NeedToUpdateOutputData();
  virtual bool NeedToUpdateCommunicator();
  bool NeedToRenderGeometry(vtkRenderer *renderer, vtkActor *actor);
  bool NeedToGatherVectors();
  bool NeedToComputeLIC();
  bool NeedToColorLIC();
  void SetUpdateAll();
  //@}

  //@{
  /**
   * resoucre allocators
   */
  bool PrepareOutput();
  void InitializeResources();
  //@}

  //@{
  /**
   * set tcoords with vectors
   */
  bool VectorsToTCoords(vtkDataObject *dataObj);
  bool VectorsToTCoords(vtkDataSet *dataObj);
  void ClearTCoords(vtkDataSet *data);
  //@}

  /**
   * Returns true when rendering LIC is possible.
   */
  bool CanRenderSurfaceLIC(vtkActor *actor, int typeflags);

protected:
  // Unit is a pixel length.
  int     NumberOfSteps;
  double  StepSize;
  int     NormalizeVectors;

  int     EnhancedLIC;
  int     EnhanceContrast;
  double  LowLICContrastEnhancementFactor;
  double  HighLICContrastEnhancementFactor;
  double  LowColorContrastEnhancementFactor;
  double  HighColorContrastEnhancementFactor;
  int     AntiAlias;

  int     MaskOnSurface;
  double  MaskThreshold;
  double  MaskIntensity;
  double  MaskColor[3];

  int     ColorMode;
  double  LICIntensity;
  double  MapModeBias;

  int     GenerateNoiseTexture;
  int     NoiseType;
  int     NoiseTextureSize;
  int     NoiseGrainSize;
  double  MinNoiseValue;
  double  MaxNoiseValue;
  int     NumberOfNoiseLevels;
  double  ImpulseNoiseProbability;
  double  ImpulseNoiseBackgroundValue;
  int     NoiseGeneratorSeed;

  int     AlwaysUpdate;
  int     Enable;
  int     CompositeStrategy;

  vtkDataObject* Output;
  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkSurfaceLICPainter(const vtkSurfaceLICPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSurfaceLICPainter&) VTK_DELETE_FUNCTION;
};

#endif
