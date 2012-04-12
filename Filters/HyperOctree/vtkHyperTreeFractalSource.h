/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeFractalSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperTreeFractalSource - Create a hyper tree grid from a fractal.
// hyperTree
// .SECTION Description
//
// .SECTION See Also
// vtkHyperTreeSampleFunction

#ifndef __vtkHyperTreeFractalSource_h
#define __vtkHyperTreeFractalSource_h

#include "vtkFiltersHyperOctreeModule.h" // For export macro
#include "vtkObject.h"

class vtkImplicitFunction;
class vtkHyperTreeGrid;
class vtkHyperTreeCursor;

class VTKFILTERSHYPEROCTREE_EXPORT vtkHyperTreeFractalSource : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeFractalSource,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  static vtkHyperTreeFractalSource *New();

  vtkSetVector3Macro(GridSize, int);
  vtkGetVector3Macro(GridSize, int);

  vtkSetMacro(MaximumLevel, int);
  vtkGetMacro(MaximumLevel, int);

  vtkSetMacro(Dimension, int);
  vtkGetMacro(Dimension, int);

  vtkSetMacro(AxisBranchFactor, int);
  vtkGetMacro(AxisBranchFactor, int);

  vtkSetMacro(Dual, int);
  vtkGetMacro(Dual, int);
  vtkBooleanMacro(Dual, int);

  // Bypass the pipeline
  vtkHyperTreeGrid* NewHyperTreeGrid();

protected:
  vtkHyperTreeFractalSource();
  ~vtkHyperTreeFractalSource();

  void Subdivide( vtkHyperTreeCursor* cursor,
                  int level,
                  vtkHyperTreeGrid* output,
                  int index,
                  int idx[3],
                  int offset );

  int GridSize[3];
  int MaximumLevel;
  int Dimension;
  int AxisBranchFactor;

  int Dual;

private:
  vtkHyperTreeFractalSource(const vtkHyperTreeFractalSource&);  // Not implemented.
  void operator=(const vtkHyperTreeFractalSource&);  // Not implemented.
};

#endif
