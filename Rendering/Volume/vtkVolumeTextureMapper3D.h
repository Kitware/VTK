/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeTextureMapper3D
 * @brief   volume render with 3D texture mapping
 *
 *
 * vtkVolumeTextureMapper3D renders a volume using 3D texture mapping.
 * This class is actually an abstract superclass - with all the actual
 * work done by vtkOpenGLVolumeTextureMapper3D.
 *
 * This mappers currently supports:
 *
 * - any data type as input
 * - one component, or two or four non-independent components
 * - composite blending
 * - intermixed opaque geometry
 * - multiple volumes can be rendered if they can
 *   be sorted into back-to-front order (use the vtkFrustumCoverageCuller)
 *
 * This mapper does not support:
 * - more than one independent component
 * - maximum intensity projection
 *
 * Internally, this mapper will potentially change the resolution of the
 * input data. The data will be resampled to be a power of two in each
 * direction, and also no greater than 128*256*256 voxels (any aspect)
 * for one or two component data, or 128*128*256 voxels (any aspect)
 * for four component data. The limits are currently hardcoded after
 * a check using the GL_PROXY_TEXTURE3D because some graphics drivers
 * were always responding "yes" to the proxy call despite not being
 * able to allocate that much texture memory.
 *
 * Currently, calculations are computed using 8 bits per RGBA channel.
 * In the future this should be expanded to handle newer boards that
 * can support 15 bit float compositing.
 *
 * This mapper supports two main families of graphics hardware:
 * nvidia and ATI. There are two different implementations of
 * 3D texture mapping used - one based on nvidia's GL_NV_texture_shader2
 * and GL_NV_register_combiners2 extension, and one based on
 * ATI's GL_ATI_fragment_shader (supported also by some nvidia boards)
 * To use this class in an application that will run on various
 * hardware configurations, you should have a back-up volume rendering
 * method. You should create a vtkVolumeTextureMapper3D, assign its
 * input, make sure you have a current OpenGL context (you've rendered
 * at least once), then call IsRenderSupported with a vtkVolumeProperty
 * as an argument. This method will return 0 if the input has more than
 * one independent component, or if the graphics hardware does not
 * support the set of required extensions for using at least one of
 * the two implemented methods (nvidia or ati)
 *
 * @par Thanks:
 * Thanks to Alexandre Gouaillard at the Megason Lab, Department of Systems
 * Biology, Harvard Medical School
 * https://wiki.med.harvard.edu/SysBio/Megason/
 * for the idea and initial patch to speed-up rendering with compressed
 * textures.
 *
 * @sa
 * vtkVolumeMapper
 * @deprecated
*/

#ifndef vtkVolumeTextureMapper3D_h
#define vtkVolumeTextureMapper3D_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeMapper.h"

class vtkImageData;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class vtkVolumeProperty;
#if !defined(VTK_LEGACY_REMOVE)
class VTKRENDERINGVOLUME_EXPORT vtkVolumeTextureMapper3D : public vtkVolumeMapper
{
public:
  vtkTypeMacro(vtkVolumeTextureMapper3D,vtkVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkVolumeTextureMapper3D *New();

  //@{
  /**
   * The distance at which to space sampling planes. This
   * may not be honored for interactive renders. An interactive
   * render is defined as one that has less than 1 second of
   * allocated render time.
   */
  vtkSetMacro( SampleDistance, float );
  vtkGetMacro( SampleDistance, float );
  //@}

  //@{
  /**
   * These are the dimensions of the 3D texture
   */
  vtkGetVectorMacro( VolumeDimensions, int,   3 );
  //@}

  //@{
  /**
   * This is the spacing of the 3D texture
   */
  vtkGetVectorMacro( VolumeSpacing,    float, 3 );
  //@}

  /**
   * Based on hardware and properties, we may or may not be able to
   * render using 3D texture mapping. This indicates if 3D texture
   * mapping is supported by the hardware, and if the other extensions
   * necessary to support the specific properties are available.
   */
  virtual int IsRenderSupported( vtkVolumeProperty *,
                                 vtkRenderer *vtkNotUsed(r))
    {return 0;}

  //@{
  /**
   * Allow access to the number of polygons used for the
   * rendering.
   */
  vtkGetMacro( NumberOfPolygons, int );
  //@}

  //@{
  /**
   * Allow access to the actual sample distance used to render
   * the image.
   */
  vtkGetMacro( ActualSampleDistance, float );
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  void Render(vtkRenderer *, vtkVolume *) VTK_OVERRIDE {}

  /**
   * What rendering method is supported?
   */
  enum
  {
    FRAGMENT_PROGRAM_METHOD=0,
    NVIDIA_METHOD=1,
    ATI_METHOD=2,
    NO_METHOD=3
  };

  /**
   * Returns the number of components of the point scalar field
   */
  int GetNumberOfScalarComponents(vtkImageData *input);

  //@{
  /**
   * Set the preferred render method. If it is supported, this
   * one will be used. Don't allow ATI_METHOD - it is not actually
   * supported.
   */
  vtkSetClampMacro( PreferredRenderMethod, int,
                    vtkVolumeTextureMapper3D::FRAGMENT_PROGRAM_METHOD,
                    vtkVolumeTextureMapper3D::NVIDIA_METHOD );
  void SetPreferredMethodToFragmentProgram()
    { this->SetPreferredRenderMethod( vtkVolumeTextureMapper3D::FRAGMENT_PROGRAM_METHOD ); }
  void SetPreferredMethodToNVidia()
    { this->SetPreferredRenderMethod( vtkVolumeTextureMapper3D::NVIDIA_METHOD ); }
  vtkGetMacro(PreferredRenderMethod, int);
  //@}


  //@{
  /**
   * Set/Get if the mapper use compressed textures (if supported by the
   * hardware). Initial value is false.
   * There are two reasons to use compressed textures: 1. rendering can be 4
   * times faster. 2. It saves some VRAM.
   * There is one reason to not use compressed textures: quality may be lower
   * than with uncompressed textures.
   */
  vtkSetMacro(UseCompressedTexture,bool);
  vtkGetMacro(UseCompressedTexture,bool);
  //@}

protected:
  vtkVolumeTextureMapper3D();
  ~vtkVolumeTextureMapper3D() VTK_OVERRIDE;

  float                    *PolygonBuffer;
  float                    *IntersectionBuffer;
  int                       NumberOfPolygons;
  int                       BufferSize;

  unsigned char            *Volume1;
  unsigned char            *Volume2;
  unsigned char            *Volume3;
  int                       VolumeSize;
  int                       VolumeComponents;
  int                       VolumeDimensions[3];
  float                     VolumeSpacing[3];

  float                     SampleDistance;
  float                     ActualSampleDistance;

  vtkImageData             *SavedTextureInput;
  vtkImageData             *SavedParametersInput;

  vtkColorTransferFunction *SavedRGBFunction;
  vtkPiecewiseFunction     *SavedGrayFunction;
  vtkPiecewiseFunction     *SavedScalarOpacityFunction;
  vtkPiecewiseFunction     *SavedGradientOpacityFunction;
  int                       SavedColorChannels;
  float                     SavedSampleDistance;
  float                     SavedScalarOpacityDistance;

  unsigned char             ColorLookup[65536*4];
  unsigned char             AlphaLookup[65536];
  float                     TempArray1[3*4096];
  float                     TempArray2[4096];
  int                       ColorTableSize;
  float                     ColorTableScale;
  float                     ColorTableOffset;

  unsigned char             DiffuseLookup[65536*4];
  unsigned char             SpecularLookup[65536*4];

  vtkTimeStamp              SavedTextureMTime;
  vtkTimeStamp              SavedParametersMTime;

  int                       RenderMethod;
  int                       PreferredRenderMethod;
  bool                      UseCompressedTexture;

  bool                      SupportsNonPowerOfTwoTextures;

  /**
   * For the given viewing direction, compute the set of polygons.
   */
  void   ComputePolygons( vtkRenderer *ren, vtkVolume *vol, double bounds[6] );

  //@{
  /**
   * Update the internal RGBA representation of the volume. Return 1 if
   * anything change, 0 if nothing changed.
   */
  int    UpdateVolumes( vtkVolume * );
  int    UpdateColorLookup( vtkVolume * );
  //@}

  /**
   * Impemented in subclass - check is texture size is OK.
   */

  virtual int IsTextureSizeSupported(int vtkNotUsed(size)[3],
                                     int vtkNotUsed(components))
  {
      return 0;
  }

private:
  vtkVolumeTextureMapper3D(const vtkVolumeTextureMapper3D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeTextureMapper3D&) VTK_DELETE_FUNCTION;
};

#endif // VTK_LEGACY_REMOVE
#endif






