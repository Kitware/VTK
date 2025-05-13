// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGridAxesHelper
 * @brief   is a helper object used by vtkGridAxesActor2D,
 * vtkGridAxesActor3D, and vtkGridAxesPlane2DActor.
 *
 * vtkGridAxesActor2D, vtkGridAxesActor3D, and vtkGridAxesPlane2DActor shares a lot
 * of the computations and logic. This class makes it possible to share all
 * such information between these classes.
 * This class works with a single face of the bounding box specified using the
 * GridBounds.
 */

#ifndef vtkGridAxesHelper_h
#define vtkGridAxesHelper_h

#include "vtkObject.h"

#include "vtkRenderingGridAxesModule.h" //needed for exports
#include "vtkVector.h"                  // needed for vtkVector.

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix4x4;
class vtkViewport;

class VTKRENDERINGGRIDAXES_EXPORT vtkGridAxesHelper : public vtkObject
{
public:
  static vtkGridAxesHelper* New();
  vtkTypeMacro(vtkGridAxesHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the bounding box defining the grid space. This, together with the
   * \c Face identify which planar surface this class is interested in. This
   * class is designed to work with a single planar surface.
   *
   * Defaults to [-1.0, 1.0, -1.0, 1.0, -1.0, 1.0]
   */
  vtkSetVector6Macro(GridBounds, double);
  vtkGetVector6Macro(GridBounds, double);
  ///@}

  // These are deliberately in the same order as the faces of a vtkVoxel.
  enum Faces : unsigned char
  {
    MIN_YZ = 0x01,
    MIN_ZX = 0x02,
    MIN_XY = 0x04,
    MAX_YZ = 0x08,
    MAX_ZX = 0x010,
    MAX_XY = 0x020
  };

  ///@{
  /**
   * Indicate which face of the specified bounds is this class operating with.
   *
   * By default, Face is vtkGridAxesHelper::MIN_YZ.
   */
  vtkSetClampMacro(Face, int, MIN_YZ, MAX_XY);
  vtkGetMacro(Face, int);
  ///@}

  /**
   * Valid values for LabelMask.
   */
  enum LabelMasks : unsigned char
  {
    MIN_X = 0x01,
    MIN_Y = 0x02,
    MIN_Z = 0x04,
    MAX_X = 0x08,
    MAX_Y = 0x010,
    MAX_Z = 0x020
  };

  ///@{
  /**
   * Set the axes to label.
   *
   * By default, all axes are labeled i.e. LabelMask = 0xff.
   */
  vtkSetMacro(LabelMask, unsigned int);
  vtkGetMacro(LabelMask, unsigned int);
  ///@}

  /**
   * Get the 4 points in world coordinates that define the grid plane. The
   * points are in anticlockwise anticlockwise order with the face normal
   * pointing outward from the box defined by the GridBounds.
   */
  vtkTuple<vtkVector3d, 4> GetPoints();

  /**
   * Returns which of the 3 coordinate axes for the 2 axes for this plane: 0 for
   * X axis, 1, for Y axis, and 3 for Z axis. The two axes are specified in
   * order so that together with the face normal (which is point outwards from
   * the box defined by GridBounds), they form a right-handed coordinate system.
   */
  vtkVector2i GetActiveAxes();

  /**
   * Returns the visibility for labels for each of the 4 axis defined by the
   * face points based on the LabelMask.
   */
  vtkTuple<bool, 4> GetLabelVisibilities();

  ///@{
  /**
   * Set the transform matrix to use to transform the points. The matrix's MTime
   * will be used to determine if the transformed points needed to be
   * recomputed, when needed.
   */
  void SetMatrix(vtkMatrix4x4*);
  vtkGetObjectMacro(Matrix, vtkMatrix4x4);
  ///@}

  /**
   * Get the 4 points of the plane transformed using the transformation matrix
   * set using SetMatrix(), if any. This method to compute the transformed
   * points the first time its called since the plane points or the
   * transformation matrix was set.
   */
  vtkTuple<vtkVector3d, 4> GetTransformedPoints();

  /**
   * Transforms the give point using the Matrix.
   */
  vtkVector3d TransformPoint(const vtkVector3d& point);

  /**
   * Get the normal to the grid plane face **after** applying the transform
   * specified using transformation matrix.
   * Similar to GetTransformedPoints(), this method will only compute when
   * input parameters have changed since the last time this method was called.
   */
  vtkVector3d GetTransformedFaceNormal();

  /**
   * Call this method before accessing any of the attributes in viewport space.
   * This computes the location of the plane in the viewport space using the
   * specified viewport. This method should typically be called once per render.
   */
  bool UpdateForViewport(vtkViewport* viewport);

  /**
   * Get the positions for the plane points in viewport coordinates.
   */
  vtkTuple<vtkVector2i, 4> GetViewportPoints() const { return this->ViewportPoints; }
  vtkTuple<vtkVector2d, 4> GetViewportPointsAsDouble() const
  {
    return this->ViewportPointsAsDouble;
  }

  /**
   * Get the axis vectors formed using the points returned by
   * GetViewportPoints(). These are in non-normalized form.
   */
  vtkTuple<vtkVector2d, 4> GetViewportVectors() const { return this->ViewportVectors; }

  /**
   * Get the normals to the axis vectors in viewport space. There are not true
   * normals to the axis vector. These are normalized.
   */
  vtkTuple<vtkVector2d, 4> GetViewportNormals() const { return this->ViewportNormals; }

  ///@{
  /**
   * Get if the face is facing backwards in the current viewport.
   */
  vtkGetMacro(Backface, bool);
  ///@}

protected:
  vtkGridAxesHelper();
  ~vtkGridAxesHelper() override;

  /**
   * Get/Set label visibility overrides. This is more of a hack. We needed a
   * mechanism to override which labels are drawn in vtkGridAxesActor3D. This
   * makes that possible.
   */
  void SetLabelVisibilityOverrides(const vtkTuple<bool, 4>& overrides)
  {
    this->LabelVisibilityOverrides = overrides;
  }
  vtkTuple<bool, 4> GetLabelVisibilityOverrides() { return this->LabelVisibilityOverrides; }
  friend class vtkGridAxesActor3D;

private:
  vtkGridAxesHelper(const vtkGridAxesHelper&) = delete;
  void operator=(const vtkGridAxesHelper&) = delete;

  double GridBounds[6];
  int Face;
  unsigned int LabelMask;
  vtkMatrix4x4* Matrix;

  vtkTuple<vtkVector3d, 4> Points;
  vtkVector2i ActiveAxes;
  vtkTuple<bool, 4> LabelVisibilities;
  vtkTuple<bool, 4> ComputedLabelVisibilities;
  vtkTuple<bool, 4> LabelVisibilityOverrides;

  vtkTuple<vtkVector3d, 4> TransformedPoints;
  vtkVector3d TransformedFaceNormal;

  vtkTuple<vtkVector2i, 4> ViewportPoints;
  vtkTuple<vtkVector2d, 4> ViewportPointsAsDouble;
  vtkTuple<vtkVector2d, 4> ViewportVectors;
  vtkTuple<vtkVector2d, 4> ViewportNormals;
  bool Backface;

  vtkMTimeType GetPointsMTime = 0;
  vtkMTimeType GetTransformedPointsMTime = 0;
};

VTK_ABI_NAMESPACE_END
#endif
