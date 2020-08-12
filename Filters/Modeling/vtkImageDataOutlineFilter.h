/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageDataOutlineFilter
 * @brief   create wireframe outline for a possibly oriented vtkImageData
 *
 * vtkImageDataOutlineFilter is a filter that generates a wireframe outline
 * of vtkImageData. It takes into account the orientation / DirectionMatrix
 * of the image, so the output outline may not be axes aligned.  The outline
 * consists of the twelve edges of the vtkImageData. Optionally, the six
 * bounding faces of the vtkImageData can be produced as well.
 *
 * @sa
 * vtkOutlineFilter
 */

#ifndef vtkImageDataOutlineFilter_h
#define vtkImageDataOutlineFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSMODELING_EXPORT vtkImageDataOutlineFilter : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation. type information, and printing.
   */
  static vtkImageDataOutlineFilter* New();
  vtkTypeMacro(vtkImageDataOutlineFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Generate the six boundary faces of the image data. This is off by default.
   */
  vtkSetMacro(GenerateFaces, vtkTypeBool);
  vtkBooleanMacro(GenerateFaces, vtkTypeBool);
  vtkGetMacro(GenerateFaces, vtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  vtkImageDataOutlineFilter();
  ~vtkImageDataOutlineFilter() override;

  vtkTypeBool GenerateFaces;
  int OutputPointsPrecision;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkImageDataOutlineFilter(const vtkImageDataOutlineFilter&) = delete;
  void operator=(const vtkImageDataOutlineFilter&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkImageDataOutlineFilter.h
