/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOutlineCornerSource
 * @brief   create wireframe outline corners around bounding box
 *
 * vtkOutlineCornerSource creates wireframe outline corners around a user-specified
 * bounding box.
*/

#ifndef vtkOutlineCornerSource_h
#define vtkOutlineCornerSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkOutlineSource.h"

class VTKFILTERSSOURCES_EXPORT vtkOutlineCornerSource : public vtkOutlineSource
{
public:
  vtkTypeMacro(vtkOutlineCornerSource,vtkOutlineSource);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct outline corner source with default corner factor = 0.2
   */
  static vtkOutlineCornerSource *New();

  //@{
  /**
   * Set/Get the factor that controls the relative size of the corners
   * to the length of the corresponding bounds
   */
  vtkSetClampMacro(CornerFactor, double, 0.001, 0.5);
  vtkGetMacro(CornerFactor, double);
  //@}

protected:
  vtkOutlineCornerSource();
  ~vtkOutlineCornerSource() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  double CornerFactor;
private:
  vtkOutlineCornerSource(const vtkOutlineCornerSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOutlineCornerSource&) VTK_DELETE_FUNCTION;
};

#endif
