/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProp3D - represents an 3D object for placement in a rendered scene 
// .SECTION Description
// vtkProp3D is an abstract class used to represent an entity in a rendering
// scene (i.e., vtkProp3D is a vtkProp with an associated transformation
// matrix).  It handles functions related to the position, orientation and
// scaling. It combines these instance variables into one 4x4 transformation
// matrix as follows: [x y z 1] = [x y z 1] Translate(-origin) Scale(scale)
// Rot(y) Rot(x) Rot (z) Trans(origin) Trans(position). Both vtkActor and
// vtkVolume are specializations of class vtkProp.  The constructor defaults
// to: origin(0,0,0) position=(0,0,0) orientation=(0,0,0), no user defined 
// matrix or transform, and no texture map.

// .SECTION See Also
// vtkProp vtkActor vtkAssembly vtkVolume

#ifndef __vtkProp3D_h
#define __vtkProp3D_h

#include "vtkProp.h"

class vtkRenderer;
class vtkTransform;
class vtkLinearTransform;

class VTK_RENDERING_EXPORT vtkProp3D : public vtkProp
{
public:
  vtkTypeMacro(vtkProp3D,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Shallow copy of this vtkProp3D.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Set/Get/Add the position of the Prop3D in world coordinates.
  virtual void SetPosition(double _arg1, double _arg2, double _arg3) 
    { 
      vtkDebugMacro(<< this->GetClassName() << " (" << this << 
      "): setting Position to (" << _arg1 << "," << _arg2 << "," << 
      _arg3 << ")"); 
      if ((this->Position[0] != _arg1)||
          (this->Position[1] != _arg2)||
          (this->Position[2] != _arg3)) 
        { 
        this->Position[0] = _arg1; 
        this->Position[1] = _arg2; 
        this->Position[2] = _arg3; 
        this->Modified(); 
        this->IsIdentity = 0;
        } 
    }; 
  virtual void SetPosition (double _arg[3]) 
    { 
      this->SetPosition (_arg[0], _arg[1], _arg[2]);
    } 
  vtkGetVectorMacro(Position,double,3);
  void AddPosition(double deltaPosition[3]);
  void AddPosition(double deltaX,double deltaY,double deltaZ);

  // Description:
  // Set/Get the origin of the Prop3D. This is the point about which all 
  // rotations take place.
  virtual void SetOrigin(double _arg1, double _arg2, double _arg3) 
    { 
      vtkDebugMacro(<< this->GetClassName() << " (" << this << 
      "): setting Origin to (" << _arg1 << "," << _arg2 << "," << 
      _arg3 << ")"); 
      if ((this->Origin[0] != _arg1)||
          (this->Origin[1] != _arg2)||
          (this->Origin[2] != _arg3)) 
        { 
        this->Origin[0] = _arg1; 
        this->Origin[1] = _arg2; 
        this->Origin[2] = _arg3; 
        this->Modified(); 
        this->IsIdentity = 0;
        } 
    }; 
  virtual void SetOrigin(double _arg[3]) 
    { 
      this->SetOrigin (_arg[0], _arg[1], _arg[2]);
    } 
  vtkGetVectorMacro(Origin,double,3);

  // Description:
  // Set/Get the scale of the actor. Scaling in performed independently on the
  // X, Y and Z axis. A scale of zero is illegal and will be replaced with one.
  virtual void SetScale(double _arg1, double _arg2, double _arg3)
    {
    vtkDebugMacro(<< this->GetClassName() << " (" << this <<
      "): setting Scale to (" << _arg1 << "," << _arg2 << "," <<
      _arg3 << ")");
    if (this->Scale[0] != _arg1 ||
        this->Scale[1] != _arg2 ||
        this->Scale[2] != _arg3 )
      {
      this->Scale[0] = _arg1;
      this->Scale[1] = _arg2;
      this->Scale[2] = _arg3;
      this->Modified();
      this->IsIdentity = 0;
      }
    };
  virtual void SetScale (double _arg[3])
    {
    this->SetScale (_arg[0], _arg[1], _arg[2]);
    }
  vtkGetVectorMacro(Scale,double,3);

  // Description:
  // Method to set the scale isotropically
  void SetScale(double s) {this->SetScale(s,s,s);};

  // Description:
  // In addition to the instance variables such as position and orientation,
  // you can add an additional transformation for your own use.  This
  // transformation is concatenated with the actor's internal transformation,
  // which you implicitly create through the use of SetPosition(),
  // SetOrigin() and SetOrientation().
  // <p>If the internal transformation
  // is identity (i.e. if you don't set the Position, Origin, or
  // Orientation) then the actors final transformation will be the
  // UserTransform, concatenated with the UserMatrix if the UserMatrix
  // is present.
  void SetUserTransform(vtkLinearTransform *transform);
  vtkGetObjectMacro(UserTransform,vtkLinearTransform);

  // Description:
  // The UserMatrix can be used in place of UserTransform.
  void SetUserMatrix(vtkMatrix4x4 *matrix);
  vtkMatrix4x4 *GetUserMatrix();

  // Description:
  // Return a reference to the Prop3D's 4x4 composite matrix.
  // Get the matrix from the position, origin, scale and orientation This
  // matrix is cached, so multiple GetMatrix() calls will be efficient.
  virtual void GetMatrix(vtkMatrix4x4 *m);
  virtual void GetMatrix(double m[16]);

  // Description:
  // Return a reference to the Prop3D's composite transform.

  // Description:
  // Get the bounds for this Prop3D as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  void GetBounds(double bounds[6]);
  virtual double *GetBounds() = 0;
  
  // Description:
  // Get the center of the bounding box in world coordinates.
  double *GetCenter();

  // Description:
  // Get the Prop3D's x range in world coordinates.
  double *GetXRange();

  // Description:
  // Get the Prop3D's y range in world coordinates.
  double *GetYRange();

  // Description:
  // Get the Prop3D's z range in world coordinates.
  double *GetZRange();

  // Description:
  // Get the length of the diagonal of the bounding box.
  double GetLength();

  // Description:
  // Rotate the Prop3D in degrees about the X axis using the right hand
  // rule. The axis is the Prop3D's X axis, which can change as other
  // rotations are performed.  To rotate about the world X axis use
  // RotateWXYZ (angle, 1, 0, 0). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateX(double);

  // Description:
  // Rotate the Prop3D in degrees about the Y axis using the right hand
  // rule. The axis is the Prop3D's Y axis, which can change as other
  // rotations are performed.  To rotate about the world Y axis use
  // RotateWXYZ (angle, 0, 1, 0). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateY(double);

  // Description:
  // Rotate the Prop3D in degrees about the Z axis using the right hand
  // rule. The axis is the Prop3D's Z axis, which can change as other
  // rotations are performed.  To rotate about the world Z axis use
  // RotateWXYZ (angle, 0, 0, 1). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateZ(double);

  // Description:
  // Rotate the Prop3D in degrees about an arbitrary axis specified by
  // the last three arguments. The axis is specified in world
  // coordinates. To rotate an about its model axes, use RotateX,
  // RotateY, RotateZ.
  void RotateWXYZ(double,double,double,double);

  // Description:
  // Sets the orientation of the Prop3D.  Orientation is specified as
  // X,Y and Z rotations in that order, but they are performed as
  // RotateZ, RotateX, and finally RotateY.
  void SetOrientation(double,double,double);

  // Description:
  // Sets the orientation of the Prop3D.  Orientation is specified as
  // X,Y and Z rotations in that order, but they are performed as
  // RotateZ, RotateX, and finally RotateY.
  void SetOrientation(double a[3]);

  // Description:
  // Returns the orientation of the Prop3D as s vector of X,Y and Z rotation.
  // The ordering in which these rotations must be done to generate the 
  // same matrix is RotateZ, RotateX, and finally RotateY. See also 
  // SetOrientation.
  double *GetOrientation();
  void GetOrientation(double o[3]);

  // Description:
  // Returns the WXYZ orientation of the Prop3D. 
  double *GetOrientationWXYZ();

  // Description:
  // Add to the current orientation. See SetOrientation and
  // GetOrientation for more details. This basically does a
  // GetOrientation, adds the passed in arguments, and then calls
  // SetOrientation.
  void AddOrientation(double,double,double);

  // Description:
  // Add to the current orientation. See SetOrientation and
  // GetOrientation for more details. This basically does a
  // GetOrientation, adds the passed in arguments, and then calls
  // SetOrientation.
  void AddOrientation(double a[3]);

  // Description:
  // This method modifies the vtkProp3D so that its transformation
  // state is set to the matrix specified. The method does this by
  // setting appropriate transformation-related ivars to initial
  // values (i.e., not transformed), and placing the user-supplied
  // matrix into the UserMatrix of this vtkProp3D. If the method is
  // called again with a NULL matrix, then the original state of the
  // vtkProp3D will be restored. This method is used to support
  // picking and assembly structures.
  void PokeMatrix(vtkMatrix4x4 *matrix);

  // Description:
  // Overload vtkProp's method for setting up assembly paths. See
  // the documentation for vtkProp.
  void InitPathTraversal();

  // Description:
  // Get the vtkProp3D's mtime 
  unsigned long int GetMTime();

  // Description:
  // Get the modified time of the user matrix or user transform.
  unsigned long int GetUserTransformMatrixMTime();
 
  // Description:
  // Generate the matrix based on ivars
  virtual void ComputeMatrix();

  // Description:
  // Get a pointer to an internal vtkMatrix4x4. that represents
  vtkMatrix4x4 *GetMatrix() 
    { 
      this->ComputeMatrix();
      return this->Matrix; 
    }

  // Description: 
  // Is the matrix for this actor identity
  vtkGetMacro(IsIdentity,int);
  
protected:
  vtkProp3D();
  ~vtkProp3D();

  vtkLinearTransform *UserTransform;
  vtkMatrix4x4  *UserMatrix;
  vtkMatrix4x4  *Matrix;
  vtkTimeStamp  MatrixMTime;
  double         Origin[3];
  double         Position[3];
  double         Orientation[3];
  double         Scale[3];
  double         Center[3];
  vtkTransform  *Transform;
  double         Bounds[6];
  vtkProp3D     *CachedProp3D; //support the PokeMatrix() method
  int           IsIdentity;
private:
  vtkProp3D(const vtkProp3D&);  // Not implemented.
  void operator=(const vtkProp3D&);  // Not implemented.
};

#endif

