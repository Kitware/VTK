// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVolumePicker
 * @brief   ray-cast picker enhanced for volumes
 *
 * vtkVolumePicker is a subclass of vtkCellPicker.  It has one
 * advantage over vtkCellPicker for volumes: it will be able to
 * correctly perform picking when CroppingPlanes are present.  This
 * isn't possible for vtkCellPicker since it doesn't link to
 * the VolumeRendering classes and hence cannot access information
 * about the CroppingPlanes.
 *
 * @sa
 * vtkPicker vtkPointPicker vtkCellPicker
 *
 * @par Thanks:
 * This class was contributed to VTK by David Gobbi on behalf of Atamai Inc.
 */

#ifndef vtkVolumePicker_h
#define vtkVolumePicker_h

#include "vtkCellPicker.h"
#include "vtkRenderingVolumeModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGVOLUME_EXPORT vtkVolumePicker : public vtkCellPicker
{
public:
  static vtkVolumePicker* New();
  vtkTypeMacro(vtkVolumePicker, vtkCellPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set whether to pick the cropping planes of props that have them.
   * If this is set, then the pick will be done on the cropping planes
   * rather than on the data. The GetCroppingPlaneId() method will return
   * the index of the cropping plane of the volume that was picked.  This
   * setting is only relevant to the picking of volumes.
   */
  vtkSetMacro(PickCroppingPlanes, vtkTypeBool);
  vtkBooleanMacro(PickCroppingPlanes, vtkTypeBool);
  vtkGetMacro(PickCroppingPlanes, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get the index of the cropping plane that the pick ray passed
   * through on its way to the prop. This will be set regardless
   * of whether PickCroppingPlanes is on.  The crop planes are ordered
   * as follows: xmin, xmax, ymin, ymax, zmin, zmax.  If the volume is
   * not cropped, the value will bet set to -1.
   */
  vtkGetMacro(CroppingPlaneId, int);
  ///@}

protected:
  vtkVolumePicker();
  ~vtkVolumePicker() override;

  void ResetPickInfo() override;

  double IntersectVolumeWithLine(const double p1[3], const double p2[3], double t1, double t2,
    vtkProp3D* prop, vtkAbstractVolumeMapper* mapper) override;

  static int ClipLineWithCroppingRegion(const double bounds[6], const int extent[6], int flags,
    const double x1[3], const double x2[3], double t1, double t2, int& extentPlaneId,
    int& numSegments, double* t1List, double* t2List, double* s1List, int* planeIdList);

  vtkTypeBool PickCroppingPlanes;
  int CroppingPlaneId;

private:
  vtkVolumePicker(const vtkVolumePicker&) = delete;
  void operator=(const vtkVolumePicker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
