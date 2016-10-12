/*=========================================================================
This source has no copyright.  It is intended to be copied by users
wishing to create their own VTK classes locally.
=========================================================================*/
/**
 * @class   vtkLocalExample
 * @brief   Example class using VTK.
 *
 * vtkLocalExample is a simple class that uses VTK.  This class can be
 * copied and modified to produce your own classes.
*/

#ifndef vtkLocalExample_h
#define vtkLocalExample_h

#include "vtkLocalExampleModule.h" // export macro
#include "vtkObject.h"

class VTKLOCALEXAMPLE_EXPORT vtkLocalExample : public vtkObject
{
public:
  static vtkLocalExample* New();
  vtkTypeMacro(vtkLocalExample, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkLocalExample();
  ~vtkLocalExample();

private:
  vtkLocalExample(const vtkLocalExample&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLocalExample&) VTK_DELETE_FUNCTION;
};

#endif
