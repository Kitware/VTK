/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkTransform.h"

class vtkRenderer;

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
  virtual void SetPosition(float _arg1, float _arg2, float _arg3) 
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
  virtual void SetPosition (float _arg[3]) 
    { 
      this->SetPosition (_arg[0], _arg[1], _arg[2]);
    } 
  vtkGetVectorMacro(Position,float,3);
  void AddPosition(float deltaPosition[3]);
  void AddPosition(float deltaX,float deltaY,float deltaZ);

  // Description:
  // Set/Get the origin of the Prop3D. This is the point about which all 
  // rotations take place.
  virtual void SetOrigin(float _arg1, float _arg2, float _arg3) 
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
  virtual void SetOrigin(float _arg[3]) 
    { 
      this->SetOrigin (_arg[0], _arg[1], _arg[2]);
    } 
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Set/Get the scale of the actor. Scaling in performed independently on the
  // X, Y and Z axis. A scale of zero is illegal and will be replaced with one.
  virtual void SetScale(float _arg1, float _arg2, float _arg3) 
    { 
      vtkDebugMacro(<< this->GetClassName() << " (" << this << 
      "): setting Scale to (" << _arg1 << "," << _arg2 << "," << 
      _arg3 << ")"); 
      if ((this->Scale[0] != _arg1)||
          (this->Scale[1] != _arg2)||
          (this->Scale[2] != _arg3)) 
        { 
        this->Scale[0] = _arg1; 
        this->Scale[1] = _arg2; 
        this->Scale[2] = _arg3; 
        this->Modified(); 
        this->IsIdentity = 0;
        } 
    }; 
  virtual void SetScale (float _arg[3]) 
    { 
      this->SetScale (_arg[0], _arg[1], _arg[2]);
    } 
  vtkGetVectorMacro(Scale,float,3);

  // Description:
  // Method to set the scale isotropically
  void SetScale(float s) {this->SetScale(s,s,s);};

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
  vtkMatrix4x4 *GetUserMatrix() { 
    if (this->UserTransform) { this->UserTransform->Update(); };
    return this->UserMatrix; };

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
  void GetBounds(float bounds[6]);
  virtual float *GetBounds() = 0;
  
  // Description:
  // Get the center of the bounding box in world coordinates.
  float *GetCenter();

  // Description:
  // Get the Prop3D's x range in world coordinates.
  float *GetXRange();

  // Description:
  // Get the Prop3D's y range in world coordinates.
  float *GetYRange();

  // Description:
  // Get the Prop3D's z range in world coordinates.
  float *GetZRange();

  // Description:
  // Get the length of the diagonal of the bounding box.
  float GetLength();

  // Description:
  // Rotate the Prop3D in degrees about the X axis using the right hand
  // rule. The axis is the Prop3D's X axis, which can change as other
  // rotations are performed.  To rotate about the world X axis use
  // RotateWXYZ (angle, 1, 0, 0). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateX(float);

  // Description:
  // Rotate the Prop3D in degrees about the Y axis using the right hand
  // rule. The axis is the Prop3D's Y axis, which can change as other
  // rotations are performed.  To rotate about the world Y axis use
  // RotateWXYZ (angle, 0, 1, 0). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateY(float);

  // Description:
  // Rotate the Prop3D in degrees about the Z axis using the right hand
  // rule. The axis is the Prop3D's Z axis, which can change as other
  // rotations are performed.  To rotate about the world Z axis use
  // RotateWXYZ (angle, 0, 0, 1). This rotation is applied before all
  // others in the current transformation matrix.
  void RotateZ(float);

  // Description:
  // Rotate the Prop3D in degrees about an arbitrary axis specified by
  // the last three arguments. The axis is specified in world
  // coordinates. To rotate an about its model axes, use RotateX,
  // RotateY, RotateZ.
  void RotateWXYZ(float,float,float,float);

  // Description:
  // Sets the orientation of the Prop3D.  Orientation is specified as
  // X,Y and Z rotations in that order, but they are performed as
  // RotateZ, RotateX, and finally RotateY.
  void SetOrientation(float,float,float);

  // Description:
  // Sets the orientation of the Prop3D.  Orientation is specified as
  // X,Y and Z rotations in that order, but they are performed as
  // RotateZ, RotateX, and finally RotateY.
  void SetOrientation(float a[3]);

  // Description:
  // Returns the orientation of the Prop3D as s vector of X,Y and Z rotation.
  // The ordering in which these rotations must be done to generate the 
  // same matrix is RotateZ, RotateX, and finally RotateY. See also 
  // SetOrientation.
  float *GetOrientation();
  void GetOrientation(float o[3]);

  // Description:
  // Returns the WXYZ orientation of the Prop3D. 
  float *GetOrientationWXYZ();

  // Description:
  // Add to the current orientation. See SetOrientation and
  // GetOrientation for more details. This basically does a
  // GetOrientation, adds the passed in arguments, and then calls
  // SetOrientation.
  void AddOrientation(float,float,float);

  // Description:
  // Add to the current orientation. See SetOrientation and
  // GetOrientation for more details. This basically does a
  // GetOrientation, adds the passed in arguments, and then calls
  // SetOrientation.
  void AddOrientation(float a[3]);

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
  vtkProp3D(const vtkProp3D&);
  void operator=(const vtkProp3D&);

  vtkLinearTransform *UserTransform;
  vtkMatrix4x4  *UserMatrix;
  vtkMatrix4x4  *Matrix;
  vtkTimeStamp  MatrixMTime;
  float         Origin[3];
  float         Position[3];
  float         Orientation[3];
  float         Scale[3];
  float         Center[3];
  vtkTransform  *Transform;
  float         Bounds[6];
  vtkProp3D     *CachedProp3D; //support the PokeMatrix() method
  int           IsIdentity;
};

#endif

