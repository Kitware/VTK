// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageTransform
 * @brief   helper class to transform output of non-axis-aligned images
 *
 * vtkImageTransform is a helper class to transform the output of
 * image filters (i.e., filter that input vtkImageData) by applying the
 * Index to Physical transformation from the input image, which can
 * include origin, spacing, direction. The transformation process is
 * threaded with vtkSMPTools for performance.
 *
 * Typically in application the single method TransformPointSet() is
 * invoked to transform the output of an image algorithm (assuming
 * that the image's direction/orientation matrix is non-identity).
 * Note that vtkPointSets encompass vtkPolyData as well
 * as vtkUnstructuredGrids. In the future other output types may be
 * added. Note that specific methods for transforming points, normals,
 * and vectors is also provided by this class in case additional
 * output data arrays need to be transformed (since
 * TransformPointSet() only processes data arrays labeled as points,
 * normals, and vectors).
 *
 * @warning
 * This class assumes that any vectors are gradients, and vector arrays
 * will therefore be transformed by first dividing by the spacing and
 * then applying the inverse transpose of the direction matrix.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 */

#ifndef vtkImageTransform_h
#define vtkImageTransform_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkImageData;
class vtkMatrix3x3;
class vtkMatrix4x4;
class vtkPointSet;

class VTKCOMMONDATAMODEL_EXPORT vtkImageTransform : public vtkObject
{
public:
  ///@{
  /**
   * Standard methods for construction, type information, printing.
   */
  static vtkImageTransform* New();
  vtkTypeMacro(vtkImageTransform, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Given a vtkImageData (and hence its associated orientation
   * matrix), and an instance of vtkPointSet, transform its points, as
   * well as any normals and vectors, associated with the
   * vtkPointSet. This is a convenience function, internally it calls
   * TranslatePoints(), TransformPoints(), TransformNormals(), and/or
   * TransformVectors() as appropriate. Note that both the normals and
   * vectors associated with the point and cell data are transformed
   * unless the second signature is called, which controls whether to
   * transform normals and/or vectors.  WARNING: unlike most VTK
   * transforms, this method assumes vectors are covariant, for example,
   * gradient vectors.  It will give incorrect results for vectors such
   * as velocity or displacement unless the spacing is (1,1,1) and the
   * direction matrix is orthonormal.
   */
  static void TransformPointSet(vtkImageData* im, vtkPointSet* ps);
  static void TransformPointSet(
    vtkImageData* im, vtkPointSet* ps, bool transNormals, bool transVectors);
  ///@}

  /**
   * Given x-y-z points represented by a vtkDataArray,
   * translate the points using the image origin. This
   * method is useful if there is no orientation or
   * spacing to apply.
   */
  static void TranslatePoints(const double t[3], vtkDataArray* da);

  /**
   * Given x-y-z points represented by a vtkDataArray,
   * transform the points using the matrix provided.
   */
  static void TransformPoints(vtkMatrix4x4* m4, vtkDataArray* da);

  /**
   * Given three-component normals represented by a vtkDataArray,
   * transform the normals using the matrix provided.
   */
  static void TransformNormals(vtkMatrix3x3* m3, const double spacing[3], vtkDataArray* da);

  /**
   * Given three-component vectors represented by a vtkDataArray, transform
   * the vectors using the matrix provided.  WARNING: unlike most VTK
   * transforms, this method assumes vectors are covariant, for example,
   * gradient vectors.  It will give incorrect results for vectors such
   * as velocity or displacement unless the spacing is (1,1,1) and the
   * direction matrix is orthonormal.
   */
  static void TransformVectors(vtkMatrix3x3* m3, const double spacing[3], vtkDataArray* da);

protected:
  vtkImageTransform() = default;
  ~vtkImageTransform() override = default;

private:
  vtkImageTransform(const vtkImageTransform&) = delete;
  void operator=(const vtkImageTransform&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
