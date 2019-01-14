/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkMultiVolume
 * @brief Represents a world axis-aligned bounding-box containing a set of
 * volumes in a rendered scene.
 *
 * vtkVolume instances registered in this class can be overlapping. They are
 * intended to be all rendered simultaneously by a vtkGPUVolumeRayCastMapper
 * (inputs should be set directly in the mapper).
 *
 * This class holds the full transformation of a bounding-box containing
 * all of the registered volumes.
 *
 *      + TexToBBox : Texture-to-Data (scaling)
 *      + Matrix : Data-to-World (translation)
 *
 * @note This class is intended to be used only by mappers supporting multiple
 * inputs.
 *
 * @sa vtkVolume vtkAbstractVolumeMapper vtkGPUVolumeRayCastMapper
*/
#ifndef vtkMultiVolume_h
#define vtkMultiVolume_h
#include <array>                      // for std::array
#include <unordered_map>              // For std::unordered_map

#include "vtkMatrix4x4.h"             // For Matrix
#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer
#include "vtkVolume.h"


class vtkAbstractVolumeMapper;
class vtkBoundingBox;
class vtkMatrix4x4;
class vtkRenderer;
class vtkVolumeProperty;
class vtkWindow;
class vtkVolumeProperty;
class vtkAbstractVolumeMapper;

class VTKRENDERINGVOLUME_EXPORT vtkMultiVolume : public vtkVolume
{
public:
  static vtkMultiVolume* New();
  vtkTypeMacro(vtkMultiVolume, vtkVolume);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Add / Remove a vtkVolume instance.
   */
  void SetVolume(vtkVolume* volume, int port = 0);
  vtkVolume* GetVolume(int port = 0);
  void RemoveVolume(int port)
  {
    this->SetVolume(nullptr, port);
  }
  //@}

  //@{
  /**
   * Given that this class represents a bounding-box only there is no property
   * directly associated with it (a cannot be set directly).
   * This instance will return the property of the volume registered in the 0th
   * port (or nullptr if no volume has been set).
   * \sa vtkVolume
   */
  void SetProperty(vtkVolumeProperty* property) override;
  vtkVolumeProperty* GetProperty() override;
  //@}

  /**
   * Computes the bounds of the box containing all of the vtkVolume instances.
   * Returns the bounds (vtkVolume::Bounds) in world coordinates [xmin, xmax,
   * ymin, ymax, zmin, zmax] but also keeps cached the bounds in data coordinates
   * (vtkMultiVolume::DataBounds).
   */
  double* GetBounds() override;

  /**
   * \sa vtkVolume
   */
  vtkMTimeType GetMTime() override;

  /**
   * Checks whether the vtkProp passed is another vtkMultiVolume and tries to
   * copy accordingly. Otherwise it falls back to vtkVolume::ShallowCopy.
   * \sa vtkVolume
   */
  void ShallowCopy(vtkProp *prop) override;

  /**
   * As with other vtkProp3D, Matrix holds the transformation from data
   * coordinates to world coordinates.  Since this class represents an
   * axis-aligned bounding-box, this transformation only contains a translation
   * vector. Each registered vtkVolume contains its own transformation with
   * respect to the world coordinate system.
   * \sa vtkProp3D vtkVolume
   */
  using vtkVolume::GetMatrix;
  vtkMatrix4x4* GetMatrix() override
    { return this->Matrix; }

  /**
   * Returns the transformation from texture coordinates to data cooridinates
   * of the bounding-box. Since this class represents an axis-aligned bounding
   * -boxThis, this transformation only contains a scaling diagonal.
   */
  vtkMatrix4x4* GetTextureMatrix()
    { return this->TexToBBox.GetPointer(); };

  /**
   * Total bounds in data coordinates.
   */
  double* GetDataBounds()
    { return this->DataBounds.data(); };

  vtkMTimeType GetBoundsTime()
    { return this->BoundsComputeTime.GetMTime(); };

  /**
   * Since vtkMultiVolume acts like a proxy volume to compute the bounding box
   * for its internal vtkVolume instances, there are no properties to be set directly
   * in this instance. For that reason, this override ignores the vtkVolumeProperty
   * check.
   */
  int RenderVolumetricGeometry(vtkViewport* vp) override;

protected:
  vtkMultiVolume();
  ~vtkMultiVolume() override;

  /**
   * The transformation matrix of this vtkProp3D is not user-definable,
   * (only the registered vtkVolume instances define the total bounding-box).
   * For that reason this method does nothing.
   * \sa vtkProp3D
   */
  void ComputeMatrix() override {};

  /**
   * Returns the vtkVolume registered in port.
   */
  vtkVolume* FindVolume(int port);

  /**
   * Checks for changes in the registered vtkVolume instances which could
   * required the bounding-box to be recomputed.
   */
  bool VolumesChanged();

  /**
   * For a box defined by bounds in coordinate system X, compute its
   * axis-aligned bounds in coordinate system Y. T defines the transformation
   * from X to Y and bounds ([x_min, x_max, y_min, y_max, z_min, z_max])
   * the box in X.
   */
  std::array<double, 6> ComputeAABounds(double bounds[6],
    vtkMatrix4x4* T) const;


  std::array<double, 6> DataBounds;
  std::unordered_map<int, vtkVolume*> Volumes;
  vtkTimeStamp BoundsComputeTime;
  vtkSmartPointer<vtkMatrix4x4> TexToBBox;

private:
  vtkMultiVolume(const vtkMultiVolume&) = delete;
  void operator=(const vtkMultiVolume&) = delete;
};
#endif
