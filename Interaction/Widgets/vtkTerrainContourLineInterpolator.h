// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTerrainContourLineInterpolator
 * @brief   Contour interpolator for DEM data.
 *
 *
 * vtkTerrainContourLineInterpolator interpolates nodes on height field data.
 * The class is meant to be used in conjunciton with a vtkContourWidget,
 * enabling you to draw paths on terrain data. The class internally uses a
 * vtkProjectedTerrainPath. Users can set kind of interpolation
 * desired between two node points by setting the modes of the this filter.
 * For instance:
 *
 * \code
 * contourRepresentation->SetLineInterpolator(interpolator);
 * interpolator->SetImageData( demDataFile );
 * interpolator->GetProjector()->SetProjectionModeToHug();
 * interpolator->SetHeightOffset(25.0);
 * \endcode
 *
 * You are required to set the ImageData to this class as the height-field
 * image.
 *
 * @sa
 * vtkTerrainDataPointPlacer vtkProjectedTerrainPath
 */

#ifndef vtkTerrainContourLineInterpolator_h
#define vtkTerrainContourLineInterpolator_h

#include "vtkContourLineInterpolator.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkProjectedTerrainPath;

class VTKINTERACTIONWIDGETS_EXPORT vtkTerrainContourLineInterpolator
  : public vtkContourLineInterpolator
{
public:
  /**
   * Instantiate this class.
   */
  static vtkTerrainContourLineInterpolator* New();

  ///@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkTerrainContourLineInterpolator, vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Interpolate to create lines between contour nodes idx1 and idx2.
   * Depending on the projection mode, the interpolated line may either
   * hug the terrain, just connect the two points with a straight line or
   * a non-occluded interpolation.
   * Used internally by vtkContourRepresentation.
   */
  int InterpolateLine(vtkRenderer* ren, vtkContourRepresentation* rep, int idx1, int idx2) override;

  /**
   * The interpolator is given a chance to update the node.
   * Used internally by vtkContourRepresentation
   * Returns 0 if the node (world position) is unchanged.
   */
  int UpdateNode(vtkRenderer*, vtkContourRepresentation*, double* vtkNotUsed(node),
    int vtkNotUsed(idx)) override;

  ///@{
  /**
   * Set the height field data. The height field data is a 2D image. The
   * scalars in the image represent the height field. This must be set.
   */
  virtual void SetImageData(vtkImageData*);
  vtkGetObjectMacro(ImageData, vtkImageData);
  ///@}

  ///@{
  /**
   * Get the vtkProjectedTerrainPath operator used to project the terrain
   * onto the data. This operator has several modes, See the documentation
   * of vtkProjectedTerrainPath. The default mode is to hug the terrain
   * data at 0 height offset.
   */
  vtkGetObjectMacro(Projector, vtkProjectedTerrainPath);
  ///@}

protected:
  vtkTerrainContourLineInterpolator();
  ~vtkTerrainContourLineInterpolator() override;

  vtkImageData* ImageData; // height field data
  vtkProjectedTerrainPath* Projector;

private:
  vtkTerrainContourLineInterpolator(const vtkTerrainContourLineInterpolator&) = delete;
  void operator=(const vtkTerrainContourLineInterpolator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
