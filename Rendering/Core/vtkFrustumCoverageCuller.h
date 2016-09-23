/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumCoverageCuller.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFrustumCoverageCuller
 * @brief   cull props based on frustum coverage
 *
 * vtkFrustumCoverageCuller will cull props based on the coverage in
 * the view frustum. The coverage is computed by enclosing the prop in
 * a bounding sphere, projecting that to the viewing coordinate system, then
 * taking a slice through the view frustum at the center of the sphere. This
 * results in a circle on the plane slice through the view frustum. This
 * circle is enclosed in a squared, and the fraction of the plane slice that
 * this square covers is the coverage. This is a number between 0 and 1.
 * If the number is less than the MinumumCoverage, the allocated render time
 * for that prop is set to zero. If it is greater than the MaximumCoverage,
 * the allocated render time is set to 1.0. In between, a linear ramp is used
 * to convert coverage into allocated render time.
 *
 * @sa
 * vtkCuller
*/

#ifndef vtkFrustumCoverageCuller_h
#define vtkFrustumCoverageCuller_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCuller.h"

#define VTK_CULLER_SORT_NONE          0
#define VTK_CULLER_SORT_FRONT_TO_BACK 1
#define VTK_CULLER_SORT_BACK_TO_FRONT 2

class vtkProp;
class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkFrustumCoverageCuller : public vtkCuller
{
public:
  static vtkFrustumCoverageCuller *New();
  vtkTypeMacro(vtkFrustumCoverageCuller,vtkCuller);
  void PrintSelf(ostream& os,vtkIndent indent);

  //@{
  /**
   * Set/Get the minimum coverage - props with less coverage than this
   * are given no time to render (they are culled)
   */
  vtkSetMacro( MinimumCoverage, double );
  vtkGetMacro( MinimumCoverage, double );
  //@}

  //@{
  /**
   * Set/Get the maximum coverage - props with more coverage than this are
   * given an allocated render time of 1.0 (the maximum)
   */
  vtkSetMacro( MaximumCoverage, double );
  vtkGetMacro( MaximumCoverage, double );
  //@}

  //@{
  /**
   * Set the sorting style - none, front-to-back or back-to-front
   * The default is none
   */
  vtkSetClampMacro( SortingStyle, int,
        VTK_CULLER_SORT_NONE, VTK_CULLER_SORT_BACK_TO_FRONT );
  vtkGetMacro(SortingStyle,int);
  void SetSortingStyleToNone()
        {this->SetSortingStyle(VTK_CULLER_SORT_NONE);};
  void SetSortingStyleToBackToFront()
    {this->SetSortingStyle(VTK_CULLER_SORT_BACK_TO_FRONT);};
  void SetSortingStyleToFrontToBack()
    {this->SetSortingStyle(VTK_CULLER_SORT_FRONT_TO_BACK);};
  const char *GetSortingStyleAsString(void);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THESE METHODS OUTSIDE OF THE RENDERING PROCESS
   * Perform the cull operation
   * This method should only be called by vtkRenderer as part of
   * the render process
   */
  double Cull( vtkRenderer *ren, vtkProp **propList,
              int& listLength, int& initialized );

protected:
  vtkFrustumCoverageCuller();
  ~vtkFrustumCoverageCuller() {}

  double       MinimumCoverage;
  double       MaximumCoverage;
  int          SortingStyle;
private:
  vtkFrustumCoverageCuller(const vtkFrustumCoverageCuller&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFrustumCoverageCuller&) VTK_DELETE_FUNCTION;
};


#endif
