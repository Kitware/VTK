#ifndef __vtkKMeansDistanceFunctor_h
#define __vtkKMeansDistanceFunctor_h

#include "vtkType.h"
class vtkVariantArray;
class vtkTable;

class vtkKMeansDistanceFunctor
{
protected:
  vtkVariantArray* EmptyTuple; // Used to quickly initialize Tuple for each datum
public:
  vtkKMeansDistanceFunctor(); 
  virtual ~vtkKMeansDistanceFunctor();
  virtual vtkVariantArray* GetEmptyTuple( vtkIdType dimension ) = 0;
  virtual void operator() ( double&, vtkVariantArray*, vtkVariantArray * ) = 0;
  virtual void PairwiseUpdate( vtkTable*, vtkIdType, vtkVariantArray *, vtkIdType, vtkIdType ) = 0;
  virtual void PerturbElement( vtkTable*, vtkTable*, vtkIdType, vtkIdType, vtkIdType, double) = 0;
};

class vtkKMeansDefaultDistanceFunctor: public vtkKMeansDistanceFunctor
{
public:
  static vtkKMeansDefaultDistanceFunctor* New();
  virtual vtkVariantArray* GetEmptyTuple( vtkIdType dimension );
  virtual void operator()( double&, vtkVariantArray*, vtkVariantArray* );
  virtual void PairwiseUpdate( vtkTable*, vtkIdType, vtkVariantArray*, vtkIdType, vtkIdType );
  virtual void PerturbElement( vtkTable*, vtkTable*, vtkIdType, vtkIdType, vtkIdType, double);
};

#endif // __vtkKMeansDistanceFunctor_h
