/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensors.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkTensors - abstract interface to tensors
// .SECTION Description
// vtkTensors provides an abstract interface to n-dimensional tensors. The 
// data model for vtkTensors is a list of arrays of nxn tensor matrices 
// accessible by point id. The subclasses of vtkTensors are concrete data 
// types (float, int, etc.) that implement the interface of vtkTensors.

#ifndef __vtkTensors_h
#define __vtkTensors_h

#include "vtkReferenceCount.h"
#include "vtkTensor.h"

class vtkIdList;
class vtkFloatTensors;

class VTK_EXPORT vtkTensors : public vtkReferenceCount 
{
public:
  vtkTensors(int dim=3);
  const char *GetClassName() {return "vtkTensors";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkTensors *MakeObject(int sze, int d=3, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "unsigned char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of tensors in array.
  virtual int GetNumberOfTensors() = 0;

  // Description:
  // Return a float tensor t[dim*dim] for a particular point id.
  virtual vtkTensor *GetTensor(int id) = 0;

  // Description:
  // Copy float tensor into user provided tensor
  // for specified point id.
  virtual void GetTensor(int id, vtkTensor& t);

  // Description:
  // Specify the number of tensors for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetTensor() method for fast insertion.
  virtual void SetNumberOfTensors(int number) = 0;

  // Description:
  // Insert tensor into object. No range checking performed (fast!).
  virtual void SetTensor(int id, vtkTensor *t) = 0;

  // Description:
  // Insert tensor into object. Range checking performed and 
  // memory allocated as necessary.
  virtual void InsertTensor(int id, vtkTensor *t) = 0;
  void InsertTensor(int id, float t11, float t12, float t13, 
                    float t21, float t22, float t23, 
                    float t31, float t32, float t33);

  // Description:
  // Insert tensor into next available slot. Returns point
  // id of slot.
  virtual int InsertNextTensor(vtkTensor *t) = 0;
  int InsertNextTensor(float t11, float t12, float t13, 
                       float t21, float t22, float t23, 
                       float t31, float t32, float t33);

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetTensors(vtkIdList& ptId, vtkFloatTensors& ft);

  vtkSetClampMacro(Dimension,int,1,3);
  vtkGetMacro(Dimension,int);

protected:
  int Dimension;

};

// These include files are placed here so that if Tensors.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"
#include "vtkFloatTensors.h"

#endif
