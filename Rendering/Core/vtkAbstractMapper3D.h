// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractMapper3D
 * @brief   abstract class specifies interface to map 3D data
 *
 * vtkAbstractMapper3D is an abstract class to specify interface between 3D
 * data and graphics primitives or software rendering techniques. Subclasses
 * of vtkAbstractMapper3D can be used for rendering geometry or rendering
 * volumetric data.
 *
 * This class also defines an API to support hardware clipping planes (at most
 * six planes can be defined). It also provides geometric data about the input
 * data it maps, such as the bounding box and center.
 *
 * @sa
 * vtkAbstractMapper vtkMapper vtkPolyDataMapper vtkVolumeMapper
 */

#ifndef vtkAbstractMapper3D_h
#define vtkAbstractMapper3D_h

#include "vtkAbstractMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkWindow;
class vtkDataSet;
class vtkMatrix4x4;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkAbstractMapper3D : public vtkAbstractMapper
{
public:
  vtkTypeMacro(vtkAbstractMapper3D, vtkAbstractMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * Update this->Bounds as a side effect.
   */
  virtual double* GetBounds() VTK_SIZEHINT(6) = 0;

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  virtual void GetBounds(double bounds[6]);

  ///@{
  /**
   * Return the Center of this mapper's data.
   */
  double* GetCenter() VTK_SIZEHINT(3);
  void GetCenter(double center[3])
  {
    double* rc = this->GetCenter();
    center[0] = rc[0];
    center[1] = rc[1];
    center[2] = rc[2];
  }
  ///@}

  /**
   * Return the diagonal length of this mappers bounding box.
   */
  double GetLength();

  /**
   * Is this a ray cast mapper? A subclass would return 1 if the
   * ray caster is needed to generate an image from this mapper.
   */
  virtual vtkTypeBool IsARayCastMapper() { return 0; }

  /**
   * Is this a "render into image" mapper? A subclass would return 1 if the
   * mapper produces an image by rendering into a software image buffer.
   */
  virtual vtkTypeBool IsARenderIntoImageMapper() { return 0; }

  /**
   * Get the ith clipping plane as a homogeneous plane equation.
   * Use GetNumberOfClippingPlanes to get the number of planes.
   */
  void GetClippingPlaneInDataCoords(vtkMatrix4x4* propMatrix, int i, double planeEquation[4]);

protected:
  vtkAbstractMapper3D();
  ~vtkAbstractMapper3D() override = default;

  double Bounds[6];
  double Center[3];

private:
  vtkAbstractMapper3D(const vtkAbstractMapper3D&) = delete;
  void operator=(const vtkAbstractMapper3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
