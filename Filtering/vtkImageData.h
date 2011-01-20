/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkStructuredData.h" // Needed for inline methods

class vtkDataArray;
class vtkLine;
class vtkPixel;
class vtkVertex;
class vtkVoxel;

class VTK_FILTERING_EXPORT vtkImageData : public vtkDataSet
{
public:
  static vtkImageData *New();

  vtkTypeMacro(vtkImageData,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy the geometric and topological structure of an input image data
  // object.
  virtual void CopyStructure(vtkDataSet *ds);

  // Description:
  // Return what type of dataset this is.
  virtual int GetDataObjectType() {return VTK_IMAGE_DATA;};

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  virtual vtkIdType GetNumberOfCells();
  virtual vtkIdType GetNumberOfPoints();
  virtual double *GetPoint(vtkIdType ptId);
  virtual void GetPoint(vtkIdType id, double x[3]);
  virtual vtkCell *GetCell(vtkIdType cellId);
  virtual void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  virtual void GetCellBounds(vtkIdType cellId, double bounds[6]);
  virtual vtkIdType FindPoint(double x, double y, double z)
    {
    return this->vtkDataSet::FindPoint(x, y, z);
    }
  virtual vtkIdType FindPoint(double x[3]);
  virtual vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkIdType cellId, double tol2,
    int& subId, double pcoords[3], double *weights);
  virtual vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkGenericCell *gencell,
    vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double *weights);
  virtual vtkCell *FindAndGetCell(double x[3], vtkCell *cell, vtkIdType cellId,
                                  double tol2, int& subId, double pcoords[3],
                                  double *weights);
  virtual int GetCellType(vtkIdType cellId);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
    {vtkStructuredData::GetCellPoints(cellId,ptIds,this->DataDescription,
                                      this->GetDimensions());}
  virtual void GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());}
  virtual void ComputeBounds();
  virtual int GetMaxCellSize() {return 8;}; //voxel is the largest

  // Description:
  // Restore data object to initial state.
  virtual void Initialize();

  // Description:
  // \deprecated{This is for backward compatibility only - use SetExtent().}
  // Same as SetExtent(0, i-1, 0, j-1, 0, k-1)
  virtual void SetDimensions(int i, int j, int k);

  // Description:
  // \deprecated{This is for backward compatibility only - use SetExtent().}
  // Same as SetExtent(0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1)
  virtual void SetDimensions(const int dims[3]);

  // Description:
  // Get dimensions of this structured points dataset.
  // It is the number of points on each axis.
  // Dimensions are computed from Extents during this call.
  virtual int *GetDimensions();
  virtual void GetDimensions(int dims[3]);

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The voxel is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the volume, and a 1 if inside the volume.
  virtual int ComputeStructuredCoordinates(
    double x[3], int ijk[3], double pcoords[3]);

  // Description:
  // Given structured coordinates (i,j,k) for a voxel cell, compute the eight
  // gradient values for the voxel corners. The order in which the gradient
  // vectors are arranged corresponds to the ordering of the voxel points.
  // Gradient vector is computed by central differences (except on edges of
  // volume where forward difference is used). The scalars s are the scalars
  // from which the gradient is to be computed. This method will treat
  // only 3D structured point datasets (i.e., volumes).
  virtual void GetVoxelGradient(
    int i,int j,int k, vtkDataArray *s, vtkDataArray *g);

  // Description:
  // Given structured coordinates (i,j,k) for a point in a structured point
  // dataset, compute the gradient vector from the scalar data at that point.
  // The scalars s are the scalars from which the gradient is to be computed.
  // This method will treat structured point datasets of any dimension.
  virtual void GetPointGradient(
    int i, int j, int k, vtkDataArray *s, double g[3]);

  // Description:
  // Return the dimensionality of the data.
  virtual int GetDataDimension();

  // Description:
  // Given a location in structured coordinates (i-j-k), return the point id.
  virtual vtkIdType ComputePointId(int ijk[3]) {
    return vtkStructuredData::ComputePointIdForExtent(this->Extent,ijk);};

  // Description:
  // Given a location in structured coordinates (i-j-k), return the cell id.
  virtual vtkIdType ComputeCellId(int ijk[3]) {
    return vtkStructuredData::ComputeCellIdForExtent(this->Extent,ijk);};

  // Description:
  // Set / Get the extent on just one axis
  virtual void SetAxisUpdateExtent(int axis, int min, int max);
  virtual void GetAxisUpdateExtent(int axis, int &min, int &max);

  // Description:
  // Override to copy information from pipeline information to data
  // information for backward compatibility.  See
  // vtkDataObject::UpdateInformation for details.
  virtual void UpdateInformation();

  // Description:
  // Set/Get the extent. On each axis, the extent is defined by the index
  // of the first point and the index of the last point.  The extent should
  // be set before the "Scalars" are set or allocated.  The Extent is
  // stored in the order (X, Y, Z).
  // The dataset extent does not have to start at (0,0,0). (0,0,0) is just the
  // extent of the origin.
  // The first point (the one with Id=0) is at extent
  // (Extent[0],Extent[2],Extent[4]). As for any dataset, a data array on point
  // data starts at Id=0.
  virtual void SetExtent(int extent[6]);
  virtual void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Get the estimated size of this data object itself. Should be called
  // after UpdateInformation() and PropagateUpdateExtent() have both been
  // called. This estimate should be fairly accurate since this is structured
  // data.
  virtual unsigned long GetEstimatedMemorySize();

  // Description:
  // These returns the minimum and maximum values the ScalarType can hold
  // without overflowing.
  virtual double GetScalarTypeMin();
  virtual double GetScalarTypeMax();

  // Description:
  // Get the size of the scalar type in bytes.
  virtual int GetScalarSize();

  // Description:
  // Different ways to get the increments for moving around the data.
  // GetIncrements() calls ComputeIncrements() to ensure the increments are
  // up to date.
  virtual vtkIdType *GetIncrements();
  virtual void GetIncrements(vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ);
  virtual void GetIncrements(vtkIdType inc[3]);

  // Description:
  // Different ways to get the increments for moving around the data.
  // incX is always returned with 0.  incY is returned with the
  // increment needed to move from the end of one X scanline of data
  // to the start of the next line.  incZ is filled in with the
  // increment needed to move from the end of one image to the start
  // of the next.  The proper way to use these values is to for a loop
  // over Z, Y, X, C, incrementing the pointer by 1 after each
  // component.  When the end of the component is reached, the pointer
  // is set to the beginning of the next pixel, thus incX is properly set to 0.
  virtual void GetContinuousIncrements(
    int extent[6], vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ);

  // Description:
  // Access the native pointer for the scalar data
  virtual void *GetScalarPointerForExtent(int extent[6]);
  virtual void *GetScalarPointer(int coordinates[3]);
  virtual void *GetScalarPointer(int x, int y, int z);
  virtual void *GetScalarPointer();

  // Description:
  // For access to data from tcl
  virtual float GetScalarComponentAsFloat(int x, int y, int z, int component);
  virtual void SetScalarComponentFromFloat(
    int x, int y, int z, int component, float v);
  virtual double GetScalarComponentAsDouble(int x, int y, int z, int component);
  virtual void SetScalarComponentFromDouble(
    int x, int y, int z, int component, double v);

  // Description:
  // Allocate the vtkScalars object associated with this object.
  virtual void AllocateScalars();

  // Description:
  // This method is passed a input and output region, and executes the filter
  // algorithm to fill the output from the input.
  // It just executes a switch statement to call the correct function for
  // the regions data types.
  virtual void CopyAndCastFrom(vtkImageData *inData, int extent[6]);
  virtual void CopyAndCastFrom(vtkImageData *inData, int x0, int x1,
                               int y0, int y1, int z0, int z1)
    {int e[6]; e[0]=x0; e[1]=x1; e[2]=y0; e[3]=y1; e[4]=z0; e[5]=z1;
    this->CopyAndCastFrom(inData, e);}

  // Description:
  // Reallocates and copies to set the Extent to the UpdateExtent.
  // This is used internally when the exact extent is requested,
  // and the source generated more than the update extent.
  virtual void Crop();

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  virtual unsigned long GetActualMemorySize();

  // Description:
  // Set the spacing (width,height,length) of the cubical cells that
  // compose the data set.
  vtkSetVector3Macro(Spacing,double);
  vtkGetVector3Macro(Spacing,double);

  // Description:
  // Set/Get the origin of the dataset. The origin is the position in world
  // coordinates of the point of extent (0,0,0). This point does not have to be
  // part of the dataset, in other words, the dataset extent does not have to
  // start at (0,0,0) and the origin can be outside of the dataset bounding
  // box.
  // The origin plus spacing determine the position in space of the points.
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);

  // Description:
  // Set/Get the data scalar type (i.e VTK_DOUBLE). Note that these methods
  // are setting and getting the pipeline scalar type. i.e. they are setting
  // the type that the image data will be once it has executed. Until the
  // REQUEST_DATA pass the actual scalars may be of some other type. This is
  // for backwards compatibility
  void SetScalarTypeToFloat(){this->SetScalarType(VTK_FLOAT);};
  void SetScalarTypeToDouble(){this->SetScalarType(VTK_DOUBLE);};
  void SetScalarTypeToInt(){this->SetScalarType(VTK_INT);};
  void SetScalarTypeToUnsignedInt()
    {this->SetScalarType(VTK_UNSIGNED_INT);};
  void SetScalarTypeToLong(){this->SetScalarType(VTK_LONG);};
  void SetScalarTypeToUnsignedLong()
    {this->SetScalarType(VTK_UNSIGNED_LONG);};
  void SetScalarTypeToShort(){this->SetScalarType(VTK_SHORT);};
  void SetScalarTypeToUnsignedShort()
    {this->SetScalarType(VTK_UNSIGNED_SHORT);};
  void SetScalarTypeToUnsignedChar()
    {this->SetScalarType(VTK_UNSIGNED_CHAR);};
  void SetScalarTypeToSignedChar()
    {this->SetScalarType(VTK_SIGNED_CHAR);};
  void SetScalarTypeToChar()
    {this->SetScalarType(VTK_CHAR);};
  void SetScalarType(int);
  int GetScalarType();
  const char* GetScalarTypeAsString()
    { return vtkImageScalarTypeNameMacro ( this->GetScalarType() ); };

  // Description:
  // Set/Get the number of scalar components for points. As with the
  // SetScalarType method this is setting pipeline info.
  void SetNumberOfScalarComponents( int n );
  int GetNumberOfScalarComponents();

  // Must only be called with vtkImageData (or subclass) as input
  virtual void CopyTypeSpecificInformation( vtkDataObject *image );

  // Description:
  // Override these to handle origin, spacing, scalar type, and scalar
  // number of components.  See vtkDataObject for details.
  virtual void CopyInformationToPipeline(vtkInformation* request,
                                         vtkInformation* input,
                                         vtkInformation* output,
                                         int forceCopy);
  virtual void CopyInformationFromPipeline(vtkInformation* request);

  // Description:
  // make the output data ready for new data to be inserted. For most
  // objects we just call Initialize. But for image data we leave the old
  // data in case the memory can be reused.
  virtual void PrepareForNewData();

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);
  virtual void DeepCopy(vtkDataObject *src);

  //--------------------------------------------------------------------------
  // Methods that apply to any array (not just scalars).
  // I am starting to experiment with generalizing imaging fitlers
  // to operate on more than just scalars.

  // Description:
  // These are convenience methods for getting a pointer
  // from any filed array.  It is a start at expanding image filters
  // to process any array (not just scalars).
  void *GetArrayPointerForExtent(vtkDataArray* array, int extent[6]);
  void *GetArrayPointer(vtkDataArray* array, int coordinates[3]);

  // Description:
  // Since various arrays have different number of components,
  // the will have different increments.
  void GetArrayIncrements(vtkDataArray *array, vtkIdType increments[3]);

  // Description:
  // Given how many pixel are required on a side for bounrary conditions (in
  // bnds), the target extent to traverse, compute the internal extent (the
  // extent for this ImageData that does nto suffer from any boundary
  // conditions) and place it in intExt
  void ComputeInternalExtent(int *intExt, int *tgtExt, int *bnds);

  // Description:
  // The extent type is a 3D extent
  virtual int GetExtentType() { return VTK_3D_EXTENT; };

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkImageData* GetData(vtkInformation* info);
  static vtkImageData* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  vtkImageData();
  ~vtkImageData();

  // The extent of what is currently in the structured grid.
  // Dimensions is just an array to return a value.
  // Its contents are out of data until GetDimensions is called.
  int Dimensions[3];
  vtkIdType Increments[3];

  double Origin[3];
  double Spacing[3];

  int Extent[6];

  void ComputeIncrements();
  void ComputeIncrements(vtkIdType inc[3]);
  void CopyOriginAndSpacingFromPipeline();

  vtkTimeStamp ExtentComputeTime;

  void SetDataDescription(int desc);
  int GetDataDescription() { return this->DataDescription; }

private:
  void InternalImageDataCopy(vtkImageData *src);
private:

  //BTX
  friend class vtkUniformGrid;
  //ETX

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  int DataDescription;

  vtkImageData(const vtkImageData&);  // Not implemented.
  void operator=(const vtkImageData&);  // Not implemented.
};


//----------------------------------------------------------------------------
inline void vtkImageData::ComputeIncrements()
{
  this->ComputeIncrements(this->Increments);
}

//----------------------------------------------------------------------------
inline void vtkImageData::GetPoint(vtkIdType id, double x[3])
{
  const double *p = this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

//----------------------------------------------------------------------------
inline vtkIdType vtkImageData::GetNumberOfPoints()
{
  const int *extent = this->Extent;
  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  return dims[0]*dims[1]*dims[2];
}

//----------------------------------------------------------------------------
inline int vtkImageData::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

#endif
