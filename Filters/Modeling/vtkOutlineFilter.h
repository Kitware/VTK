/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOutlineFilter
 * @brief   create wireframe outline for arbitrary data set
 *
 * vtkOutlineFilter is a filter that generates a wireframe outline of any
 * data set. The outline consists of the twelve edges of the dataset
 * bounding box.
*/

#ifndef vtkOutlineFilter_h
#define vtkOutlineFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkOutlineSource;

class VTKFILTERSMODELING_EXPORT vtkOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkOutlineFilter *New();
  vtkTypeMacro(vtkOutlineFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Generate solid faces for the box. This is off by default.
   */
  vtkSetMacro(GenerateFaces, vtkTypeBool);
  vtkBooleanMacro(GenerateFaces, vtkTypeBool);
  vtkGetMacro(GenerateFaces, vtkTypeBool);
  //@}

protected:
  vtkOutlineFilter();
  ~vtkOutlineFilter() override;

  vtkTypeBool GenerateFaces;
  vtkOutlineSource *OutlineSource;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkOutlineFilter(const vtkOutlineFilter&) = delete;
  void operator=(const vtkOutlineFilter&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkOutlineFilter.h
