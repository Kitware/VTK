/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxContour.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxContour - generate isosurfaces/isolines from scalar values
// .SECTION Description
// This filter uses vtkContourFilter to generate isosurfaces/isolines from
// scalar values

// .SECTION See Also
// vtkContourFilter

#ifndef __vtkHierarchicalBoxContour_h
#define __vtkHierarchicalBoxContour_h

#include "vtkHierarchicalBoxToPolyDataFilter.h"

class vtkContourFilter;
class vtkDataObject;

class VTK_GRAPHICS_EXPORT vtkHierarchicalBoxContour : public vtkHierarchicalBoxToPolyDataFilter
{
public:
  static vtkHierarchicalBoxContour *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxContour,
                       vtkHierarchicalBoxToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Delegated to contour
  void SetValue(int i, double value);
  double GetValue(int i);
  double *GetValues();
  void GetValues(double *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  const char* GetInputScalarsSelection();
  void SelectInputScalars(const char *fieldName);
  unsigned long GetMTime();

protected:
  vtkHierarchicalBoxContour();
  ~vtkHierarchicalBoxContour();

  virtual void ExecuteData(vtkDataObject*);

  vtkContourFilter* Contour;

private:
  vtkHierarchicalBoxContour(const vtkHierarchicalBoxContour&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxContour&);  // Not implemented.
};


#endif



