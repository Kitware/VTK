/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointData.h
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
// .NAME vtkPointData - represent and manipulate point attribute data
// .SECTION Description
// vtkPointData is a class that is used to represent and manipulate
// point attribute data (e.g., scalars, vectors, normals, texture 
// coordinates, etc.) Special methods are provided to work with filter
// objects, such as passing data through filter, copying data from one 
// point to another, and interpolating data given cell interpolation weights.

#ifndef __vtkPointData_h
#define __vtkPointData_h

#include "vtkObject.h"
#include "vtkScalars.h"
#include "vtkVectors.h"
#include "vtkNormals.h"
#include "vtkTCoords.h"
#include "vtkTensors.h"
#include "vtkUserDefined.h"

class vtkPointData : public vtkObject 
{
public:
  vtkPointData();
  void Initialize();
  ~vtkPointData();
  char *GetClassName() {return "vtkPointData";};
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkPointData (const vtkPointData& pd);
  vtkPointData &operator=(vtkPointData& pd);
  virtual void Update() {};

  // pass thru all input data to output
  void PassData(vtkPointData* pd);

  // use to copy data on a point by point basis
  void CopyAllocate(vtkPointData* pd, int sze=0, int ext=1000);
  void CopyData(vtkPointData *fromPd, int fromId, int toId);

  // use to interpolate data
  void InterpolateAllocate(vtkPointData* pd, int sze=0, int ext=1000);
  void InterpolatePoint(vtkPointData *fromPd, int toId, vtkIdList *ptIds, 
                        float *weights);
  void InterpolateEdge(vtkPointData *fromPd, int toId, int p1, int p2,
                        float t);

  // Set point data to null values
  void NullPoint(int ptId);

  // Reclaim memory
  void Squeeze();

  // Description:
  // Set scalar data.
  vtkSetRefCountedObjectMacro(Scalars,vtkScalars);
  vtkGetObjectMacro(Scalars,vtkScalars);

  // Description:
  // Set vector data.
  vtkSetRefCountedObjectMacro(Vectors,vtkVectors);
  vtkGetObjectMacro(Vectors,vtkVectors);

  // Description:
  // Set normal data.
  vtkSetRefCountedObjectMacro(Normals,vtkNormals);
  vtkGetObjectMacro(Normals,vtkNormals);

  // Description:
  // Set texture coordinate data.
  vtkSetRefCountedObjectMacro(TCoords,vtkTCoords);
  vtkGetObjectMacro(TCoords,vtkTCoords);

  // Description:
  // Set tensor data.
  vtkSetRefCountedObjectMacro(Tensors,vtkTensors);
  vtkGetObjectMacro(Tensors,vtkTensors);

  // Description:
  // Set user defined data.
  vtkSetRefCountedObjectMacro(UserDefined,vtkUserDefined);
  vtkGetObjectMacro(UserDefined,vtkUserDefined);

  // Description:
  // Turn on/off the copying of scalar data.
  vtkSetMacro(CopyScalars,int);
  vtkGetMacro(CopyScalars,int);
  vtkBooleanMacro(CopyScalars,int);

  // Description:
  // Turn on/off the copying of vector data.
  vtkSetMacro(CopyVectors,int);
  vtkGetMacro(CopyVectors,int);
  vtkBooleanMacro(CopyVectors,int);

  // Description:
  // Turn on/off the copying of normals data.
  vtkSetMacro(CopyNormals,int);
  vtkGetMacro(CopyNormals,int);
  vtkBooleanMacro(CopyNormals,int);

  // Description:
  // Turn on/off the copying of texture coordinates data.
  vtkSetMacro(CopyTCoords,int);
  vtkGetMacro(CopyTCoords,int);
  vtkBooleanMacro(CopyTCoords,int);

  // Description:
  // Turn on/off the copying of tensor data.
  vtkSetMacro(CopyTensors,int);
  vtkGetMacro(CopyTensors,int);
  vtkBooleanMacro(CopyTensors,int);

  // Description:
  // Turn on/off the copying of user defined data.
  vtkSetMacro(CopyUserDefined,int);
  vtkGetMacro(CopyUserDefined,int);
  vtkBooleanMacro(CopyUserDefined,int);

  void CopyAllOn();
  void CopyAllOff();

protected:
  vtkScalars *Scalars;
  vtkVectors *Vectors;
  vtkNormals *Normals;
  vtkTCoords *TCoords;
  vtkTensors *Tensors;
  vtkUserDefined *UserDefined;
  int CopyScalars;
  int CopyVectors;
  int CopyNormals;
  int CopyTCoords;
  int CopyTensors;
  int CopyUserDefined;
};

#endif


