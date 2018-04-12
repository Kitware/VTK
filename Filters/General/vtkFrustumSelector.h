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
 * @brief   Computes the portion of a dataset which lies within a selection frustum
 *
 */

#ifndef vtkFrustumSelector_h
#define vtkFrustumSelector_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkObject.h"

#include "vtkSmartPointer.h" // for smart pointer

class vtkDataSet;
class vtkPlanes;
class vtkSignedCharArray;

class VTKFILTERSGENERAL_EXPORT vtkFrustumSelector : public vtkObject
{
public:
  static vtkFrustumSelector *New();
  vtkTypeMacro(vtkFrustumSelector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  void ComputePointsInside(vtkDataSet* input, vtkSignedCharArray* pointsInside);
  void ComputeCellsInside(vtkDataSet* input, vtkSignedCharArray* cellsInside);

  int OverallBoundsTest(double bounds[6]);

protected:
  vtkFrustumSelector(vtkPlanes *f=nullptr);
  ~vtkFrustumSelector() override;

  vtkSmartPointer<vtkPlanes> Frustum;

private:
  vtkFrustumSelector(const vtkFrustumSelector&) = delete;
  void operator=(const vtkFrustumSelector&) = delete;
};

#endif
