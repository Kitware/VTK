/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformCoordinateSystems.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTransformCoordinateSystems
 * @brief   transform points into different coordinate systems
 *
 * This filter transforms points from one coordinate system to another. The user
 * must specify the coordinate systems in which the input and output are
 * specified. The user must also specify the VTK viewport (i.e., renderer) in
 * which the transformation occurs.
 *
 * @sa
 * vtkCoordinate vtkTransformFilter vtkTransformPolyData vtkPolyDataMapper2D
*/

#ifndef vtkTransformCoordinateSystems_h
#define vtkTransformCoordinateSystems_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"
#include "vtkCoordinate.h" //to get the defines in vtkCoordinate

class VTKRENDERINGCORE_EXPORT vtkTransformCoordinateSystems : public vtkPointSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkTransformCoordinateSystems, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Instantiate this class. By default no transformation is specified and
   * the input and output is identical.
   */
  static vtkTransformCoordinateSystems *New();

  //@{
  /**
   * Set/get the coordinate system in which the input is specified.
   * The current options are World, Viewport, and Display. By default the
   * input coordinate system is World.
   */
  vtkSetMacro(InputCoordinateSystem, int);
  vtkGetMacro(InputCoordinateSystem, int);
  void SetInputCoordinateSystemToDisplay()
    { this->SetInputCoordinateSystem(VTK_DISPLAY); }
  void SetInputCoordinateSystemToViewport()
    { this->SetInputCoordinateSystem(VTK_VIEWPORT); }
  void SetInputCoordinateSystemToWorld()
    { this->SetInputCoordinateSystem(VTK_WORLD); }
  //@}

  //@{
  /**
   * Set/get the coordinate system to which to transform the output.
   * The current options are World, Viewport, and Display. By default the
   * output coordinate system is Display.
   */
  vtkSetMacro(OutputCoordinateSystem, int);
  vtkGetMacro(OutputCoordinateSystem, int);
  void SetOutputCoordinateSystemToDisplay()
    { this->SetOutputCoordinateSystem(VTK_DISPLAY); }
  void SetOutputCoordinateSystemToViewport()
    { this->SetOutputCoordinateSystem(VTK_VIEWPORT); }
  void SetOutputCoordinateSystemToWorld()
    { this->SetOutputCoordinateSystem(VTK_WORLD); }
  //@}

  /**
   * Return the MTime also considering the instance of vtkCoordinate.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * In order for a successful coordinate transformation to occur, an
   * instance of vtkViewport (e.g., a VTK renderer) must be specified.
   * NOTE: this is a raw pointer, not a weak pointer nor a reference counted
   * object, to avoid reference cycle loop between rendering classes and filter
   * classes.
   */
  void SetViewport(vtkViewport* viewport);
  vtkGetObjectMacro(Viewport, vtkViewport);
  //@}

protected:
  vtkTransformCoordinateSystems();
  ~vtkTransformCoordinateSystems() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int InputCoordinateSystem;
  int OutputCoordinateSystem;
  vtkViewport* Viewport;

  vtkCoordinate* TransformCoordinate;

private:
  vtkTransformCoordinateSystems(const vtkTransformCoordinateSystems&) = delete;
  void operator=(const vtkTransformCoordinateSystems&) = delete;
};

#endif
