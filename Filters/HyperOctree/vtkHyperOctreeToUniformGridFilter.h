/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeToUniformGridFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeToUniformGridFilter - Flat the octree into a uniform
// grid
// .SECTION Description
// vtkHyperOctreeToUniformGridFilter creates a uniform grid with a resolution
// based on the number of levels of the hyperoctree. Then, it copies celldata
// in each cell of the uniform grid that belongs to an actual leaf of the
// hyperoctree.
//
// .SECTION See Also
// vtkGeometryFilter vtkStructuredGridGeometryFilter.

#ifndef __vtkHyperOctreeToUniformGridFilter_h
#define __vtkHyperOctreeToUniformGridFilter_h

#include "vtkImageAlgorithm.h"

class vtkHyperOctreeCursor;
class vtkCellData;
class vtkDataSetAttributes;

class VTK_GRAPHICS_EXPORT vtkHyperOctreeToUniformGridFilter : public vtkImageAlgorithm
{
public:
  static vtkHyperOctreeToUniformGridFilter *New();
  vtkTypeMacro(vtkHyperOctreeToUniformGridFilter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkHyperOctreeToUniformGridFilter();
  ~vtkHyperOctreeToUniformGridFilter();
  
  int RequestInformation (vtkInformation * vtkNotUsed(request),
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);
  
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  
  void CopyCellData(int cellExtent[6]);
  
  // Variables used by generate recursively.
  // It avoids to pass to much argument.
  vtkDataSetAttributes *InputCD;
  vtkCellData *OutputCD;
  vtkHyperOctreeCursor *Cursor;
  int YExtent;
  int ZExtent;
  vtkImageData *Output;
  
private:
  vtkHyperOctreeToUniformGridFilter(const vtkHyperOctreeToUniformGridFilter&);  // Not implemented.
  void operator=(const vtkHyperOctreeToUniformGridFilter&);  // Not implemented.
};

#endif
