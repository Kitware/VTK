/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributes.h
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
#include "vtkGhostLevels.h"

class VTK_EXPORT vtkDataSetAttributes : public vtkObject
{
public:
  // Description:
  // Construct object with copying turned on for all data.
  static vtkDataSetAttributes *New();
  
  vtkTypeMacro(vtkDataSetAttributes,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize all of the object's data to NULL
  void Initialize();

  // Description:
  // Attributes have a chance to bring themselves up to date; right
  // now this is ignored.
  virtual void Update() {};

  // Description:
  // Pass entire arrays of input data through to output. Obey the "copy"
  // flags.
  void PassData(vtkDataSetAttributes* pd);

  // Description:
  // Pass entire arrays of input data through to output. Obey the "copy"
  // flags. Only passes the data if the output attribute is NULL (i.e., not
  // set).
  void PassNoReplaceData(vtkDataSetAttributes* pd);

  // Description:
  // Allocates point data for point-by-point (or cell-by-cell) copy operation.
  // If sze=0, then use the input DataSetAttributes to create (i.e., find 
  // initial size of) new objects; otherwise use the sze variable.
  void CopyAllocate(vtkDataSetAttributes* pd, int sze=0, int ext=1000);

  // Description:
  // Copy the attribute data from one id to another. Make sure CopyAllocate()
  // has been invoked before using this method.
  void CopyData(vtkDataSetAttributes *fromPd, int fromId, int toId);

  // Description:
  // Initialize point interpolation method.
  void InterpolateAllocate(vtkDataSetAttributes* pd, int sze=0, int ext=1000);
  
  // Description:
  // Interpolate data set attributes from other data set attributes
  // given cell or point ids and associated interpolation weights.
  void InterpolatePoint(vtkDataSetAttributes *fromPd, int toId, 
                        vtkIdList *ids, float *weights);
  
  // Description:
  // Interpolate data from the two points p1,p2 (forming an edge) and an 
  // interpolation factor, t, along the edge. The weight ranges from (0,1), 
  // with t=0 located at p1. Make sure that the method InterpolateAllocate() 
  // has been invoked before using this method.
  void InterpolateEdge(vtkDataSetAttributes *fromPd, int toId, int p1, int p2,
                        float t);

  // Description:
  // Interpolate data from the same id (point or cell) at different points
  // in time (parameter t). Two input data set attributes objects are input.
  // The parameter t lies between (0<=t<=1). IMPORTANT: it is assumed that
  // the number of attributes and number of components is the same for both
  // from1 and from2, and the type of data for from1 and from2 are the same.
  // Make sure that the method InterpolateAllocate() has been invoked before 
  // using this method.
  void InterpolateTime(vtkDataSetAttributes *from1, vtkDataSetAttributes *from2,
                       int id, float t);

  // Description:
  // Deep copy of data (i.e., create new data arrays and
  // copy from input data).
  void DeepCopy(vtkDataSetAttributes *pd);

  // Description:
  // Shallow copy of data (i.e., use reference counting).
  void ShallowCopy(vtkDataSetAttributes *pd);

  // Description:
  // Resize object to just fit data requirements. Reclaims extra memory.
  void Squeeze();

  // Description:
  // Check object's components for modified times.
  unsigned long int GetMTime();

  // Description:
  // Set/Get the scalar data.
  vtkSetObjectMacro(Scalars,vtkScalars);
  vtkGetObjectMacro(Scalars,vtkScalars);

  // Description:
  // Set/Get the vector data.
  vtkSetObjectMacro(Vectors,vtkVectors);
  vtkGetObjectMacro(Vectors,vtkVectors);

  // Description:
  // Set/get the normal data.
  vtkSetObjectMacro(Normals,vtkNormals);
  vtkGetObjectMacro(Normals,vtkNormals);

  // Description:
  // Set/Get the ghost level data.
  vtkSetObjectMacro(GhostLevels, vtkGhostLevels);
  vtkGetObjectMacro(GhostLevels, vtkGhostLevels);

  // Description:
  // Set/Get the texture coordinate data.
  vtkSetObjectMacro(TCoords,vtkTCoords);
  vtkGetObjectMacro(TCoords,vtkTCoords);

  // Description:
  // Set/Get the tensor data.
  vtkSetObjectMacro(Tensors,vtkTensors);
  vtkGetObjectMacro(Tensors,vtkTensors);

  // Description:
  // Set/Get the field data.
  vtkSetObjectMacro(FieldData,vtkFieldData);
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
  // Turn on/off the copying of ghost level data.
  vtkSetMacro(CopyGhostLevels, int);
  vtkGetMacro(CopyGhostLevels, int);
  vtkBooleanMacro(CopyGhostLevels, int);

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
  // Copy a tuple of data from one data array to another. This method (and
  // following ones) assume that the fromData and toData objects are of the
  // same type, and have the same number of components. This is true if you
  // invoke CopyAllocate() or InterpolateAllocate().
  void CopyTuple(vtkDataArray *fromData, vtkDataArray *toData, int fromId, 
                 int toId);

  // Description:
  // Return the memory in kilobytes consumed by this attribute data. 
  // Used to support streaming and reading/writing data. The value 
  // returned is guaranteed to be greater than or equal to the memory 
  // required to actually represent the data represented by this object. 
  // The information returned is valid only after the pipeline has 
  // been updated.
  unsigned long GetActualMemorySize();
  
#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // For legacy compatibility. Do not use.
  void DeepCopy(vtkDataSetAttributes &pd) 
    {VTK_LEGACY_METHOD(DeepCopy,"3.2"); this->DeepCopy(&pd);}
  void ShallowCopy(vtkDataSetAttributes &pd) 
    {VTK_LEGACY_METHOD(ShalowCopy,"3.2"); this->ShallowCopy(&pd);}
#endif
  
protected:
  vtkDataSetAttributes();
  ~vtkDataSetAttributes();
  vtkDataSetAttributes(const vtkDataSetAttributes&) {};
  void operator=(const vtkDataSetAttributes&) {};

  // special methods to support managing data
  void InterpolateTuple(vtkDataArray *fromData, vtkDataArray *toData, int toId,
                        vtkIdList *ptIds, float *weights);
  void InterpolateTuple(vtkDataArray *fromData, vtkDataArray *toData, int toId,
                        int id1, int id2, float t);
  void InterpolateTuple(vtkDataArray *fromData1, vtkDataArray *fromData2, 
                        vtkDataArray *toData, int id, float t);
  
  // support manipulation and access of attribute data
  vtkScalars *Scalars;
  vtkVectors *Vectors;
  vtkNormals *Normals;
  vtkTCoords *TCoords;
  vtkTensors *Tensors;
  vtkFieldData *FieldData;
  vtkGhostLevels *GhostLevels;

  // User flags control whether data is to be copied
  int CopyScalars;
  int CopyVectors;
  int CopyNormals;
  int CopyTCoords;
  int CopyTensors;
  int CopyFieldData;
  int CopyGhostLevels;

  // Flags are evaluated in CopyAllocate to determine whether copying is possible
  int AnyEnabled;
  int CopyScalarsEnabled;
  int CopyVectorsEnabled;
  int CopyNormalsEnabled;
  int CopyTCoordsEnabled;
  int CopyTensorsEnabled;
  int CopyFieldDataEnabled;
  int CopyGhostLevelsEnabled;

  // used to set null values
  float Null3Tuple[3];
  float Null4Tuple[4];
  vtkTensor *NullTensor;
  float *NullTuple;
  int TupleSize;
  float *Tuple;
};

#endif


