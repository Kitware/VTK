#ifndef __vtkKMeansDistanceFunctorCalculator_h
#define __vtkKMeansDistanceFunctorCalculator_h

// .NAME vtkKMeansDistanceFunctorCalculator - measure distance from k-means cluster centers using a user-specified expression
// .SECTION Description
// This is a subclass of the default k-means distance functor that allows
// the user to specify a distance function as a string. The provided
// expression is evaluated whenever the parenthesis operator is invoked
// but this is much slower than the default distance calculation.
//
// User-specified distance expressions should be written in terms of
// two vector variables named "x" and "y".
// The length of the vectors will be determined by the k-means request
// and all columns of interest in the request must contain values that
// may be converted to a floating point representation. (Strings and
// vtkObject pointers are not allowed.)
// An example distance expression is "sqrt( (x0-y0)^2 + (x1-y1)^2 )"
// which computes Euclidian distance in a plane defined by the first
// 2 coordinates of the vectors specified.

#include "vtkKMeansDistanceFunctor.h"

class vtkFunctionParser;
class vtkDoubleArray;

class VTK_INFOVIS_EXPORT vtkKMeansDistanceFunctorCalculator : public vtkKMeansDistanceFunctor
{
public:
  static vtkKMeansDistanceFunctorCalculator* New();
  vtkTypeMacro(vtkKMeansDistanceFunctorCalculator,vtkKMeansDistanceFunctor);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Compute the distance from one observation to another, returning the distance
  // in the first argument.
  virtual void operator() ( double&, vtkVariantArray*, vtkVariantArray * );

  // Description:
  // Set/get the distance function expression.
  vtkSetStringMacro(DistanceExpression);
  vtkGetStringMacro(DistanceExpression);

  // Description:
  // Set/get the string containing an expression which evaluates to the
  // distance metric used for k-means computation. The scalar variables
  // "x0", "x1", ... "xn" and "y0", "y1", ..., "yn" refer to the coordinates
  // involved in the computation.
  virtual void SetFunctionParser( vtkFunctionParser* );
  vtkGetObjectMacro(FunctionParser,vtkFunctionParser);

protected:
  vtkKMeansDistanceFunctorCalculator();
  virtual ~vtkKMeansDistanceFunctorCalculator();

  char* DistanceExpression;
  int TupleSize;
  vtkFunctionParser* FunctionParser;

private:
  vtkKMeansDistanceFunctorCalculator( const vtkKMeansDistanceFunctorCalculator& ); // Not implemented.
  void operator = ( const vtkKMeansDistanceFunctorCalculator& ); // Not implemented.
};

#endif // __vtkKMeansDistanceFunctorCalculator_h
