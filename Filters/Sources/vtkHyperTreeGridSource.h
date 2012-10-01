/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperTreeGridSource - Create a synthetic grid of hypertrees.
//
// .SECTION Description
//
// .SECTION See Also
// vtkHyperTreeGridSampleFunction
//
// .SECTION Thanks
// This class was written by Philippe Pebay, Kitware SAS 2012

#ifndef __vtkHyperTreeGridSource_h
#define __vtkHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkDataArray;
class vtkImplicitFunction;

class VTKFILTERSSOURCES_EXPORT vtkHyperTreeGridSource : public vtkHyperTreeGridAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridSource,vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHyperTreeGridSource *New();

  // Description:
  // Return the maximum number of levels of the hyperoctree.
  // \post positive_result: result>=1
  int GetMaximumLevel();

  // Description:
  // Set the maximum number of levels of the hyperoctree. If
  // GetMinLevels()>=levels, GetMinLevels() is changed to levels-1.
  // \pre positive_levels: levels>=1
  // \post is_set: this->GetLevels()==levels
  // \post min_is_valid: this->GetMinLevels()<this->GetLevels()
  void SetMaximumLevel(int levels);

  // Description:
  // Return the minimal number of levels of systematic subdivision.
  // \post positive_result: result>=0
  void SetMinimumLevel(int level);
  int GetMinimumLevel();

  vtkSetVector3Macro(GridSize, int);
  vtkGetVector3Macro(GridSize, int);

  vtkSetMacro(AxisBranchFactor, int);
  vtkGetMacro(AxisBranchFactor, int);

  vtkSetMacro(Dual, int);
  vtkGetMacro(Dual, int);
  vtkBooleanMacro(Dual, int);

  // Description:
  // Create a 2D or 3D fractal
  vtkSetClampMacro(Dimension, int, 2, 3);
  vtkGetMacro(Dimension, int);

protected:
  vtkHyperTreeGridSource();
  ~vtkHyperTreeGridSource();

  int RequestInformation (vtkInformation * vtkNotUsed(request),
                          vtkInformationVector ** vtkNotUsed( inputVector ),
                          vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  void Subdivide( vtkHyperTreeCursor* cursor,
                  int level,
                  vtkHyperTreeGrid* output,
                  int index,
                  int idx[3],
                  int offset );

  int GridSize[3];
  int MaximumLevel;
  int MinimumLevel;
  int Dimension;
  int AxisBranchFactor;

  int Dual;

  vtkDataArray *XCoordinates;
  vtkDataArray *YCoordinates;
  vtkDataArray *ZCoordinates;

private:
  vtkHyperTreeGridSource(const vtkHyperTreeGridSource&);  // Not implemented.
  void operator=(const vtkHyperTreeGridSource&);  // Not implemented.
};

#endif
