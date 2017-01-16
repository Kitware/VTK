/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageToPoints
 * @brief   Extract all image voxels as points.
 *
 * This filter takes an input image and an optional stencil, and creates
 * a vtkPolyData that contains the points and the point attributes but no
 * cells.  If a stencil is provided, only the points inside the stencil
 * are included.
 * @par Thanks:
 * Thanks to David Gobbi, Calgary Image Processing and Analysis Centre,
 * University of Calgary, for providing this class.
*/

#ifndef vtkImageToPoints_h
#define vtkImageToPoints_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkImageStencilData;

class VTKIMAGINGHYBRID_EXPORT vtkImageToPoints :
  public vtkPolyDataAlgorithm
{
public:
  static vtkImageToPoints *New();
  vtkTypeMacro(vtkImageToPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Only extract the points that lie within the stencil.
   */
  void SetStencilConnection(vtkAlgorithmOutput *port);
  vtkAlgorithmOutput *GetStencilConnection();
  void SetStencilData(vtkImageStencilData *stencil);
  //@}

  //@{
  /**
   * Set the desired precision for the output points.
   * See vtkAlgorithm::DesiredOutputPrecision for the available choices.
   * The default is double precision.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  vtkImageToPoints();
  ~vtkImageToPoints() VTK_OVERRIDE;

  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inInfo,
                                 vtkInformationVector *outInfo) VTK_OVERRIDE;

  int RequestUpdateExtent(vtkInformation *request,
                                 vtkInformationVector **inInfo,
                                 vtkInformationVector *outInfo) VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
                          vtkInformationVector **inInfo,
                          vtkInformationVector *outInfo) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  int OutputPointsPrecision;

private:
  vtkImageToPoints(const vtkImageToPoints&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageToPoints&) VTK_DELETE_FUNCTION;
};

#endif
