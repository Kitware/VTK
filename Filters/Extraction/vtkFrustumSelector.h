/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFrustumSelector.h
 *
 * vtkFrustumSelector is a vtkSelector that selects elements based
 * on whether they are inside or intersect a frustum of interest.  This handles
 * the vtkSelectionNode::FRUSTUM selection type.
 *
 */

#ifndef vtkFrustumSelector_h
#define vtkFrustumSelector_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkSelector.h"

#include "vtkSmartPointer.h" // for smart pointer

class vtkDataSet;
class vtkPlanes;
class vtkSignedCharArray;

class VTKFILTERSEXTRACTION_EXPORT vtkFrustumSelector : public vtkSelector
{
public:
  static vtkFrustumSelector *New();
  vtkTypeMacro(vtkFrustumSelector, vtkSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkSelectionNode* node, const std::string& insidednessArrayName) override;

  /**
   * Return the MTime taking into account changes to the Frustum
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Set the selection frustum. The planes object must contain six planes.
   */
  void SetFrustum(vtkPlanes*);
  vtkPlanes* GetFrustum();
  //@}

protected:
  vtkFrustumSelector(vtkPlanes *f=nullptr);
  ~vtkFrustumSelector() override;

  vtkSmartPointer<vtkPlanes> Frustum;

  bool ComputeSelectedElementsForBlock(vtkDataObject* input,
    vtkSignedCharArray* insidednessArray, unsigned int compositeIndex,
    unsigned int amrLevel, unsigned int amrIndex) override;

  /**
   * Given eight vertices, creates a frustum.
   * each pt is x,y,z,1
   * in the following order
   * near lower left, far lower left
   * near upper left, far upper left
   * near lower right, far lower right
   * near upper right, far upper right
   */
  void CreateFrustum(double vertices[32]);

  /**
   * Computes which points in the dataset are inside the frustum and populates the pointsInside
   * array with 1 for inside and 0 for outside.
   */
  void ComputeSelectedPoints(vtkDataSet* input, vtkSignedCharArray* pointsInside);
  /**
   * Computes which cells in the dataset are inside or intersect the frustum and populates
   * the cellsInside array with 1 for inside/intersecting and 0 for outside.
   */
  void ComputeSelectedCells(vtkDataSet* input, vtkSignedCharArray* cellsInside);

  int OverallBoundsTest(double bounds[6]);

private:
  vtkFrustumSelector(const vtkFrustumSelector&) = delete;
  void operator=(const vtkFrustumSelector&) = delete;
};

#endif
