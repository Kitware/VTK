/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkDataSetAttributes - represent and manipulate attribute data in a dataset
// .SECTION Description
// vtkDataSetAttributes is a class that is used to represent and manipulate
// attribute data (e.g., scalars, vectors, normals, texture coordinates,
// tensors, and field data) Special methods are provided to work with filter
// objects, such as passing data through filter, copying data from one
// attribute set to another, and interpolating data given cell interpolation
// weights.

#ifndef __vtkDataSetAttributes_h
#define __vtkDataSetAttributes_h

#include "vtkObject.h"
#include "vtkScalars.h"
#include "vtkVectors.h"
#include "vtkNormals.h"
#include "vtkTCoords.h"
#include "vtkTensors.h"
#include "vtkFieldData.h"

class VTK_EXPORT vtkDataSetAttributes : public vtkObject 
{
public:

// Description:
// Construct object with copying turned on for all data.
  vtkDataSetAttributes();


// Description:
// Initialize all of the object's data to NULL
  void Initialize();


// Description:
// Destructor for the vtkDataSetAttributes objects.
  ~vtkDataSetAttributes();

  static vtkDataSetAttributes *New() {return new vtkDataSetAttributes;};
  const char *GetClassName() {return "vtkDataSetAttributes";};
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual void Update() {};

  // pass thru all input data to output

// Description:
// Pass entire arrays of input data through to output. Obey the "copy"
// flags.
  void PassData(vtkDataSetAttributes* pd);


  // pass thru all input data to output. Only attribute data that is not
  // already set is passed.

// Description:
// Pass entire arrays of input data through to output. Obey the "copy"
// flags. Only passes the data if the output attribute is NULL (i.e., not set).
  void PassNoReplaceData(vtkDataSetAttributes* pd);


  // use to copy data on a point by point basis

// Description:
// Allocates point data for point-by-point (or cell-by-cell) copy operation.  
// If sze=0, then use the input DataSetAttributes to create (i.e., find 
// initial size of) new objects; otherwise use the sze variable.
  void CopyAllocate(vtkDataSetAttributes* pd, int sze=0, int ext=1000);


// Description:
// Copy the attribute data from one id to another. Make sure CopyAllocate() has
// been invoked before using this method.
  void CopyData(vtkDataSetAttributes *fromPd, int fromId, int toId);


  // use to interpolate data

// Description:
// Initialize point interpolation method.
  void InterpolateAllocate(vtkDataSetAttributes* pd, int sze=0, int ext=1000);

  void InterpolatePoint(vtkDataSetAttributes *fromPd, int toId, vtkIdList *ptIds, 
                        float *weights);

// Description:
// Interpolate data from the two points p1,p2 (forming an edge) and an 
// interpolation factor, t, along the edge. The weight ranges from (0,1), 
// with t=0 located at p1. Make sure that the method InterpolateAllocate() 
// has been invoked before using this method.
  void InterpolateEdge(vtkDataSetAttributes *fromPd, int toId, int p1, int p2,
                        float t);


  // Different ways of copying data

// Description:
// Deep copy of data (i.e., create new data arrays and
// copy from input data).
  void DeepCopy(vtkDataSetAttributes& pd);


// Description:
// Shallow copy of data (i.e., use reference counting).
  void ShallowCopy(vtkDataSetAttributes& pd);


  // Reclaim memory

// Description:
// Resize object to just fit data requirements. Reclaims extra memory.
  void Squeeze();


  // Need to check component pieces for modified time

// Description:
// Check object's components for modified times.
  unsigned long int GetMTime();


  // Description:
  // Set scalar data.
  vtkSetReferenceCountedObjectMacro(Scalars,vtkScalars);
  vtkGetObjectMacro(Scalars,vtkScalars);

  // Description:
  // Set vector data.
  vtkSetReferenceCountedObjectMacro(Vectors,vtkVectors);
  vtkGetObjectMacro(Vectors,vtkVectors);

  // Description:
  // Set normal data.
  vtkSetReferenceCountedObjectMacro(Normals,vtkNormals);
  vtkGetObjectMacro(Normals,vtkNormals);

  // Description:
  // Set texture coordinate data.
  vtkSetReferenceCountedObjectMacro(TCoords,vtkTCoords);
  vtkGetObjectMacro(TCoords,vtkTCoords);

  // Description:
  // Set tensor data.
  vtkSetReferenceCountedObjectMacro(Tensors,vtkTensors);
  vtkGetObjectMacro(Tensors,vtkTensors);

  // Set field data.
  vtkSetReferenceCountedObjectMacro(FieldData,vtkFieldData);
  vtkGetObjectMacro(FieldData,vtkFieldData);

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
  // Turn on/off the copying of field data.
  vtkSetMacro(CopyFieldData,int);
  vtkGetMacro(CopyFieldData,int);
  vtkBooleanMacro(CopyFieldData,int);

  // Description;
  // Flag indicates whether any data is to be copied or interpolated. This
  // flag can be used to improve performance by avoiding extra CopyData() or
  // Interpolate() calls. This method returns valid results only after
  // CopyAllocate() or InterpolateAllocate() has been invoked.
  int GetAnyEnabled() {return this->AnyEnabled;}


// Description:
// Turn on copying of all data.
  void CopyAllOn();


// Description:
// Turn off copying of all data.
  void CopyAllOff();



// Description:
// Copy a tuple of data from one data array to another. This method (and following
// ones) assume that the fromData and toData objects are of the same type, and have
// the same number of components. This is true if you invoke CopyAllocate() or
// InterpolateAllocate().
  void CopyTuple(vtkDataArray *fromData, vtkDataArray *toData, int fromId, int toId);


protected:
  // special methods to support managing data
  void InterpolateTuple(vtkDataArray *fromData, vtkDataArray *toData, int toId,
			vtkIdList *ptIds, float *weights);
  void InterpolateTuple(vtkDataArray *fromData, vtkDataArray *toData, int toId, 
			int id1, int id2, float t);
  
  // support manipulation and access of atribute data
  vtkScalars *Scalars;
  vtkVectors *Vectors;
  vtkNormals *Normals;
  vtkTCoords *TCoords;
  vtkTensors *Tensors;
  vtkFieldData *FieldData;

  // User flags control whether data is to be copied
  int CopyScalars;
  int CopyVectors;
  int CopyNormals;
  int CopyTCoords;
  int CopyTensors;
  int CopyFieldData;

  // Flags are evaluated in CopyAllocate to determine whether copying is possible
  int AnyEnabled;
  int CopyScalarsEnabled;
  int CopyVectorsEnabled;
  int CopyNormalsEnabled;
  int CopyTCoordsEnabled;
  int CopyTensorsEnabled;
  int CopyFieldDataEnabled;

  // used to set null values
  float Null3Tuple[3];
  float Null4Tuple[4];
  vtkTensor NullTensor;
  float *NullTuple;
  int TupleSize;
  float *Tuple;
};

#endif


