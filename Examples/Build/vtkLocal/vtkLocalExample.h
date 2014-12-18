/*=========================================================================
This source has no copyright.  It is intended to be copied by users
wishing to create their own VTK classes locally.
=========================================================================*/
// .NAME vtkLocalExample - Example class using VTK.
// .SECTION Description
// vtkLocalExample is a simple class that uses VTK.  This class can be
// copied and modified to produce your own classes.

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
  vtkLocalExample(const vtkLocalExample&);  // Not implemented.
  void operator=(const vtkLocalExample&);  // Not implemented.
};

#endif
