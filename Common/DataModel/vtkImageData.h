// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageData
 * @brief   topologically and geometrically regular array of data
 *
 * vtkImageData is a data object that is a concrete implementation of
 * vtkCartesianGrid. vtkImageData represents a geometric structure that is
 * a topological and geometrical regular array of points. Examples include
 * volumes (voxel data) and pixmaps. This representation supports images
 * up to three dimensions. The image may also be oriented (see the
 * DirectionMatrices and related transformation methods). Note however,
 * that not all filters support oriented images. It also supports blanking.
 *
 * @sa
 * vtkImageTransform
 */

#ifndef vtkImageData_h
#define vtkImageData_h

#include "vtkCartesianGrid.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer ivars
#include "vtkStructuredData.h"        // Needed for inline methods
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkStructuredCellArray;
class vtkLine;
class vtkMatrix3x3;
class vtkMatrix4x4;
class vtkPixel;
class vtkPoints;
class vtkVertex;
class vtkVoxel;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkImageData : public vtkCartesianGrid
{
public:
  static vtkImageData* New();
  static vtkImageData* ExtendedNew();

  vtkTypeMacro(vtkImageData, vtkCartesianGrid);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_IMAGE_DATA; }

  ///@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  ///@}

  /**
   * Copy the geometric and topological structure of an input image data
   * object.
   */
  void CopyStructure(vtkDataSet* ds) override;

  using Superclass::FindCell;
  using Superclass::GetCell;

  ///@{
  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  void GetCell(vtkIdType cellId, vtkGenericCell* cell) override;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;
  using vtkDataSet::FindPoint;
  vtkIdType FindPoint(double x[3]) override;
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) override;
  void ComputeBounds() override;
  ///@}

  ///@{
  /**
   * Computes the structured coordinates for a point x[3].
   * The voxel is specified by the array ijk[3], and the parametric coordinates
   * in the cell are specified with pcoords[3]. The function returns a 0 if the
   * point x is outside of the volume, and a 1 if inside the volume, using squared tolerance tol2
   * (1e-12 if not provided).
   */
  int ComputeStructuredCoordinates(const double x[3], int ijk[3], double pcoords[3]) override;
  virtual int ComputeStructuredCoordinates(
    const double x[3], int ijk[3], double pcoords[3], double tol2);
  ///@}

  /**
   * Given a location in structured coordinates (i-j-k), return the point id.
   * Rely on vtkStructuredData::ComputePointIdForExtent
   */
  vtkIdType ComputePointId(int ijk[3]) override;

  /**
   * Given a location in structured coordinates (i-j-k), return the cell id.
   * Rely on vtkStructuredData::ComputeCellIdForExtent
   */
  vtkIdType ComputeCellId(int ijk[3]) override;

  /**
   * Given structured coordinates (i,j,k) for a voxel cell, compute the eight
   * gradient values for the voxel corners. The order in which the gradient
   * vectors are arranged corresponds to the ordering of the voxel points.
   * Gradient vector is computed by central differences (except on edges of
   * volume where forward difference is used). The scalars s are the scalars
   * from which the gradient is to be computed. This method will treat
   * only 3D structured point datasets (i.e., volumes).
   */
  virtual void GetVoxelGradient(int i, int j, int k, vtkDataArray* s, vtkDataArray* g);

  /**
   * Given structured coordinates (i,j,k) for a point in a structured point
   * dataset, compute the gradient vector from the scalar data at that point.
   * The scalars s are the scalars from which the gradient is to be computed.
   * This method will treat structured point datasets of any dimension.
   */
  virtual void GetPointGradient(int i, int j, int k, vtkDataArray* s, double g[3]);

  ///@{
  /**
   * Set / Get the extent on just one axis
   */
  virtual void SetAxisUpdateExtent(
    int axis, int min, int max, const int* updateExtent, int* axisUpdateExtent);
  virtual void GetAxisUpdateExtent(int axis, int& min, int& max, const int* updateExtent);
  ///@}

  ///@{
  /**
   * These returns the minimum and maximum values the ScalarType can hold
   * without overflowing.
   */
  virtual double GetScalarTypeMin(vtkInformation* meta_data);
  virtual double GetScalarTypeMin();
  virtual double GetScalarTypeMax(vtkInformation* meta_data);
  virtual double GetScalarTypeMax();
  ///@}

  ///@{
  /**
   * Get the size of the scalar type in bytes.
   */
  virtual int GetScalarSize(vtkInformation* meta_data);
  virtual int GetScalarSize();
  ///@}

  ///@{
  /**
   * Different ways to get the increments for moving around the data.
   * GetIncrements() calls ComputeIncrements() to ensure the increments are
   * up to date.  The first three methods compute the increments based on the
   * active scalar field while the next three, the scalar field is passed in.
   *
   * Note that all methods which do not have the increments passed in are not
   * thread-safe. When working on a given `vtkImageData` instance on multiple
   * threads, each thread should use the `inc*` overloads to compute the
   * increments to avoid racing with other threads.
   */
  virtual vtkIdType* GetIncrements() VTK_SIZEHINT(3);
  virtual void GetIncrements(vtkIdType& incX, vtkIdType& incY, vtkIdType& incZ);
  virtual void GetIncrements(vtkIdType inc[3]);
  virtual vtkIdType* GetIncrements(vtkDataArray* scalars) VTK_SIZEHINT(3);
  virtual void GetIncrements(
    vtkDataArray* scalars, vtkIdType& incX, vtkIdType& incY, vtkIdType& incZ);
  virtual void GetIncrements(vtkDataArray* scalars, vtkIdType inc[3]);
  ///@}

  ///@{
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
    int extent[6], vtkIdType& incX, vtkIdType& incY, vtkIdType& incZ);
  virtual void GetContinuousIncrements(
    vtkDataArray* scalars, int extent[6], vtkIdType& incX, vtkIdType& incY, vtkIdType& incZ);
  ///@}

  ///@{
  /**
   * Access the native pointer for the scalar data
   */
  virtual void* GetScalarPointerForExtent(int extent[6]);
  virtual void* GetScalarPointer(int coordinates[3]);
  virtual void* GetScalarPointer(int x, int y, int z);
  virtual void* GetScalarPointer();
  ///@}

  ///@{
  /**
   * Access the index for the scalar data
   */
  virtual vtkIdType GetScalarIndexForExtent(int extent[6]);
  virtual vtkIdType GetScalarIndex(int coordinates[3]);
  virtual vtkIdType GetScalarIndex(int x, int y, int z);
  ///@}

  ///@{
  /**
   * For access to data from wrappers
   */
  virtual float GetScalarComponentAsFloat(int x, int y, int z, int component);
  virtual void SetScalarComponentFromFloat(int x, int y, int z, int component, float v);
  virtual double GetScalarComponentAsDouble(int x, int y, int z, int component);
  virtual void SetScalarComponentFromDouble(int x, int y, int z, int component, double v);
  ///@}

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

  ///@{
  /**
   * This method is passed a input and output region, and executes the filter
   * algorithm to fill the output from the input.
   * It just executes a switch statement to call the correct function for
   * the regions data types.
   */
  virtual void CopyAndCastFrom(vtkImageData* inData, int extent[6]);
  virtual void CopyAndCastFrom(vtkImageData* inData, int x0, int x1, int y0, int y1, int z0, int z1)
  {
    int e[6];
    e[0] = x0;
    e[1] = x1;
    e[2] = y0;
    e[3] = y1;
    e[4] = z0;
    e[5] = z1;
    this->CopyAndCastFrom(inData, e);
  }
  ///@}

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

  ///@{
  /**
   * Set the spacing (width,height,length) of the cubical cells that
   * compose the data set.
   */
  vtkGetVector3Macro(Spacing, double);
  virtual void SetSpacing(double i, double j, double k);
  virtual void SetSpacing(const double ijk[3]);
  ///@}

  ///@{
  /**
   * Set/Get the origin of the dataset. The origin is the position in world
   * coordinates of the point of extent (0,0,0). This point does not have to be
   * part of the dataset, in other words, the dataset extent does not have to
   * start at (0,0,0) and the origin can be outside of the dataset bounding
   * box.
   * The origin plus spacing determine the position in space of the points.
   */
  vtkGetVector3Macro(Origin, double);
  virtual void SetOrigin(double i, double j, double k);
  virtual void SetOrigin(const double ijk[3]);
  ///@}

  ///@{
  /**
   * Set/Get the direction transform of the dataset. The direction matrix is
   * a 3x3 transformation matrix supporting scaling and rotation.
   */
  vtkGetObjectMacro(DirectionMatrix, vtkMatrix3x3);
  virtual void SetDirectionMatrix(vtkMatrix3x3* m);
  virtual void SetDirectionMatrix(const double elements[9]);
  virtual void SetDirectionMatrix(double e00, double e01, double e02, double e10, double e11,
    double e12, double e20, double e21, double e22);
  ///@}

  ///@{
  /**
   * Get the transformation matrix from the index space to the physical space
   * coordinate system of the dataset. The transform is a 4 by 4 matrix.
   */
  vtkGetObjectMacro(IndexToPhysicalMatrix, vtkMatrix4x4);
  ///@}

  ///@{
  /**
   * Set the transformation matrix from the index space to the physical space
   * coordinate system of the dataset. The transform is a 4 by 4 matrix.
   * The supplied matrix pointer is not stored in the the image object but the matrix
   * values are used for updating the Origin, Spacing, and DirectionMatrix.
   * \sa SetOrigin
   * \sa SetSpacing
   * \sa SetDirectionMatrix
   */
  void ApplyIndexToPhysicalMatrix(vtkMatrix4x4* source);
  ///@}

  ///@{
  /**
   * Convert coordinates from index space (ijk) to physical space (xyz).
   */
  virtual void TransformContinuousIndexToPhysicalPoint(double i, double j, double k, double xyz[3]);
  virtual void TransformContinuousIndexToPhysicalPoint(const double ijk[3], double xyz[3]);
  virtual void TransformIndexToPhysicalPoint(int i, int j, int k, double xyz[3]);
  virtual void TransformIndexToPhysicalPoint(const int ijk[3], double xyz[3]);
  static void TransformContinuousIndexToPhysicalPoint(double i, double j, double k,
    double const origin[3], double const spacing[3], double const direction[9], double xyz[3]);
  ///@}

  ///@{
  /**
   * Get the transformation matrix from the physical space to the index space
   * coordinate system of the dataset. The transform is a 4 by 4 matrix.
   */
  vtkGetObjectMacro(PhysicalToIndexMatrix, vtkMatrix4x4);
  ///@}

  ///@{
  /**
   * Get the transformation matrix from the physical space to the index space
   * coordinate system of the dataset. The transform is a 4 by 4 matrix.
   * The supplied matrix pointer is not stored in the the image object but the matrix
   * values are used for updating the Origin, Spacing, and DirectionMatrix.
   * \sa SetOrigin
   * \sa SetSpacing
   * \sa SetDirectionMatrix
   */
  void ApplyPhysicalToIndexMatrix(vtkMatrix4x4* source);
  ///@}

  ///@{
  /**
   * Convert coordinates from physical space (xyz) to index space (ijk).
   */
  virtual void TransformPhysicalPointToContinuousIndex(double x, double y, double z, double ijk[3]);
  virtual void TransformPhysicalPointToContinuousIndex(const double xyz[3], double ijk[3]);
  ///@}

  ///@{
  /**
   * Convert normal from physical space (xyz) to index space (ijk).
   */
  virtual void TransformPhysicalNormalToContinuousIndex(const double xyz[3], double ijk[3]);
  ///@}

  /**
   * Convert a plane from physical to a continuous index. The plane is represented as
   * n(x-xo)=0; or using a four component normal: pplane=( nx,ny,nz,-(n(x0)) ).
   */
  virtual void TransformPhysicalPlaneToContinuousIndex(double const pplane[4], double iplane[4]);

  ///@{
  /**
   * Static method to compute the IndexToPhysicalMatrix.
   */
  static void ComputeIndexToPhysicalMatrix(
    double const origin[3], double const spacing[3], double const direction[9], double result[16]);

  ///@{
  /**
   * Static method to compute the PhysicalToIndexMatrix.
   */
  static void ComputePhysicalToIndexMatrix(
    double const origin[3], double const spacing[3], double const direction[9], double result[16]);
  ///@}

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

  //--------------------------------------------------------------------------
  // Methods that apply to any array (not just scalars).
  // I am starting to experiment with generalizing imaging filters
  // to operate on more than just scalars.

  ///@{
  /**
   * These are convenience methods for getting a pointer
   * from any filed array.  It is a start at expanding image filters
   * to process any array (not just scalars).
   */
  void* GetArrayPointerForExtent(vtkDataArray* array, int extent[6]);
  void* GetArrayPointer(vtkDataArray* array, int coordinates[3]);
  ///@}

  ///@{
  /**
   * Given a data array and a coordinate, return the index of the tuple in the
   * array corresponding to that coordinate.
   *
   * This method is analogous to GetArrayPointer(), but it conforms to the API
   * of vtkGenericDataArray.
   */
  vtkIdType GetTupleIndex(vtkDataArray* array, int coordinates[3]);
  ///@}

  /**
   * Since various arrays have different number of components,
   * the will have different increments.
   */
  void GetArrayIncrements(vtkDataArray* array, vtkIdType increments[3]);

  /**
   * Given how many pixel are required on a side for boundary conditions (in
   * bnds), the target extent to traverse, compute the internal extent (the
   * extent for this ImageData that does not suffer from any boundary
   * conditions) and place it in intExt
   */
  void ComputeInternalExtent(int* intExt, int* tgtExt, int* bnds);

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkImageData* GetData(vtkInformation* info);
  static vtkImageData* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkImageData();
  ~vtkImageData() override;

  vtkIdType Increments[3];

  // Variables used to define dataset physical orientation
  double Origin[3];
  double Spacing[3];
  vtkMatrix3x3* DirectionMatrix;
  vtkMatrix4x4* IndexToPhysicalMatrix;
  vtkMatrix4x4* PhysicalToIndexMatrix;

  // The first method assumes Active Scalars
  void ComputeIncrements();
  // This one is given the number of components of the
  // scalar field explicitly
  void ComputeIncrements(int numberOfComponents);
  void ComputeIncrements(vtkDataArray* scalars);

  // The first method assumes Active Scalars
  void ComputeIncrements(vtkIdType inc[3]);
  // This one is given the number of components of the
  // scalar field explicitly
  void ComputeIncrements(int numberOfComponents, vtkIdType inc[3]);
  void ComputeIncrements(vtkDataArray* scalars, vtkIdType inc[3]);

  // for the index to physical methods
  void ComputeTransforms();

  void BuildPoints() override;

  /**
   * Override this method because of blanking.
   */
  void ComputeScalarRange() override;

private:
  void InternalImageDataCopy(vtkImageData* src);

  friend class vtkUniformGrid;

  bool DirectionMatrixIsIdentity;

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
inline void vtkImageData::ComputeIncrements(vtkDataArray* scalars)
{
  this->ComputeIncrements(scalars, this->Increments);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkImageData::ComputePointId(int ijk[3])
{
  return vtkStructuredData::ComputePointIdForExtent(this->GetExtent(), ijk);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkImageData::ComputeCellId(int ijk[3])
{
  return vtkStructuredData::ComputeCellIdForExtent(this->GetExtent(), ijk);
}

VTK_ABI_NAMESPACE_END
#endif
