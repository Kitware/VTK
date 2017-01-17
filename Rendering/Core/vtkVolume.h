/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolume
 * @brief   represents a volume (data & properties) in a rendered scene
 *
 *
 * vtkVolume is used to represent a volumetric entity in a rendering scene.
 * It inherits functions related to the volume's position, orientation and
 * origin from vtkProp3D. The volume maintains a reference to the
 * volumetric data (i.e., the volume mapper). The volume also contains a
 * reference to a volume property which contains all common volume rendering
 * parameters.
 *
 * @sa
 * vtkAbstractVolumeMapper vtkVolumeProperty vtkProp3D
*/

#ifndef vtkVolume_h
#define vtkVolume_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkRenderer;
class vtkPropCollection;
class vtkVolumeCollection;
class vtkWindow;
class vtkVolumeProperty;
class vtkAbstractVolumeMapper;

class VTKRENDERINGCORE_EXPORT vtkVolume : public vtkProp3D
{
public:
  vtkTypeMacro(vtkVolume, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a Volume with the following defaults: origin(0,0,0)
   * position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
   * orientation=(0,0,0).
   */
  static vtkVolume *New();

  //@{
  /**
   * Set/Get the volume mapper.
   */
  void SetMapper(vtkAbstractVolumeMapper *mapper);
  vtkGetObjectMacro(Mapper, vtkAbstractVolumeMapper);
  //@}

  //@{
  /**
   * Set/Get the volume property.
   */
  void SetProperty(vtkVolumeProperty *property);
  vtkVolumeProperty *GetProperty();
  //@}

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. This method
   * is used in that process.
   */
  void GetVolumes(vtkPropCollection *vc) VTK_OVERRIDE;

  /**
   * Update the volume rendering pipeline by updating the volume mapper
   */
  void Update();

  //@{
  /**
   * Get the bounds - either all six at once
   * (xmin, xmax, ymin, ymax, zmin, zmax) or one at a time.
   */
  double *GetBounds() VTK_OVERRIDE;
  void GetBounds(double bounds[6])
    { this->vtkProp3D::GetBounds(bounds); }
  double GetMinXBound();
  double GetMaxXBound();
  double GetMinYBound();
  double GetMaxYBound();
  double GetMinZBound();
  double GetMaxZBound();
  //@}

  /**
   * Return the MTime also considering the property etc.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, mappers,
   * etc.
   */
  vtkMTimeType GetRedrawMTime() VTK_OVERRIDE;

  /**
   * Shallow copy of this vtkVolume. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp *prop) VTK_OVERRIDE;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Support the standard render methods.
   * Depending on the mapper type, the volume may be rendered using
   * this method (FRAMEBUFFER volume such as texture mapping will
   * be rendered this way)
   */
  int RenderVolumetricGeometry(vtkViewport *viewport) VTK_OVERRIDE;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this volume.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  float *GetCorrectedScalarOpacityArray(int);
  float *GetCorrectedScalarOpacityArray()
    { return this->GetCorrectedScalarOpacityArray(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  float *GetScalarOpacityArray(int);
  float *GetScalarOpacityArray()
    { return this->GetScalarOpacityArray(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  float *GetGradientOpacityArray(int);
  float *GetGradientOpacityArray()
    { return this->GetGradientOpacityArray(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  float *GetGrayArray(int);
  float *GetGrayArray()
    { return this->GetGrayArray(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  float *GetRGBArray(int);
  float *GetRGBArray()
    { return this->GetRGBArray(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  float GetGradientOpacityConstant(int);
  float GetGradientOpacityConstant()
    { return this->GetGradientOpacityConstant(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  float  GetArraySize()
    { return static_cast<float>(this->ArraySize); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  void UpdateTransferFunctions(vtkRenderer *ren);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   */
  void UpdateScalarOpacityforSampleSize(vtkRenderer *ren,
                                        float sample_distance);

  /// Used by vtkHardwareSelector to determine if the prop supports hardware
  /// selection.
  /// @warning INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  /// DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  bool GetSupportsSelection() VTK_OVERRIDE
   { return true; }

protected:
  vtkVolume();
  ~vtkVolume() VTK_OVERRIDE;

  vtkAbstractVolumeMapper *Mapper;
  vtkVolumeProperty *Property;

  // The rgb transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the rgb transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float *RGBArray[VTK_MAX_VRCOMP];
  vtkTimeStamp RGBArrayMTime[VTK_MAX_VRCOMP];

  // The gray transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the gray transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float *GrayArray[VTK_MAX_VRCOMP];
  vtkTimeStamp GrayArrayMTime[VTK_MAX_VRCOMP];

  // The scalar opacity transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the opacity transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float *ScalarOpacityArray[VTK_MAX_VRCOMP];
  vtkTimeStamp ScalarOpacityArrayMTime[VTK_MAX_VRCOMP];

  // The corrected scalar opacity transfer function array - this is identical
  // to the opacity transfer function array when the step size is 1.
  // In other cases, it is corrected to reflect the new material thickness
  // modelled by a step size different than 1.
  float *CorrectedScalarOpacityArray[VTK_MAX_VRCOMP];
  vtkTimeStamp CorrectedScalarOpacityArrayMTime[VTK_MAX_VRCOMP];

  // CorrectedStepSize is the step size currently modelled by
  // CorrectedArray.  It is used to determine when the
  // CorrectedArray needs to be updated to match SampleDistance
  // in the volume mapper.
  float CorrectedStepSize;

  // Number of elements in the rgb, gray, and opacity transfer function arrays
  int ArraySize;

  // The magnitude of gradient opacity transfer function array
  float GradientOpacityArray[VTK_MAX_VRCOMP][256];
  float GradientOpacityConstant[VTK_MAX_VRCOMP];
  vtkTimeStamp GradientOpacityArrayMTime[VTK_MAX_VRCOMP];

  // Function to compute screen coverage of this volume
  double ComputeScreenCoverage(vtkViewport *vp);

private:
  vtkVolume(const vtkVolume&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolume&) VTK_DELETE_FUNCTION;
};

#endif
