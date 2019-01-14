/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToAMR.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageToAMR
 * @brief   filter to convert any vtkImageData to a
 * vtkOverlappingAMR.
 *
 * vtkImageToAMR is a simple filter that converts any vtkImageData to a
 * vtkOverlappingAMR dataset. The input vtkImageData is treated as the highest
 * refinement available for the highest level. The lower refinements and the
 * number of blocks is controlled properties specified on the filter.
*/

#ifndef vtkImageToAMR_h
#define vtkImageToAMR_h

#include "vtkOverlappingAMRAlgorithm.h"
#include "vtkFiltersAMRModule.h" // For export macro

class VTKFILTERSAMR_EXPORT vtkImageToAMR : public vtkOverlappingAMRAlgorithm
{
public:
  static vtkImageToAMR* New();
  vtkTypeMacro(vtkImageToAMR, vtkOverlappingAMRAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the maximum number of levels in the generated Overlapping-AMR.
   */
  vtkSetClampMacro(NumberOfLevels, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfLevels, int);
  //@}

  //@{
  /**
   * Set the refinement ratio for levels. This refinement ratio is used for all
   * levels.
   */
  vtkSetClampMacro(RefinementRatio, int, 2, VTK_INT_MAX);
  vtkGetMacro(RefinementRatio, int);
  //@}

  //@{
  /**
   * Set the maximum number of blocks in the output
   */
  vtkSetClampMacro(MaximumNumberOfBlocks, int, 1, VTK_INT_MAX);
  vtkGetMacro(MaximumNumberOfBlocks, int);
  //@}

protected:
  vtkImageToAMR();
  ~vtkImageToAMR() override;

  /**
   * Fill the input port information objects for this algorithm.  This
   * is invoked by the first call to GetInputPortInformation for each
   * port so subclasses can specify what they can handle.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector) override;

  int NumberOfLevels;
  int MaximumNumberOfBlocks;
  int RefinementRatio;


private:
  vtkImageToAMR(const vtkImageToAMR&) = delete;
  void operator=(const vtkImageToAMR&) = delete;

};

#endif
