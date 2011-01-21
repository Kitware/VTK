/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix4x4.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMatrix4x4 - represent and manipulate 4x4 transformation matrices
// .SECTION Description
// vtkMatrix4x4 is a class to represent and manipulate 4x4 matrices. 
// Specifically, it is designed to work on 4x4 transformation matrices
// found in 3D rendering using homogeneous coordinates [x y z w].

// .SECTION See Also
// vtkTransform

#ifndef __vtkMatrix4x4_h
#define __vtkMatrix4x4_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkMatrix4x4 : public vtkObject
{
  // Some of the methods in here have a corresponding static (class)
  // method taking a pointer to 16 doubles that constitutes a user
  // supplied matrix. This allows C++ clients to allocate double arrays
  // on the stack and manipulate them using vtkMatrix4x4 methods. 
  // This is an alternative to allowing vtkMatrix4x4 instances to be
  // created on the stack (which is frowned upon) or doing lots of
  // temporary heap allocation within vtkTransform and vtkActor methods,
  // which is inefficient.

public:
  double Element[4][4];

  // Description:
  // Construct a 4x4 identity matrix.
  static vtkMatrix4x4 *New();

  vtkTypeMacro(vtkMatrix4x4,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the elements of the matrix to the same values as the elements
  // of the source Matrix.
  void DeepCopy(const vtkMatrix4x4 *source)
    {vtkMatrix4x4::DeepCopy(*this->Element,source); this->Modified(); }
//BTX
  static void DeepCopy(double Elements[16], const vtkMatrix4x4 *source)
    {vtkMatrix4x4::DeepCopy(Elements,*source->Element); }
  static void DeepCopy(double Elements[16], const double newElements[16]);
//ETX

  // Description:
  // Non-static member function. Assigns *from* elements array
  void DeepCopy(const double Elements[16]) 
    { this->DeepCopy(*this->Element,Elements); this->Modified(); }

  // Description:
  // Set all of the elements to zero.
  void Zero() 
    { vtkMatrix4x4::Zero(*this->Element); this->Modified(); }
//BTX
  static void Zero(double Elements[16]);
//ETX  

  // Description:
  // Set equal to Identity matrix
  void Identity() 
    { vtkMatrix4x4::Identity(*this->Element); this->Modified();}
//BTX
  static void Identity(double Elements[16]);
//ETX  

  // Description:
  // Matrix Inversion (adapted from Richard Carling in "Graphics Gems," 
  // Academic Press, 1990).
  static void Invert(const vtkMatrix4x4 *in, vtkMatrix4x4 *out)
    {vtkMatrix4x4::Invert(*in->Element,*out->Element); out->Modified(); }
  void Invert() 
    { vtkMatrix4x4::Invert(this,this); }
//BTX
  static void Invert(const double inElements[16], double outElements[16]);
//ETX


  // Description:
  // Transpose the matrix and put it into out. 
  static void Transpose(const vtkMatrix4x4 *in, vtkMatrix4x4 *out)
    {vtkMatrix4x4::Transpose(*in->Element,*out->Element); out->Modified(); }
  void Transpose() 
    { vtkMatrix4x4::Transpose(this,this); }
//BTX
  static void Transpose(const double inElements[16], double outElements[16]);
//ETX

  // Description:
  // Multiply a homogeneous coordinate by this matrix, i.e. out = A*in.
  // The in[4] and out[4] can be the same array.
  void MultiplyPoint(const float in[4], float out[4]) 
    {vtkMatrix4x4::MultiplyPoint(*this->Element,in,out); }
  void MultiplyPoint(const double in[4], double out[4]) 
    {vtkMatrix4x4::MultiplyPoint(*this->Element,in,out); }

//BTX
  static void MultiplyPoint(const double Elements[16], 
                            const float in[4], float out[4]);
  static void MultiplyPoint(const double Elements[16], 
                            const double in[4], double out[4]);
//ETX

  // Description:
  // For use in Java, Python or Tcl.  The default MultiplyPoint() uses
  // a single-precision point.
  float *MultiplyPoint(const float in[4]) 
    {return this->MultiplyFloatPoint(in); }
  float *MultiplyFloatPoint(const float in[4]) 
    {this->MultiplyPoint(in,this->FloatPoint); return this->FloatPoint; } 
  double *MultiplyDoublePoint(const double in[4]) 
    {this->MultiplyPoint(in,this->DoublePoint); return this->DoublePoint; } 

  // Description:
  // Multiplies matrices a and b and stores the result in c.
  static void Multiply4x4(const vtkMatrix4x4 *a, const vtkMatrix4x4 *b, vtkMatrix4x4 *c) {
    vtkMatrix4x4::Multiply4x4(*a->Element,*b->Element,*c->Element); };
//BTX
  static void Multiply4x4(const double a[16], const double b[16], 
                          double c[16]);
//ETX

  // Description:
  // Compute adjoint of the matrix and put it into out.
  void Adjoint(const vtkMatrix4x4 *in, vtkMatrix4x4 *out)
    {vtkMatrix4x4::Adjoint(*in->Element,*out->Element);}
//BTX
  static void Adjoint(const double inElements[16], double outElements[16]);
//ETX

  // Description:
  // Compute the determinant of the matrix and return it.
  double Determinant() {return vtkMatrix4x4::Determinant(*this->Element);}
//BTX
  static double Determinant(const double Elements[16]);
//ETX

  // Description:
  // Sets the element i,j in the matrix.
  void SetElement(int i, int j, double value);

  // Description:
  // Returns the element i,j from the matrix.
  double GetElement(int i, int j) const 
    {return this->Element[i][j];}

//BTX
  double *operator[](const unsigned int i) 
    {return &(this->Element[i][0]);}
  const double *operator[](unsigned int i) const
    { return &(this->Element[i][0]); }  
  void Adjoint(vtkMatrix4x4 &in,vtkMatrix4x4 &out)
    {this->Adjoint(&in,&out);}
  double Determinant(vtkMatrix4x4 &in) 
    {return this->Determinant(&in);}
  double Determinant(vtkMatrix4x4 *in) 
    {return vtkMatrix4x4::Determinant(*in->Element);}
  void Invert(vtkMatrix4x4 &in,vtkMatrix4x4 &out)
    {this->Invert(&in,&out);}
  void Transpose(vtkMatrix4x4 &in,vtkMatrix4x4 &out)
    {this->Transpose(&in,&out);}
  static void PointMultiply(const double Elements[16], 
                            const float in[4], float out[4]);
  static void PointMultiply(const double Elements[16], 
                            const double in[4], double out[4]);
//ETX

protected:
  vtkMatrix4x4() { vtkMatrix4x4::Identity(*this->Element); };
  ~vtkMatrix4x4() {};
  
  float FloatPoint[4];
  double DoublePoint[4];
private:
  vtkMatrix4x4(const vtkMatrix4x4&);  // Not implemented
  void operator= (const vtkMatrix4x4&);  // Not implemented
};

inline void vtkMatrix4x4::SetElement(int i, int j, double value)
{
  if (this->Element[i][j] != value)
    {
    this->Element[i][j] = value;
    this->Modified();
    }
}

#endif

