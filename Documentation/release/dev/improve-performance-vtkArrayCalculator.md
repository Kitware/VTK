## Improve vtkArrayCalculator's Performance

The function parsers used by vtkArrayCalculator now don't call Modified every time SetScalarValue/SetVectorValue is
called, because when many threads are used, Modified is a communication overhead. Even when only one thread is
used calling Modified very often is still a performance overhead. vtkArrayCalculator's functor now saves the arrays'
size since they are constantly required. Also, instead of using 3 GetComponent, now only 1 GetTuple is used with a
pre-allocate a temp tuple. Finally, Dispatch is used on the ResultArray instead of using the vtkDataArray::SetTuple.
