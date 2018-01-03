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
/**
 * @class   vtkImageData
 * @brief   topologically and geometrically regular array of data
 *
 * vtkImageData is a data object that is a concrete implementation of
 * vtkDataSet. vtkImageData represents a geometric structure that is
 * a topological and geometrical regular array of points. Examples include
 * volumes (voxel data) and pixmaps.
*/

#ifndef vtkImageData_h
#define vtkImageData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"

#include "vtkStructuredData.h" // Needed for inline methods

class vtkDataArray;
class vtkLine;
class vtkPixel;
class vtkVertex;
class vtkVoxel;

class VTKCOMMONDATAMODEL_EXPORT vtkImageData : public vtkDataSet
{
public:
  static vtkImageData *New();

  vtkTypeMacro(vtkImageData,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copy the geometric and topological structure of an input image data
   * object.
   */
  void CopyStructure(vtkDataSet *ds) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() override {return VTK_IMAGE_DATA;};

  //@{
  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  vtkIdType GetNumberOfCells() override;
  vtkIdType GetNumberOfPoints() override;
  double *GetPoint(vtkIdType ptId) VTK_SIZEHINT(3) override;
  void GetPoint(vtkIdType id, double x[3]) override;
  vtkCell *GetCell(vtkIdType cellId) override;
  vtkCell *GetCell(int i, int j, int k) override;
  void GetCell(vtkIdType cellId, vtkGenericCell *cell) override;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;
  virtual vtkIdType FindPoint(double x, double y, double z)
  {
    return this->vtkDataSet::FindPoint(x, y, z);
  }
  vtkIdType FindPoint(double x[3]) override;
  vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkIdType cellId, double tol2,
    int& subId, double pcoords[3], double *weights) override;
  vtkIdType FindCell(
    double x[3], vtkCell *cell, vtkGenericCell *gencell,
    vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double *weights) override;
  vtkCell *FindAndGetCell(double x[3], vtkCell *cell, vtkIdType cellId,
                                  double tol2, int& subId, double pcoords[3],
                                  double *weights) override;
  int GetCellType(vtkIdType cellId) override;
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds) override
    {vtkStructuredData::GetCellPoints(cellId,ptIds,this->DataDescription,
                                      this->GetDimensions());}
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds) override
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->GetDimensions());}
  void ComputeBounds() override;
  int GetMaxCellSize() override {return 8;}; //voxel is the largest
  //@}

  /**
   * Restore data object to initial state.
   */
  void Initialize() override;

  /**
   * Same as SetExtent(0, i-1, 0, j-1, 0, k-1)
   */
  virtual void SetDimensions(int i, int j, int k);

  /**
   * Same as SetExtent(0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1)
   */
  virtual void SetDimensions(const int dims[3]);

  /**
   * Get dimensions of this structured points dataset.
   * It is the number of points on each axis.
   * Dimensions are computed from Extents during this call.
   * \warning Non thread-safe, use second signature if you want it to be.
   */
  virtual int *GetDimensions() VTK_SIZEHINT(3);

  /**
   * Get dimensions of this structured points dataset.
   * It is the number of points on each axis.
   * This method is thread-safe.
   * \warning The Dimensions member variable is not updated during this call.
   */
  virtual void GetDimensions(int dims[3]);

  /**
   * Convenience function computes the structured coordinates for a point x[3].
   * The voxel is specified by the array ijk[3], and the parametric coordinates
   * in the cell are specified with pcoords[3]. The function returns a 0 if the
   * point x is outside of the volume, and a 1 if inside the volume.
   */
  virtual int ComputeStructuredCoordinates(
    const double x[3],  int ijk[3],  double pcoords[3]);

  static int ComputeStructuredCoordinates( const double x[3], int ijk[3], double pcoords[3],
                                           const int* extent,
                                           const double* spacing,
                                           const double* origin,
                                           const double* bounds);
  /**
   * Given structured coordinates (i,j,k) for a voxel cell, compute the eight
   * gradient values for the voxel corners. The order in which the gradient
   * vectors are arranged corresponds to the ordering of the voxel points.
   * Gradient vector is computed by central differences (except on edges of
   * volume where forward difference is used). The scalars s are the scalars
   * from which the gradient is to be computed. This method will treat
   * only 3D structured point datasets (i.e., volumes).
   */
  virtual void GetVoxelGradient(
    int i,int j,int k, vtkDataArray *s, vtkDataArray *g);

  /**
   * Given structured coordinates (i,j,k) for a point in a structured point
   * dataset, compute the gradient vector from the scalar data at that point.
   * The scalars s are the scalars from which the gradient is to be computed.
   * This method will treat structured point datasets of any dimension.
   */
  virtual void GetPointGradient(
    int i, int j, int k, vtkDataArray *s, double g[3]);

  /**
   * Return the dimensionality of the data.
   */
  virtual int GetDataDimension();

  /**
   * Given a location in structured coordinates (i-j-k), return the point id.
   */
  virtual vtkIdType ComputePointId(int ijk[3]) {
    return vtkStructuredData::ComputePointIdForExtent(this->Extent,ijk);};

  /**
   * Given a location in structured coordinates (i-j-k), return the cell id.
   */
  virtual vtkIdType ComputeCellId(int ijk[3]) {
    return vtkStructuredData::ComputeCellIdForExtent(this->Extent,ijk);};

  //@{
  /**
   * Set / Get the extent on just one axis
   */
  virtual void SetAxisUpdateExtent(int axis, int min, int max,
                                   const int* updateExtent,
                                   int* axisUpdateExtent);
  virtual void GetAxisUpdateExtent(int axis, int &min, int &max, const int* updateExtent);
  //@}

  //@{
  /**
   * Set/Get the extent. On each axis, the extent is defined by the index
   * of the first point and the index of the last point.  The extent should
   * be set before the "Scalars" are set or allocated.  The Extent is
   * stored in the order (X, Y, Z).
   * The dataset extent does not have to start at (0,0,0). (0,0,0) is just the
   * extent of the origin.
   * The first point (the one with Id=0) is at extent
   * (Extent[0],Extent[2],Extent[4]). As for any dataset, a data array on point
   * data starts at Id=0.
   */
  virtual void SetExtent(int extent[6]);
  virtual void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent, int);
  //@}

  //@{
  /**
   * These returns the minimum and maximum values the ScalarType can hold
   * without overflowing.
   */
  virtual double GetScalarTypeMin(vtkInformation* meta_data);
  virtual double GetScalarTypeMin();
  virtual double GetScalarTypeMax(vtkInformation* meta_data);
  virtual double GetScalarTypeMax();
  //@}

  //@{
  /**
   * Get the size of the scalar type in bytes.
   */
  virtual int GetScalarSize(vtkInformation* meta_data);
  virtual int GetScalarSize();
  //@}

  //@{
  /**
   * Different ways to get the increments for moving around the data.
   * GetIncrements() calls ComputeIncrements() to ensure the increments are
   * up to date.  The first three methods compute the increments based on the
   * active scalar field while the next three, the scalar field is passed in.
   */
  virtual vtkIdType *GetIncrements() VTK_SIZEHINT(3);
  virtual void GetIncrements(vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ);
  virtual void GetIncrements(vtkIdType inc[3]);
  virtual vtkIdType *GetIncrements(vtkDataArray *scalars) VTK_SIZEHINT(3);
  virtual void GetIncrements(vtkDataArray *scalars,
                             vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ);
  virtual void GetIncrements(vtkDataArray *scalars, vtkIdType inc[3]);
  //@}

  //@{
  /**
   * Different ways to get the increments for moving around the data.
   * incX is always returned with 0.  incY is returned with the
   * increment needed to move from the end of one X scanline of data
   * to the start of the next line.  incZ is filled in with the
   * increment needed to move from the end of one image to the start
   * of the next.  The proper way to use these values is to for a loop
   * over Z, Y, X, C, incrementing the pointer by 1 after each
   * component.  When the end of the component is reached, the pointer
   * is set to the beginning of the next pixel, thus incX is properly set to 0.
   * The first form of GetContinuousIncrements uses the active scalar field
   * while the second form allows the scalar array to be passed in.
   */
  virtual void GetContinuousIncrements(
    int extent[6], vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ);
  virtual void GetContinuousIncrements(vtkDataArray *scalars,
    int extent[6], vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ);
  //@}

  //@{
  /**
   * Access the native pointer for the scalar data
   */
  virtual void *GetScalarPointerForExtent(int extent[6]);
  virtual void *GetScalarPointer(int coordinates[3]);
  virtual void *GetScalarPointer(int x, int y, int z);
  virtual void *GetScalarPointer();
  //@}

  //@{
  /**
   * For access to data from wrappers
   */
  virtual float GetScalarComponentAsFloat(int x, int y, int z, int component);
  virtual void SetScalarComponentFromFloat(
    int x, int y, int z, int component, float v);
  virtual double GetScalarComponentAsDouble(int x, int y, int z, int component);
  virtual void SetScalarComponentFromDouble(
    int x, int y, int z, int component, double v);
  //@}

  /**
   * Allocate the point scalars for this dataset. The data type determines
   * the type of the array (VTK_FLOAT, VTK_INT etc.) where as numComponents
   * determines its number of components.
   */
  virtual void AllocateScalars(int dataType, int numComponents);

  /**
   * Allocate the point scalars for this dataset. The data type and the
   * number of components of the array is determined by the meta-data in
   * the pipeline information. This is usually produced by a reader/filter
   * upstream in the pipeline.
   */
  virtual void AllocateScalars(vtkInformation* pipeline_info);

  //@{
  /**
   * This method is passed a input and output region, and executes the filter
   * algorithm to fill the output from the input.
   * It just executes a switch statement to call the correct function for
   * the regions data types.
   */
  virtual void CopyAndCastFrom(vtkImageData *inData, int extent[6]);
  virtual void CopyAndCastFrom(vtkImageData *inData, int x0, int x1,
                               int y0, int y1, int z0, int z1)
    {int e[6]; e[0]=x0; e[1]=x1; e[2]=y0; e[3]=y1; e[4]=z0; e[5]=z1;
    this->CopyAndCastFrom(inData, e);}
  //@}

  /**
   * Reallocates and copies to set the Extent to updateExtent.
   * This is used internally when the exact extent is requested,
   * and the source generated more than the update extent.
   */
  void Crop(const int* updateExtent) override;

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() override;

  //@{
  /**
   * Set the spacing (width,height,length) of the cubical cells that
   * compose the data set.
   */
  vtkSetVector3Macro(Spacing,double);
  vtkGetVector3Macro(Spacing,double);
  //@}

  //@{
  /**
   * Set/Get the origin of the dataset. The origin is the position in world
   * coordinates of the point of extent (0,0,0). This point does not have to be
   * part of the dataset, in other words, the dataset extent does not have to
   * start at (0,0,0) and the origin can be outside of the dataset bounding
   * box.
   * The origin plus spacing determine the position in space of the points.
   */
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);
  //@}

  static void SetScalarType(int, vtkInformation* meta_data);
  static int GetScalarType(vtkInformation* meta_data);
  static bool HasScalarType(vtkInformation* meta_data);
  int GetScalarType();
  const char* GetScalarTypeAsString()
    { return vtkImageScalarTypeNameMacro ( this->GetScalarType() ); };

  //@{
  /**
   * Set/Get the number of scalar components for points. As with the
   * SetScalarType method this is setting pipeline info.
   */
  static void SetNumberOfScalarComponents( int n, vtkInformation* meta_data);
  static int GetNumberOfScalarComponents(vtkInformation* meta_data);
  static bool HasNumberOfScalarComponents(vtkInformation* meta_data);
  int GetNumberOfScalarComponents();
  //@}

  /**
   * Override these to handle origin, spacing, scalar type, and scalar
   * number of components.  See vtkDataObject for details.
   */
  void CopyInformationFromPipeline(vtkInformation* information) override;

  /**
   * Copy information from this data object to the pipeline information.
   * This is used by the vtkTrivialProducer that is created when someone
   * calls SetInputData() to connect the image to a pipeline.
   */
  void CopyInformationToPipeline(vtkInformation* information) override;

  /**
   * make the output data ready for new data to be inserted. For most
   * objects we just call Initialize. But for image data we leave the old
   * data in case the memory can be reused.
   */
  void PrepareForNewData() override;

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject *src) override;
  void DeepCopy(vtkDataObject *src) override;
  //@}

  //--------------------------------------------------------------------------
  // Methods that apply to any array (not just scalars).
  // I am starting to experiment with generalizing imaging filters
  // to operate on more than just scalars.

  //@{
  /**
   * These are convenience methods for getting a pointer
   * from any filed array.  It is a start at expanding image filters
   * to process any array (not just scalars).
   */
  void *GetArrayPointerForExtent(vtkDataArray* array, int extent[6]);
  void *GetArrayPointer(vtkDataArray* array, int coordinates[3]);
  //@}

  /**
   * Since various arrays have different number of components,
   * the will have different increments.
   */
  void GetArrayIncrements(vtkDataArray *array, vtkIdType increments[3]);

  /**
   * Given how many pixel are required on a side for bounrary conditions (in
   * bnds), the target extent to traverse, compute the internal extent (the
   * extent for this ImageData that does not suffer from any boundary
   * conditions) and place it in intExt
   */
  void ComputeInternalExtent(int *intExt, int *tgtExt, int *bnds);

  /**
   * The extent type is a 3D extent
   */
  int GetExtentType() override { return VTK_3D_EXTENT; };

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkImageData* GetData(vtkInformation* info);
  static vtkImageData* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkImageData();
  ~vtkImageData() override;

  // The extent of what is currently in the structured grid.
  // Dimensions is just an array to return a value.
  // Its contents are out of data until GetDimensions is called.
  int Dimensions[3];
  vtkIdType Increments[3];

  double Origin[3];
  double Spacing[3];

  int Extent[6];

  // The first method assumes Active Scalars
  void ComputeIncrements();
  // This one is given the number of components of the
  // scalar field explicitly
  void ComputeIncrements(int numberOfComponents);
  void ComputeIncrements(vtkDataArray *scalars);

  // The first method assumes Acitive Scalars
  void ComputeIncrements(vtkIdType inc[3]);
  // This one is given the number of components of the
  // scalar field explicitly
  void ComputeIncrements(int numberOfComponents, vtkIdType inc[3]);
  void ComputeIncrements(vtkDataArray *scalars, vtkIdType inc[3]);
  void CopyOriginAndSpacingFromPipeline(vtkInformation* info);

  vtkTimeStamp ExtentComputeTime;

  void SetDataDescription(int desc);
  int GetDataDescription() { return this->DataDescription; }

private:
  void InternalImageDataCopy(vtkImageData *src);
private:

  friend class vtkUniformGrid;

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  // for the GetPoint method
  double Point[3];

  int DataDescription;

  vtkImageData(const vtkImageData&) = delete;
  void operator=(const vtkImageData&) = delete;
};


//----------------------------------------------------------------------------
inline void vtkImageData::ComputeIncrements()
{
  this->ComputeIncrements(this->Increments);
}

//----------------------------------------------------------------------------
inline void vtkImageData::ComputeIncrements(int numberOfComponents)
{
  this->ComputeIncrements(numberOfComponents, this->Increments);
}

//----------------------------------------------------------------------------
inline void vtkImageData::ComputeIncrements(vtkDataArray *scalars)
{
  this->ComputeIncrements(scalars, this->Increments);
}

//----------------------------------------------------------------------------
inline double * vtkImageData::GetPoint(vtkIdType id)
{
  this->GetPoint(id, this->Point);
  return this->Point;
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
