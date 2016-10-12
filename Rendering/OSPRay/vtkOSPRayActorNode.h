/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayActorNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayActorNode
 * @brief   links vtkActor and vtkMapper to OSPRay
 *
 * Translates vtkActor/Mapper state into OSPRay rendering calls
*/

#ifndef vtkOSPRayActorNode_h
#define vtkOSPRayActorNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkActorNode.h"

class vtkActor;
class vtkCompositeDataDisplayAttributes;
class vtkDataArray;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkPiecewiseFunction;
class vtkPolyData;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayActorNode :
  public vtkActorNode
{
public:
  static vtkOSPRayActorNode* New();
  vtkTypeMacro(vtkOSPRayActorNode, vtkActorNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Overridden to take into account my renderables time, including
   * mapper and data into mapper inclusive of composite input
   */
  virtual vtkMTimeType GetMTime();

  /**
   * When added to the mapper, enables scale array and scale function.
   */
  static vtkInformationIntegerKey* ENABLE_SCALING();

  //@{
  /**
   * Convenience method to set enabled scaling on my renderable.
   */
  static void SetEnableScaling(int value, vtkActor *);
  static int GetEnableScaling(vtkActor *);
  //@}

  /**
   * Name of a point aligned, single component wide, double valued array that,
   * when added to the mapper, will be used to scale each element in the
   * sphere and cylinder representations individually.
   * When not supplied the radius is constant across all elements and
   * is a function of the Mapper's PointSize and LineWidth.
   */
  static vtkInformationStringKey* SCALE_ARRAY_NAME();

  /**
   * Convenience method to set a scale array on my renderable.
   */
  static void SetScaleArrayName(const char *scaleArrayName, vtkActor *);

  /**
   * A piecewise function for values from the scale array that alters the resulting
   * radii arbitrarily
   */
  static vtkInformationObjectBaseKey* SCALE_FUNCTION();

  /**
   * Convenience method to set a scale function on my renderable.
   */
  static void SetScaleFunction(vtkPiecewiseFunction *scaleFunction, vtkActor *);

protected:
  vtkOSPRayActorNode();
  ~vtkOSPRayActorNode();

private:
  vtkOSPRayActorNode(const vtkOSPRayActorNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayActorNode&) VTK_DELETE_FUNCTION;
};
#endif
