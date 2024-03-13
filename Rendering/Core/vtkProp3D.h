// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProp3D
 * @brief   represents an 3D object for placement in a rendered scene
 *
 * vtkProp3D is an abstract class used to represent an entity in a rendering
 * scene (i.e., vtkProp3D is a vtkProp with an associated transformation
 * matrix).  It handles functions related to the position, orientation and
 * scaling. It combines these instance variables into one 4x4 transformation
 * matrix as follows: [x y z 1] = [x y z 1] Translate(-origin) Scale(scale)
 * Rot(y) Rot(x) Rot (z) Trans(origin) Trans(position). Both vtkActor and
 * vtkVolume are specializations of class vtkProp.  The constructor defaults
 * to: origin(0,0,0) position=(0,0,0) orientation=(0,0,0), no user defined
 * matrix or transform, and no texture map.
 *
 * @sa
 * vtkProp vtkActor vtkAssembly vtkVolume
 */

#ifndef vtkProp3D_h
#define vtkProp3D_h

#include "vtkNew.h" // for ivar
#include "vtkProp.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWeakPointer.h"         // For vtkWeakPointer
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkLinearTransform;
class vtkMatrix4x4;
class vtkRenderer;
class vtkTransform;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkProp3D : public vtkProp
{
public:
  vtkTypeMacro(vtkProp3D, vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Shallow copy of this vtkProp3D.
   */
  void ShallowCopy(vtkProp* prop) override;

  ///@{
  /**
   * Set/Get/Add the position of the Prop3D in world coordinates.
   */
  virtual void SetPosition(double x, double y, double z)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Position to (" << x << ","
                  << y << "," << z << ")");
    if ((this->Position[0] != x) || (this->Position[1] != y) || (this->Position[2] != z))
    {
      this->Position[0] = x;
      this->Position[1] = y;
      this->Position[2] = z;
      this->Modified();
      this->IsIdentity = 0;
    }
  }
  ///@}

  virtual void SetPosition(double pos[3]) { this->SetPosition(pos[0], pos[1], pos[2]); }
  vtkGetVectorMacro(Position, double, 3);
  void AddPosition(double deltaPosition[3]);
  void AddPosition(double deltaX, double deltaY, double deltaZ);

  ///@{
  /**
   * Set/Get the origin of the Prop3D. This is the point about which all
   * rotations take place.
   */
  virtual void SetOrigin(double x, double y, double z)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Origin to (" << x << ","
                  << y << "," << z << ")");
    if ((this->Origin[0] != x) || (this->Origin[1] != y) || (this->Origin[2] != z))
    {
      this->Origin[0] = x;
      this->Origin[1] = y;
      this->Origin[2] = z;
      this->Modified();
      this->IsIdentity = 0;
    }
  }
  virtual void SetOrigin(const double pos[3]) { this->SetOrigin(pos[0], pos[1], pos[2]); }
  vtkGetVectorMacro(Origin, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the scale of the actor. Scaling in performed independently on the
   * X, Y and Z axis. A scale of zero is illegal and will be replaced with one.
   */
  virtual void SetScale(double x, double y, double z)
  {
    vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Scale to (" << x << ","
                  << y << "," << z << ")");
    if (this->Scale[0] != x || this->Scale[1] != y || this->Scale[2] != z)
    {
      this->Scale[0] = x;
      this->Scale[1] = y;
      this->Scale[2] = z;
      this->Modified();
      this->IsIdentity = 0;
    }
  }
  virtual void SetScale(double scale[3]) { this->SetScale(scale[0], scale[1], scale[2]); }
  vtkGetVectorMacro(Scale, double, 3);
  ///@}

  /**
   * Method to set the scale isotropically
   */
  void SetScale(double s) { this->SetScale(s, s, s); }

  ///@{
  /**
   * In addition to the instance variables such as position and orientation,
   * you can add an additional transformation for your own use.  This
   * transformation is concatenated with the actor's internal transformation,
   * which you implicitly create through the use of SetPosition(),
   * SetOrigin() and SetOrientation().
   * <p>If the internal transformation
   * is identity (i.e. if you don't set the Position, Origin, or
   * Orientation) then the actors final transformation will be the
   * UserTransform, concatenated with the UserMatrix if the UserMatrix
   * is present.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  void SetUserTransform(vtkLinearTransform* transform);
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  vtkGetObjectMacro(UserTransform, vtkLinearTransform);
  ///@}

  ///@{
  /**
   * The UserMatrix can be used in place of UserTransform.
   */
  void SetUserMatrix(vtkMatrix4x4* matrix);
  vtkMatrix4x4* GetUserMatrix();
  ///@}

  ///@{
  /**
   * Return a reference to the Prop3D's 4x4 composite matrix.
   * Get the matrix from the position, origin, scale and orientation This
   * matrix is cached, so multiple GetMatrix() calls will be efficient.
   */
  virtual void GetMatrix(vtkMatrix4x4* result);
  virtual void GetMatrix(double result[16]);
  ///@}

  ///@{
  /**
   * Return a reference to the Prop3D's Model to World matrix.
   * This method takes into account the coordinate system the prop is in.
   */
  virtual void GetModelToWorldMatrix(vtkMatrix4x4* result);
  ///@}

  /**
   * Set the position, scale, orientation from a provided model to world matrix.
   * If the prop is in a coordinate system other than world, then ren must be non-null
   */
  virtual void SetPropertiesFromModelToWorldMatrix(vtkMatrix4x4* modelToWorld);

  /**
   * Return a reference to the Prop3D's composite transform.
   */

  ///@{
  /**
   * Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  void GetBounds(double bounds[6]);
  double* GetBounds() VTK_SIZEHINT(6) override = 0;
  ///@}

  /**
   * Get the center of the bounding box in world coordinates.
   */
  double* GetCenter() VTK_SIZEHINT(3);

  /**
   * Get the Prop3D's x range in world coordinates.
   */
  double* GetXRange() VTK_SIZEHINT(2);

  /**
   * Get the Prop3D's y range in world coordinates.
   */
  double* GetYRange() VTK_SIZEHINT(2);

  /**
   * Get the Prop3D's z range in world coordinates.
   */
  double* GetZRange() VTK_SIZEHINT(2);

  /**
   * Get the length of the diagonal of the bounding box.
   */
  double GetLength();

  /**
   * Rotate the Prop3D in degrees about the X axis using the right hand
   * rule. The axis is the Prop3D's X axis, which can change as other
   * rotations are performed.  To rotate about the world X axis use
   * RotateWXYZ (angle, 1, 0, 0). This rotation is applied before all
   * others in the current transformation matrix.
   */
  void RotateX(double);

  /**
   * Rotate the Prop3D in degrees about the Y axis using the right hand
   * rule. The axis is the Prop3D's Y axis, which can change as other
   * rotations are performed.  To rotate about the world Y axis use
   * RotateWXYZ (angle, 0, 1, 0). This rotation is applied before all
   * others in the current transformation matrix.
   */
  void RotateY(double);

  /**
   * Rotate the Prop3D in degrees about the Z axis using the right hand
   * rule. The axis is the Prop3D's Z axis, which can change as other
   * rotations are performed.  To rotate about the world Z axis use
   * RotateWXYZ (angle, 0, 0, 1). This rotation is applied before all
   * others in the current transformation matrix.
   */
  void RotateZ(double);

  /**
   * Rotate the Prop3D in degrees about an arbitrary axis specified by
   * the last three arguments. The axis is specified in world
   * coordinates. To rotate an about its model axes, use RotateX,
   * RotateY, RotateZ.
   */
  void RotateWXYZ(double w, double x, double y, double z);

  /**
   * Sets the orientation of the Prop3D.  Orientation is specified as
   * X,Y and Z rotations in that order, but they are performed as
   * RotateZ, RotateX, and finally RotateY.
   */
  void SetOrientation(double x, double y, double z);

  /**
   * Sets the orientation of the Prop3D.  Orientation is specified as
   * X,Y and Z rotations in that order, but they are performed as
   * RotateZ, RotateX, and finally RotateY.
   */
  void SetOrientation(double orientation[3]);

  ///@{
  /**
   * Returns the orientation of the Prop3D as s vector of X,Y and Z rotation.
   * The ordering in which these rotations must be done to generate the
   * same matrix is RotateZ, RotateX, and finally RotateY. See also
   * SetOrientation.
   */
  double* GetOrientation() VTK_SIZEHINT(3);
  void GetOrientation(double orentation[3]);
  ///@}

  /**
   * Returns the WXYZ orientation of the Prop3D.
   */
  double* GetOrientationWXYZ() VTK_SIZEHINT(4);

  /**
   * Add to the current orientation. See SetOrientation and
   * GetOrientation for more details. This basically does a
   * GetOrientation, adds the passed in arguments, and then calls
   * SetOrientation.
   */
  void AddOrientation(double x, double y, double z);

  /**
   * Add to the current orientation. See SetOrientation and
   * GetOrientation for more details. This basically does a
   * GetOrientation, adds the passed in arguments, and then calls
   * SetOrientation.
   */
  void AddOrientation(double orentation[3]);

  /**
   * This method modifies the vtkProp3D so that its transformation
   * state is set to the matrix specified. The method does this by
   * setting appropriate transformation-related ivars to initial
   * values (i.e., not transformed), and placing the user-supplied
   * matrix into the UserMatrix of this vtkProp3D. If the method is
   * called again with a NULL matrix, then the original state of the
   * vtkProp3D will be restored. This method is used to support
   * picking and assembly structures.
   */
  void PokeMatrix(vtkMatrix4x4* matrix) override;

  /**
   * Overload vtkProp's method for setting up assembly paths. See
   * the documentation for vtkProp.
   */
  void InitPathTraversal() override;

  /**
   * Get the vtkProp3D's mtime
   */
  vtkMTimeType GetMTime() override;

  /**
   * Get the modified time of the user matrix or user transform.
   */
  vtkMTimeType GetUserTransformMatrixMTime();

  ///@{
  /**
   * Generate the matrix based on ivars
   */
  virtual void ComputeMatrix();
  ///@}

  ///@{
  /**
   * Get a pointer to an internal vtkMatrix4x4. that represents
   */
  vtkMatrix4x4* GetMatrix() override
  {
    this->ComputeMatrix();
    return this->Matrix;
  }
  ///@}

  ///@{
  /**
   * Is the matrix for this actor identity
   */
  vtkGetMacro(IsIdentity, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the coordinate system that this prop is relative to.
   * This defaults to WORLD but can be set to PHYSICAL which for
   * VirtualReality is the physical space (aka room) the viewer
   * is in (in meters). When set to device the CoordinateSystemDevice
   * is used to place the prop relative to that device (such as a HMD
   * or controller)
   */
  enum CoordinateSystems
  {
    WORLD = 0,
    PHYSICAL = 1,
    DEVICE = 2
  };
  void SetCoordinateSystemToWorld() { this->SetCoordinateSystem(WORLD); }
  void SetCoordinateSystemToPhysical() { this->SetCoordinateSystem(PHYSICAL); }
  void SetCoordinateSystemToDevice() { this->SetCoordinateSystem(DEVICE); }
  void SetCoordinateSystem(CoordinateSystems val);
  vtkGetMacro(CoordinateSystem, CoordinateSystems);
  const char* GetCoordinateSystemAsString();
  ///@}

  ///@{
  /**
   * Specify the Renderer that the prop3d is relative to when the
   * coordinate system is set to PHYSICAL or DEVICE
   */
  void SetCoordinateSystemRenderer(vtkRenderer* ren);
  vtkRenderer* GetCoordinateSystemRenderer();
  ///@}

  ///@{
  /**
   * Specify the device to be used when the coordinate system is set
   * to DEVICE. Defaults to vtkEventDataDevice::HeadMountedDisplay.
   */
  vtkSetMacro(CoordinateSystemDevice, int);
  vtkGetMacro(CoordinateSystemDevice, int);
  ///@}

protected:
  vtkProp3D();
  ~vtkProp3D() override;

  vtkLinearTransform* UserTransform;
  vtkMatrix4x4* UserMatrix;
  vtkMatrix4x4* Matrix;
  vtkTimeStamp MatrixMTime;
  double Origin[3];
  double Position[3];
  double Orientation[3];
  double Scale[3];
  double Center[3];
  vtkTransform* Transform;
  double Bounds[6];
  vtkProp3D* CachedProp3D; // support the PokeMatrix() method
  vtkTypeBool IsIdentity;

  int CoordinateSystemDevice;
  CoordinateSystems CoordinateSystem = WORLD;
  vtkWeakPointer<vtkRenderer> CoordinateSystemRenderer;
  vtkNew<vtkMatrix4x4> TempMatrix4x4;

private:
  vtkProp3D(const vtkProp3D&) = delete;
  void operator=(const vtkProp3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
