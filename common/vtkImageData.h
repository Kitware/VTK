/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.h
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
// .NAME vtkImageData - topologically and geometrically regular array of data
// .SECTION Description
// vtkImageData is a data object that is a concrete implementation of
// vtkDataSet. vtkImageData represents a geometric structure that is 
// a topological and geometrical regular array of points. Examples include
// volumes (voxel data) and pixmaps. 

#ifndef __vtkImageData_h
#define __vtkImageData_h

#include "vtkDataSet.h"
#include "vtkStructuredData.h"
class vtkVertex;
class vtkLine;
class vtkPixel;
class vtkVoxel;


class VTK_EXPORT vtkImageData : public vtkDataSet
{
public:
  static vtkImageData *New();

  vtkTypeMacro(vtkImageData,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the same type object as this (virtual constructor).
  vtkDataObject *MakeObject() {return vtkImageData::New();};

  // Description:
  // Copy the geometric and topological structure of an input image data
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_IMAGE_DATA;};

  // Description:
  // This update method will supply the ghost level arrays if they are requested.
  void UpdateData();
  
  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  vtkIdType GetNumberOfCells();
  vtkIdType GetNumberOfPoints();
  float *GetPoint(vtkIdType ptId);
  void GetPoint(vtkIdType id, float x[3]);
  vtkCell *GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  void GetCellBounds(vtkIdType cellId, float bounds[6]);
  vtkIdType FindPoint(float x, float y, float z) { return this->vtkDataSet::FindPoint(x, y, z);};
  vtkIdType FindPoint(float x[3]);
  vtkIdType FindCell(float x[3], vtkCell *cell, vtkIdType cellId, float tol2, 
                     int& subId, float pcoords[3], float *weights);
  vtkIdType FindCell(float x[3], vtkCell *cell, vtkGenericCell *gencell,
                     vtkIdType cellId, float tol2, int& subId, 
                     float pcoords[3], float *weights);
  vtkCell *FindAndGetCell(float x[3], vtkCell *cell, vtkIdType cellId, 
                          float tol2, int& subId, float pcoords[3],
                          float *weights);
  int GetCellType(vtkIdType cellId);
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
    {vtkStructuredData::GetCellPoints(cellId,ptIds,this->DataDescription,
				      this->GetDimensions());}
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());}
  void ComputeBounds();
  int GetMaxCellSize() {return 8;}; //voxel is the largest

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int i, int j, int k);

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int dims[3]);

  // Description:
  // Get dimensions of this structured points dataset.
  // Dimensions are computed from Extents during this call.
  int *GetDimensions();
  void GetDimensions(int dims[3]);

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The voxel is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the volume, and a 1 if inside the volume.
  int ComputeStructuredCoordinates(float x[3], int ijk[3], float pcoords[3]);
  
  // Description:
  // Given structured coordinates (i,j,k) for a voxel cell, compute the eight 
  // gradient values for the voxel corners. The order in which the gradient
  // vectors are arranged corresponds to the ordering of the voxel points. 
  // Gradient vector is computed by central differences (except on edges of 
  // volume where forward difference is used). The scalars s are the scalars
  // from which the gradient is to be computed. This method will treat 
  // only 3D structured point datasets (i.e., volumes).
  void GetVoxelGradient(int i,int j,int k, vtkScalars *s, vtkVectors *g);

  // Description:
  // Given structured coordinates (i,j,k) for a point in a structured point 
  // dataset, compute the gradient vector from the scalar data at that point. 
  // The scalars s are the scalars from which the gradient is to be computed.
  // This method will treat structured point datasets of any dimension.
  void GetPointGradient(int i, int j, int k, vtkScalars *s, float g[3]);

  // Description:
  // Return the dimensionality of the data.
  int GetDataDimension();

  // Description:
  // Given a location in structured coordinates (i-j-k), return the point id.
  vtkIdType ComputePointId(int ijk[3]) {
    return vtkStructuredData::ComputePointId(this->GetDimensions(),ijk);};

  // Description:
  // Given a location in structured coordinates (i-j-k), return the cell id.
  vtkIdType ComputeCellId(int ijk[3]) {
    return vtkStructuredData::ComputeCellId(this->GetDimensions(),ijk);};

  // Description:
  // Set / Get the extent on just one axis
  void SetAxisUpdateExtent(int axis, int min, int max);
  void GetAxisUpdateExtent(int axis, int &min, int &max);

  // Description:
  // Required for the lowest common denominator for setting the UpdateExtent
  // (i.e. vtkDataSetToStructuredPointsFilter).  This assumes that WholeExtent
  // is valid (UpdateInformation has been called).
  void SetUpdateExtent(int piece, int numPieces, int ghostLevel);
  void SetUpdateExtent(int piece, int numPieces)
    {this->SetUpdateExtent(piece, numPieces, 0);}
  
  // Description:
  // Call superclass method to avoid hiding
  void SetUpdateExtent( int x1, int x2, int y1, int y2, int z1, int z2 )
    { this->vtkDataSet::SetUpdateExtent( x1, x2, y1, y2, z1, z2 ); };
  void SetUpdateExtent( int ext[6] )
    { this->vtkDataSet::SetUpdateExtent( ext ); };

  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored  in the order (X, Y, Z).
  void SetExtent(int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent,int);

  // Description:
  // Get the estimated size of this data object itself. Should be called
  // after UpdateInformation() and PropagateUpdateExtent() have both been 
  // called. This estimate should be fairly accurate since this is structured
  // data.
  virtual unsigned long GetEstimatedMemorySize();

  // Description:
  // These returns the minimum and maximum values the ScalarType can hold
  // without overflowing.
  double GetScalarTypeMin();
  double GetScalarTypeMax();
  
  // Description:
  // Set the size of the scalar type in bytes.
  int GetScalarSize();

  // Description:
  // Different ways to get the increments for moving around the data.
  // GetIncrements() calls ComputeIncrements() to ensure the increments are
  // up to date.
  int *GetIncrements();
  void GetIncrements(int &incX, int &incY, int &incZ);
  void GetIncrements(int inc[3]);
  
  // Description:
  // Different ways to get the increments for moving around the data.
  // They are store (Component, X, Y, Z). This method returns
  // increments that are suited for continuous incrementing of the
  // pointer in a Z, Y, X, C nested for loop.
  void GetContinuousIncrements(int extent[6], int &incX, int &incY,
			       int &incZ);
  
  // Description:
  // Access the native pointer for the scalar data
  void *GetScalarPointerForExtent(int coordinates[6]);
  void *GetScalarPointer(int coordinates[3]);
  void *GetScalarPointer(int x, int y, int z);
  void *GetScalarPointer();

  // Description:
  // For access to data from tcl
  float GetScalarComponentAsFloat(int x, int y, int z, int component);
  
  // Description:
  // Allocate the vtkScalars object associated with this object.
  void AllocateScalars();
  
  // Description:
  // This method is passed a input and output region, and executes the filter
  // algorithm to fill the output from the input.
  // It just executes a switch statement to call the correct function for
  // the regions data types.
  void CopyAndCastFrom(vtkImageData *inData, int extent[6]);
  void CopyAndCastFrom(vtkImageData *inData, int x0, int x1,
                       int y0, int y1, int z0, int z1)
    {int e[6]; e[0]=x0; e[1]=x1; e[2]=y0; e[3]=y1; e[4]=z0; e[5]=z1; 
    this->CopyAndCastFrom(inData, e);}

  // Description:
  // Reallocates and copies to set the Extent to the UpdateExtent.
  // This is used internally when the exact extent is requested, 
  // and the source generated more than the update extent. 
  void Crop();

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Set the spacing (width,height,length) of the cubical cells that
  // compose the data set.
  vtkSetVector3Macro(Spacing,float);
  vtkGetVector3Macro(Spacing,float);
  
  // Description:
  // Set the origin of the data. The origin plus spacing determine the
  // position in space of the points.
  vtkSetVector3Macro(Origin,float);
  vtkGetVector3Macro(Origin,float);
  
  // Description:
  // Set/Get the data scalar type (i.e VTK_FLOAT).
  vtkSetMacro(ScalarType, int);
  int GetScalarType();

  // Description:
  // Set/Get the number of scalar components for points.
  void SetNumberOfScalarComponents( int n );
  vtkGetMacro(NumberOfScalarComponents,int);

  // Must only be called with vtkImageData (or subclass) as input
  void CopyTypeSpecificInformation( vtkDataObject *image );

  // Description:
  // make the output data ready for new data to be inserted. For most 
  // objects we just call Initialize. But for image data we leave the old
  // data in case the memory can be reused.
  virtual void PrepareForNewData();

  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);  
  void DeepCopy(vtkDataObject *src);

protected:
  vtkImageData();
  ~vtkImageData();
  vtkImageData(const vtkImageData&) {};
  void operator=(const vtkImageData&) {};

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  // The extent type is a 3D extent
  int GetExtentType() { return VTK_3D_EXTENT; };

  // The extent of what is currently in the structured grid.
  // Dimensions is just an array to return a value.
  // Its contents are out of data until GetDimensions is called.
  int Dimensions[3];
  int DataDescription;
  int Increments[3];

  float Origin[3];
  float Spacing[3];
  int ScalarType;
  int NumberOfScalarComponents;

  void ComputeIncrements();

private:
  void InternalImageDataCopy(vtkImageData *src);
};


inline void vtkImageData::GetPoint(vtkIdType id, float x[3])
{
  float *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}



inline vtkIdType vtkImageData::GetNumberOfPoints()
{
  int *dims = this->GetDimensions();
  return dims[0]*dims[1]*dims[2];
}

inline int vtkImageData::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

#endif



