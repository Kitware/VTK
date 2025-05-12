// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariActorNode
 * @brief   links vtkActor and vtkMapper to ANARI
 *
 * Translates vtkActor/Mapper state into ANARI rendering calls
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariActorNode_h
#define vtkAnariActorNode_h

#include "vtkActorNode.h"
#include "vtkRenderingAnariModule.h" // For export macro
#include "vtkWeakPointer.h"          // For ivar

VTK_ABI_NAMESPACE_BEGIN

class vtkActor;
class vtkInformationDoubleKey;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringVectorKey;
class vtkInformationStringKey;
class vtkMapper;
class vtkPiecewiseFunction;
class vtkProperty;
class vtkTimeStamp;

class VTKRENDERINGANARI_EXPORT vtkAnariActorNode : public vtkActorNode
{
public:
  static vtkAnariActorNode* New();
  vtkTypeMacro(vtkAnariActorNode, vtkActorNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to take into account my renderables time, including
   * mapper and data into mapper inclusive of composite input
   */
  vtkMTimeType GetMTime() override;

  /**
   * Scaling modes for the spheres and cylinders that the back-end
   * renders for points and lines created by VTK.
   */
  enum class ScalingMode : int8_t
  {
    ALL_EXACT = -1,
    ALL_APPROXIMATE,
    EACH_MAPPED,
    EACH_EXACT
  };

  /**
   * A key to set the ScalingMode. The default is ALL_EXACT.
   * ALL_EXACT means use vtkActor PointSize/LineWidth for all radii.
   * ALL_APPROXIMATE sets all radii to approximate GL's pixel sizes via a function of
   * PointSize/LineWidth and object bounding box.
   * EACH_MAPPED means map every value from SCALE_ARRAY_NAME through the SCALE_FUNCTION
   * lookup table to set each radius independently.
   * EACH_EXACT means use the SCALE_ARRAY_NAME to set each radius directly.
   */
  static vtkInformationIntegerKey* ENABLE_SCALING();

  //@{
  /**
   * Convenience method to set enabled scaling on my renderable.
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

  //@{
  /**
   * Convenience method to get/set a scale array on my renderable.
   */
  static void SetScaleArrayName(const char*, vtkActor*);
  static const char* GetScaleArrayName(vtkActor*);
  //@}

  /**
   * A piecewise function for values from the scale array that alters the resulting
   * radii arbitrarily
   */
  static vtkInformationObjectBaseKey* SCALE_FUNCTION();

  //@{
  /**
   * Convenience method to set a scale function on my renderable.
   */
  static void SetScaleFunction(vtkPiecewiseFunction*, vtkActor*);
  static vtkPiecewiseFunction* GetScaleFunction(vtkActor*);
  //@}

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

  /**
   * Name of the node, used for debugging or representation
   * metadata in case an ANARI backend is chosen which - instead
   * of rendering to a screen - outputs to intermediate
   * authoring stages (such as files or network resources).
   */
  static vtkInformationStringKey* ACTOR_NODE_NAME();

  /**
   * Indicates that the actor includes point and cell attribute arrays
   * within its rendering output. This allows ANARI backends that transfer
   * rendering data to intermediate authoring stages to get access to
   * additional data than what is typically used by VTK's rendering itself.
   */
  static vtkInformationIntegerKey* OUTPUT_POINT_AND_CELL_ARRAYS();

  /**
   * Whether the output enabled with OUTPUT_POINT_AND_CELL_ARRAYS should
   * convert double arrays to float.
   */
  static vtkInformationIntegerKey* OUTPUT_POINT_AND_CELL_ARRAYS_DOUBLE_TO_FLOAT();

  /**
   * Array metadata for intermediate authoring steps, which denotes the arrays
   * which are not written out separately for every timestep,
   * but instead contain only a single representation for all timesteps.
   */
  static vtkInformationStringVectorKey* SCENEGRAPH_TIME_CONSTANT_POINT_ARRAYS();
  static vtkInformationStringVectorKey* SCENEGRAPH_TIME_CONSTANT_CELL_ARRAYS();

protected:
  vtkAnariActorNode();

private:
  vtkAnariActorNode(const vtkAnariActorNode&) = delete;
  void operator=(const vtkAnariActorNode&) = delete;

  vtkWeakPointer<vtkMapper> LastMapper;
  vtkTimeStamp MapperChangedTime;
};

VTK_ABI_NAMESPACE_END
#endif
