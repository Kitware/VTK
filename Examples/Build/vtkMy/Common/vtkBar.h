/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBar.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/
// .NAME vtkBar - Bar class for vtk
// .SECTION Description
// None.

#ifndef __vtkBar_h
#define __vtkBar_h

#include "vtkObject.h"
#include "vtkmyCommonWin32Header.h"

class VTK_MY_COMMON_EXPORT vtkBar : public vtkObject
{
public:
  static vtkBar *New();
  vtkTypeRevisionMacro(vtkBar,vtkObject);

protected:
  vtkBar() {};
  ~vtkBar() {};
private:
  vtkBar(const vtkBar&);  // Not implemented.
  void operator=(const vtkBar&);  // Not implemented.
};

#endif 
