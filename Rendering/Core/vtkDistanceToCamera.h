/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceToCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkDistanceToCamera
 * @brief   calculates distance from points to the camera.
 *
 *
 * This filter adds a double array containing the distance from each point
 * to the camera. If Scaling is on, it will use the values in the input
 * array to process in order to scale the size of the points. ScreenSize
 * sets the size in screen pixels that you would want a rendered rectangle
 * at that point to be, if it was scaled by the output array.
*/

#ifndef vtkDistanceToCamera_h
#define vtkDistanceToCamera_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkDistanceToCamera : public vtkPolyDataAlgorithm
{
public:
  static vtkDistanceToCamera *New();
  vtkTypeMacro(vtkDistanceToCamera,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The renderer which will ultimately render these points.
   */
  void SetRenderer(vtkRenderer* ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  //@}

  //@{
  /**
   * The desired screen size obtained by scaling glyphs by the distance
   * array. It assumes the glyph at each point will be unit size.
   */
  vtkSetMacro(ScreenSize, double);
  vtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * Whether to scale the distance by the input array to process.
   */
  vtkSetMacro(Scaling, bool);
  vtkGetMacro(Scaling, bool);
  vtkBooleanMacro(Scaling, bool);
  //@}

  /**
   * The modified time of this filter.
   */
  virtual vtkMTimeType GetMTime();

protected:
  vtkDistanceToCamera();
  ~vtkDistanceToCamera();

  int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  vtkRenderer* Renderer;
  double ScreenSize;
  bool Scaling;
  int LastRendererSize[2];
  double LastCameraPosition[3];
  double LastCameraFocalPoint[3];
  double LastCameraViewUp[3];
  double LastCameraParallelScale;

private:
  vtkDistanceToCamera(const vtkDistanceToCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDistanceToCamera&) VTK_DELETE_FUNCTION;
};

#endif
