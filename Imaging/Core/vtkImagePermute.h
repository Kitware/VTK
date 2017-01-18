/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePermute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImagePermute
 * @brief    Permutes axes of input.
 *
 * vtkImagePermute reorders the axes of the input. Filtered axes specify
 * the input axes which become X, Y, Z.  The input has to have the
 * same scalar type of the output. The filter does copy the
 * data when it executes. This filter is actually a very thin wrapper
 * around vtkImageReslice.
*/

#ifndef vtkImagePermute_h
#define vtkImagePermute_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImageReslice.h"

class VTKIMAGINGCORE_EXPORT vtkImagePermute : public vtkImageReslice
{
public:
  static vtkImagePermute *New();
  vtkTypeMacro(vtkImagePermute,vtkImageReslice);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The filtered axes are the input axes that get relabeled to X,Y,Z.
   */
  void SetFilteredAxes(int x, int y, int z);
  void SetFilteredAxes(const int xyz[3]) {
    this->SetFilteredAxes(xyz[0], xyz[1], xyz[2]); };
  vtkGetVector3Macro(FilteredAxes, int);
  //@}

protected:
  vtkImagePermute();
  ~vtkImagePermute()VTK_OVERRIDE {}

  int FilteredAxes[3];

private:
  vtkImagePermute(const vtkImagePermute&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImagePermute&) VTK_DELETE_FUNCTION;
};

#endif



