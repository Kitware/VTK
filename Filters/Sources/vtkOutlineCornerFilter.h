/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOutlineCornerFilter
 * @brief   create wireframe outline corners for arbitrary data set
 *
 * vtkOutlineCornerFilter is a filter that generates wireframe outline corners of any
 * data set. The outline consists of the eight corners of the dataset
 * bounding box.
*/

#ifndef vtkOutlineCornerFilter_h
#define vtkOutlineCornerFilter_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
class vtkOutlineCornerSource;

class VTKFILTERSSOURCES_EXPORT vtkOutlineCornerFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkOutlineCornerFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct outline corner filter with default corner factor = 0.2
   */
  static vtkOutlineCornerFilter *New();

  //@{
  /**
   * Set/Get the factor that controls the relative size of the corners
   * to the length of the corresponding bounds
   */
  vtkSetClampMacro(CornerFactor, double, 0.001, 0.5);
  vtkGetMacro(CornerFactor, double);
  //@}

protected:
  vtkOutlineCornerFilter();
  ~vtkOutlineCornerFilter() VTK_OVERRIDE;

  vtkOutlineCornerSource *OutlineCornerSource;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  double CornerFactor;
private:
  vtkOutlineCornerFilter(const vtkOutlineCornerFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOutlineCornerFilter&) VTK_DELETE_FUNCTION;
};

#endif
