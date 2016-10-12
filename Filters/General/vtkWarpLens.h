/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpLens.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWarpLens
 * @brief   deform geometry by applying lens distortion
 *
 * vtkWarpLens is a filter that modifies point coordinates by moving
 * in accord with a lens distortion model.
*/

#ifndef vtkWarpLens_h
#define vtkWarpLens_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkWarpLens : public vtkPointSetAlgorithm
{
public:
  static vtkWarpLens *New();
  vtkTypeMacro(vtkWarpLens,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify second order symmetric radial lens distortion parameter.
   * This is obsoleted by newer instance variables.
   */
  void SetKappa(double kappa);
  double GetKappa();
  //@}

  //@{
  /**
   * Specify the center of radial distortion in pixels.
   * This is obsoleted by newer instance variables.
   */
  void SetCenter(double centerX, double centerY);
  double *GetCenter();
  //@}

  //@{
  /**
   * Specify the calibrated principal point of the camera/lens
   */
  vtkSetVector2Macro(PrincipalPoint,double);
  vtkGetVectorMacro(PrincipalPoint,double,2);
  //@}

  //@{
  /**
   * Specify the symmetric radial distortion parameters for the lens
   */
  vtkSetMacro(K1,double);
  vtkGetMacro(K1,double);
  vtkSetMacro(K2,double);
  vtkGetMacro(K2,double);
  //@}

  //@{
  /**
   * Specify the decentering distortion parameters for the lens
   */
  vtkSetMacro(P1,double);
  vtkGetMacro(P1,double);
  vtkSetMacro(P2,double);
  vtkGetMacro(P2,double);
  //@}

  //@{
  /**
   * Specify the imager format width / height in mm
   */
  vtkSetMacro(FormatWidth,double);
  vtkGetMacro(FormatWidth,double);
  vtkSetMacro(FormatHeight,double);
  vtkGetMacro(FormatHeight,double);
  //@}

  //@{
  /**
   * Specify the image width / height in pixels
   */
  vtkSetMacro(ImageWidth,int);
  vtkGetMacro(ImageWidth,int);
  vtkSetMacro(ImageHeight,int);
  vtkGetMacro(ImageHeight,int);
  //@}

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

protected:
  vtkWarpLens();
  ~vtkWarpLens() VTK_OVERRIDE {}

  int RequestDataObject(vtkInformation *request,
                        vtkInformationVector **inputVector,
                        vtkInformationVector *outputVector) VTK_OVERRIDE;
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  double PrincipalPoint[2];      // The calibrated principal point of camera/lens in mm
  double K1;                     // Symmetric radial distortion parameters
  double K2;
  double P1;                     // Decentering distortion parameters
  double P2;
  double FormatWidth;            // imager format width in mm
  double FormatHeight;           // imager format height in mm
  int ImageWidth;               // image width in pixels
  int ImageHeight;              // image height in pixels
private:
  vtkWarpLens(const vtkWarpLens&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWarpLens&) VTK_DELETE_FUNCTION;
};

#endif
