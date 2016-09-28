#ifndef vtkKMeansDistanceFunctorCalculator_h
#define vtkKMeansDistanceFunctorCalculator_h

/**
 * @class   vtkKMeansDistanceFunctorCalculator
 * @brief   measure distance from k-means cluster centers using a user-specified expression
 *
 * This is a subclass of the default k-means distance functor that allows
 * the user to specify a distance function as a string. The provided
 * expression is evaluated whenever the parenthesis operator is invoked
 * but this is much slower than the default distance calculation.
 *
 * User-specified distance expressions should be written in terms of
 * two vector variables named "x" and "y".
 * The length of the vectors will be determined by the k-means request
 * and all columns of interest in the request must contain values that
 * may be converted to a floating point representation. (Strings and
 * vtkObject pointers are not allowed.)
 * An example distance expression is "sqrt( (x0-y0)^2 + (x1-y1)^2 )"
 * which computes Euclidian distance in a plane defined by the first
 * 2 coordinates of the vectors specified.
*/

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkKMeansDistanceFunctor.h"

class vtkFunctionParser;
class vtkDoubleArray;

class VTKFILTERSSTATISTICS_EXPORT vtkKMeansDistanceFunctorCalculator : public vtkKMeansDistanceFunctor
{
public:
  static vtkKMeansDistanceFunctorCalculator* New();
  vtkTypeMacro(vtkKMeansDistanceFunctorCalculator,vtkKMeansDistanceFunctor);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  /**
   * Compute the distance from one observation to another, returning the distance
   * in the first argument.
   */
  void operator() ( double&, vtkVariantArray*, vtkVariantArray * ) VTK_OVERRIDE;

  //@{
  /**
   * Set/get the distance function expression.
   */
  vtkSetStringMacro(DistanceExpression);
  vtkGetStringMacro(DistanceExpression);
  //@}

  //@{
  /**
   * Set/get the string containing an expression which evaluates to the
   * distance metric used for k-means computation. The scalar variables
   * "x0", "x1", ... "xn" and "y0", "y1", ..., "yn" refer to the coordinates
   * involved in the computation.
   */
  virtual void SetFunctionParser( vtkFunctionParser* );
  vtkGetObjectMacro(FunctionParser,vtkFunctionParser);
  //@}

protected:
  vtkKMeansDistanceFunctorCalculator();
  ~vtkKMeansDistanceFunctorCalculator() VTK_OVERRIDE;

  char* DistanceExpression;
  int TupleSize;
  vtkFunctionParser* FunctionParser;

private:
  vtkKMeansDistanceFunctorCalculator( const vtkKMeansDistanceFunctorCalculator& ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkKMeansDistanceFunctorCalculator& ) VTK_DELETE_FUNCTION;
};

#endif // vtkKMeansDistanceFunctorCalculator_h
