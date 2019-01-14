/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCookieCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCookieCutter
 * @brief   cut vtkPolyData defined on the 2D plane with one or more polygons
 *
 * This filter crops an input vtkPolyData consisting of cells (i.e., points,
 * lines, polygons, and triangle strips) with loops specified by a second
 * input containing polygons. Note that this filter can handle concave
 * polygons and/or loops. It may produce multiple output polygons for each
 * polygon/loop interaction. Similarly, it may produce multiple line segments
 * and so on.
 *
 * @warning
 * The z-values of the input vtkPolyData and the points defining the loops are
 * assumed to lie at z=constant. In other words, this filter assumes that the data lies
 * in a plane orthogonal to the z axis.
 *
*/

#ifndef vtkCookieCutter_h
#define vtkCookieCutter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSMODELING_EXPORT vtkCookieCutter : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to instantiate, print and provide type information.
   */
  static vtkCookieCutter *New();
  vtkTypeMacro(vtkCookieCutter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify the a second vtkPolyData input which defines loops used to cut
   * the input polygonal data. These loops must be manifold, i.e., do not
   * self intersect. The loops are defined from the polygons defined in
   * this second input.
   */
  void SetLoopsConnection(vtkAlgorithmOutput* algOutput);
  vtkAlgorithmOutput* GetLoopsConnection();

  //@{
  /**
   * Specify the a second vtkPolyData input which defines loops used to cut
   * the input polygonal data. These loops must be manifold, i.e., do not
   * self intersect. The loops are defined from the polygons defined in
   * this second input.
   */
  void SetLoopsData(vtkDataObject *loops);
  vtkDataObject *GetLoops();
  //@}

protected:
  vtkCookieCutter();
  ~vtkCookieCutter() override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;
  int FillInputPortInformation(int, vtkInformation *) override;

private:
  vtkCookieCutter(const vtkCookieCutter&) = delete;
  void operator=(const vtkCookieCutter&) = delete;
};


#endif
