#ifndef __vtkKMeansDistanceFunctor_h
#define __vtkKMeansDistanceFunctor_h

// .NAME vtkKMeansDistanceFunctor - measure distance from k-means cluster centers
// .SECTION Description
// This is an abstract class (with a default concrete subclass) that implements
// algorithms used by the vtkKMeansStatistics filter that rely on a distance metric.
// If you wish to use a non-Euclidean distance metric (this could include
// working with strings that do not have a Euclidean distance metric, implementing
// k-mediods, or trying distance metrics in norms other than L2), you
// should subclass vtkKMeansDistanceFunctor.

#include "vtkType.h"

class vtkVariantArray;
class vtkAbstractArray;
class vtkTable;

class vtkKMeansDistanceFunctor
{
public:
  vtkKMeansDistanceFunctor(); 
  virtual ~vtkKMeansDistanceFunctor();

  // Description:
  // Return an empty tuple. These values are used as cluster center coordinates
  // when no initial cluster centers are specified.
  virtual vtkVariantArray* GetEmptyTuple( vtkIdType dimension ) = 0;

  // Description:
  // Compute the distance from one observation to another, returning the distance
  // in the first argument.
  virtual void operator() ( double&, vtkVariantArray*, vtkVariantArray * ) = 0;

  // Description:
  // This is called once per observation per run per iteration in order to assign the
  // observation to its nearest cluster center after the distance functor has been
  // evaluated for all the cluster centers.
  //
  // The distance functor is responsible for incrementally updating the cluster centers
  // to account for the assignment.
  virtual void PairwiseUpdate( vtkTable* clusterCenters, vtkIdType row, vtkVariantArray* data, vtkIdType dataCardinality, vtkIdType totalCardinality ) = 0;

  // Description:
  // When a cluster center (1) has no observations that are closer to it than other cluster centers
  // or (2) has exactly the same coordinates as another cluster center, its coordinates should be
  // perturbed. This function should perform that perturbation.
  //
  // Since perturbation relies on a distance metric, this function is the responsibility of the
  // distance functor.
  virtual void PerturbElement( vtkTable*, vtkTable*, vtkIdType, vtkIdType, vtkIdType, double ) = 0;

  // Description:
  // Allocate an array large enough to hold \a size coordinates and return a void pointer to this array.
  // This is used by vtkPKMeansStatistics to send (receive) cluster center coordinates to (from) other processes.
  virtual void* AllocateElementArray( vtkIdType size ) = 0;

  // Description:
  // Free an array allocated with AllocateElementArray.
  virtual void DeallocateElementArray( void* ) = 0;

  // Description:
  // Return a vtkAbstractArray capable of holding cluster center coordinates.
  // This is used by vtkPKMeansStatistics to hold cluster center coordinates sent to (received from) other processes.
  virtual vtkAbstractArray*  GetNewVTKArray() = 0;

  // Description:
  // Pack the cluster center coordinates in \a vElements into columns of \a curTable.
  // This code may assume that the columns in \a curTable are all of the type returned by \a GetNewVTKArray().
  virtual void PackElements( vtkTable* curTable, void* vElements ) = 0;

  // Description:
  // Unpack the cluster center coordinates in \a vElements into columns of \a curTable.
  // This code may assume that the columns in \a curTable are all of the type returned by \a GetNewVTKArray().
  virtual void UnPackElements( vtkTable* curTable, vtkTable* newTable, void* vLocalElements, void* vGlobalElements, int np ) = 0;
  virtual void UnPackElements( vtkTable* curTable, void* vLocalElements, vtkIdType numRows, vtkIdType numCols ) = 0;

  // Description:
  // Return the data type used to store cluster center coordinates.
  virtual int GetDataType() = 0;

protected:
  vtkVariantArray* EmptyTuple; // Used to quickly initialize Tuple for each datum
};

class vtkKMeansDefaultDistanceFunctor: public vtkKMeansDistanceFunctor
{
public:
  static vtkKMeansDefaultDistanceFunctor* New();
  virtual vtkVariantArray* GetEmptyTuple( vtkIdType dimension );
  virtual void operator()( double&, vtkVariantArray*, vtkVariantArray* );
  virtual void PairwiseUpdate( vtkTable*, vtkIdType, vtkVariantArray*, vtkIdType, vtkIdType );
  virtual void PerturbElement( vtkTable*, vtkTable*, vtkIdType, vtkIdType, vtkIdType, double );
  virtual void* AllocateElementArray( vtkIdType size );
  virtual void DeallocateElementArray( void* );
  virtual vtkAbstractArray*  GetNewVTKArray();
  virtual void PackElements( vtkTable* curTable, void* vElements );
  virtual void UnPackElements( vtkTable* curTable, vtkTable* newTable, void* vLocalElements, void* vGlobalElements, int np );
  virtual void UnPackElements( vtkTable* curTable, void* vLocalElements, vtkIdType numRows, vtkIdType numCols );
  virtual int GetDataType();
};

#endif // __vtkKMeansDistanceFunctor_h
