/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToTextureFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkScalarsToTextureFilter
 * @brief   generate texture coordinates and a texture image based on a scalar field
 *
 * This filter computes texture coordinates and a 2D texture image based on a polydata,
 * a color transfer function and an array.
 * The output port 0 will contain the input polydata with computed texture coordinates.
 * The output port 1 will contain the texture.
 * The computed texture coordinates is based on vtkTextureMapToPlane which computes them using
 * 3D positions projected on the best fitting plane.
 * @sa vtkTextureMapToPlane vtkResampleToImage
 */

#ifndef vtkScalarsToTextureFilter_h
#define vtkScalarsToTextureFilter_h

#include "vtkFiltersTextureModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // For smart pointer

class vtkImageData;
class vtkPolyData;
class vtkScalarsToColors;

class VTKFILTERSTEXTURE_EXPORT vtkScalarsToTextureFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkScalarsToTextureFilter* New();
  vtkTypeMacro(vtkScalarsToTextureFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get a color transfer function.
   * This transfer function will be used to determine the pixel colors of the texture.
   * If not specified, the filter use a default one (blue/white/red) based on the range of the
   * input array.
   */
  void SetTransferFunction(vtkScalarsToColors* stc);
  vtkScalarsToColors* GetTransferFunction();
  //@}

  //@{
  /**
   * Specify if a new point array containing RGBA values have to be computed by the specified
   * color transfer function.
   */
  vtkGetMacro(UseTransferFunction, bool);
  vtkSetMacro(UseTransferFunction, bool);
  vtkBooleanMacro(UseTransferFunction, bool);
  //@}

  //@{
  /**
   * Get/Set the width and height of the generated texture.
   * Default is 128x128. The width and height must be greater than 1.
   */
  vtkSetVector2Macro(TextureDimensions, int);
  vtkGetVector2Macro(TextureDimensions, int);
  //@}

protected:
  vtkScalarsToTextureFilter();
  ~vtkScalarsToTextureFilter() override = default;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  void operator=(const vtkScalarsToTextureFilter&) = delete;
  vtkScalarsToTextureFilter(const vtkScalarsToTextureFilter&) = delete;

  vtkSmartPointer<vtkScalarsToColors> TransferFunction;
  int TextureDimensions[2];
  bool UseTransferFunction = true;
};
#endif
