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

#include "vtkActorNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro
#include "vtkTimeStamp.h"                 //for mapper changed time
#include "vtkWeakPointer.h"               //also for mapper changed time

class vtkActor;
class vtkCompositeDataDisplayAttributes;
class vtkDataArray;
class vtkInformationDoubleKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkMapper;
class vtkPiecewiseFunction;
class vtkPolyData;
class vtkProperty;
class vtkTimeStamp;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayActorNode : public vtkActorNode
{
public:
  static vtkOSPRayActorNode* New();
  vtkTypeMacro(vtkOSPRayActorNode, vtkActorNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to take into account my renderables time, including
   * mapper and data into mapper inclusive of composite input
   */
  vtkMTimeType GetMTime() override;

  /**
   * Scaling modes for the spheres and cylinders that the raytracer
   * renders for points and lines created by VTK.
   */
  enum ScalingMode
  {
    ALL_EXACT = -1,
    ALL_APPROXIMATE,
    EACH_MAPPED,
    EACH_EXACT
  };

  /**
   * A key to set the ScalingMode. The default is ALL_APPROXIMATE.
   * ALL_EXACT means use vtkActor.PointSize/LineWidth for all radii.
   * ALL_APPROXIMATE sets all radii to approximate GL's pixel sizes via a function of
   * PointSize/LineWidth and object bounding box. EACH_MAPPED means map every value from
   * SCALE_ARRAY_NAME through the SCALE_FUNCTION lookup table to set each radius independently.
   * EACH_EXACT means use the SCALE_ARRAY_NAME to set each radius directly.
   */
  static vtkInformationIntegerKey* ENABLE_SCALING();

  //@{
  /**
   * Convenience method to set enable_scaling on my renderable.
   */
  static void SetEnableScaling(int value, vtkActor*);
  static int GetEnableScaling(vtkActor*);
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
   * Convenience method to set a scale_array_name on my renderable.
   */
  static void SetScaleArrayName(const char* scaleArrayName, vtkActor*);

  /**
   * A piecewise function for values from the scale array that alters the resulting
   * radii arbitrarily
   */
  static vtkInformationObjectBaseKey* SCALE_FUNCTION();

  /**
   * Convenience method to set a scale_function on my renderable.
   */
  static void SetScaleFunction(vtkPiecewiseFunction* scaleFunction, vtkActor*);

  /**
   * Indicates that the actor acts as a light emitting object.
   */
  static vtkInformationDoubleKey* LUMINOSITY();

  //@{
  /**
   * Convenience method to set luminosity on my renderable.
   */
  static void SetLuminosity(double value, vtkProperty*);
  static double GetLuminosity(vtkProperty*);
  //@}

protected:
  vtkOSPRayActorNode();
  ~vtkOSPRayActorNode() override;

private:
  vtkOSPRayActorNode(const vtkOSPRayActorNode&) = delete;
  void operator=(const vtkOSPRayActorNode&) = delete;

  vtkWeakPointer<vtkMapper> LastMapper;
  vtkTimeStamp MapperChangedTime;
};
#endif
