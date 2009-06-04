#ifndef __vtkKMeansDistanceFunctor_h
#define __vtkKMeansDistanceFunctor_h

#include "vtkKMeansStatistics.h"
#include <vtkstd/vector>

class vtkKMeansDistanceFunctor
{
protected:
  vtkVariantArray* EmptyTuple; // Used to quickly initialize Tuple for each datum
public:
  vtkKMeansDistanceFunctor(); 
  virtual ~vtkKMeansDistanceFunctor();
  virtual vtkVariantArray* GetEmptyTuple( int dimension ) = 0;
  virtual void operator() ( double&, vtkVariantArray*, vtkVariantArray * ) = 0;
  virtual void IncrementallyUpdate( vtkTable*, int, vtkVariantArray *, int ) = 0;
  virtual void PerturbElement( vtkTable*, vtkTable*, int, vtkIdType, vtkIdType, double) = 0;
};

class vtkKMeansDefaultDistanceFunctor: public vtkKMeansDistanceFunctor
{
public:
  static vtkKMeansDefaultDistanceFunctor* New();
  virtual vtkVariantArray* GetEmptyTuple( int dimension );
  virtual void operator()( double&, vtkVariantArray*, vtkVariantArray* );
  virtual void IncrementallyUpdate( vtkTable*, int, vtkVariantArray*, int );
  virtual void PerturbElement( vtkTable*, vtkTable*, int, vtkIdType, vtkIdType, double);
};

#endif // __vtkKMeansDistanceFunctor_h
