/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHausdorffDistancePointSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Copyright (c) 2011 LTSI INSERM U642
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
//     * Neither name of LTSI, INSERM nor the names
// of any contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/** @class vtkHausdorffDistancePointSetFilter
 *  @brief Compute Hausdorff distance between two point sets
 *
 * This class computes the relative and hausdorff distances from two point
 * sets (input port 0 and input port 1). If no topology is specified (ie.
 * vtkPointSet or vtkPolyData without vtkPolys), the distances are
 * computed between point location. If polys exist (ie triangulation),
 * the TargetDistanceMethod allows for an interpolation of the cells to
 * ensure a better minimal distance exploration.
 *
 * The outputs (port 0 and 1) have the same geometry and topology as its
 * respective input port. Two FieldData arrays are added : HausdorffDistance
 * and RelativeDistance. The former is equal on both outputs whereas the
 * latter may differ. A PointData containing the specific point minimal
 * distance is also added to both outputs.
 *
 * @author Frederic Commandeur
 * @author Jerome Velut
 * @author LTSI
 *
 * @see https://www.vtkjournal.org/browse/publication/839
 */

#ifndef vtkHausdorffDistancePointSetFilter_h
#define vtkHausdorffDistancePointSetFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class VTKFILTERSMODELING_EXPORT vtkHausdorffDistancePointSetFilter : public vtkPointSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type and printing.
   */
  static vtkHausdorffDistancePointSetFilter* New();
  vtkTypeMacro(vtkHausdorffDistancePointSetFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Get the Relative Distance from A to B and B to A.
   */
  vtkGetVector2Macro(RelativeDistance, double);
  //@}

  //@{
  /**
   * Get the Hausdorff Distance.
   */
  vtkGetMacro(HausdorffDistance, double);
  //@}

  enum DistanceMethod
  {
    POINT_TO_POINT,
    POINT_TO_CELL
  };

  //@{
  /**
   * Specify the strategy for computing the distance. If no topology is specified (ie.
   * vtkPointSet or vtkPolyData without vtkPolys), the distances are
   * computed between point location. If polys exist (i.e. triangulation),
   * the TargetDistanceMethod allows for an interpolation of the cells to
   * ensure a better minimal distance exploration.
   *
   */
  vtkSetMacro(TargetDistanceMethod, int);
  vtkGetMacro(TargetDistanceMethod, int);
  void SetTargetDistanceMethodToPointToPoint() { this->SetTargetDistanceMethod(POINT_TO_POINT); }
  void SetTargetDistanceMethodToPointToCell() { this->SetTargetDistanceMethod(POINT_TO_CELL); }
  const char* GetTargetDistanceMethodAsString();
  //@}

protected:
  vtkHausdorffDistancePointSetFilter();
  ~vtkHausdorffDistancePointSetFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int TargetDistanceMethod;   //!< point-to-point if 0, point-to-cell if 1
  double RelativeDistance[2]; //!< relative distance between inputs
  double HausdorffDistance;   //!< hausdorff distance (max(relative distance))

private:
  vtkHausdorffDistancePointSetFilter(const vtkHausdorffDistancePointSetFilter&) = delete;
  void operator=(const vtkHausdorffDistancePointSetFilter&) = delete;
};
inline const char* vtkHausdorffDistancePointSetFilter::GetTargetDistanceMethodAsString()
{
  if (this->TargetDistanceMethod == POINT_TO_POINT)
  {
    return "PointToPoint";
  }
  else
  {
    return "PointToCell";
  }
}
#endif
