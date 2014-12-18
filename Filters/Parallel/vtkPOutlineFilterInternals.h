/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOutlineFilterInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPOutlineFilterInternals - create wireframe outline (or corners) for arbitrary data set
// .SECTION Description
// vtkPOutlineFilterInternals has common code for vtkOutlineFilter and
// vtkOutlineCornerFilter. It assumes the filter is operated in a data parallel
// pipeline.

#ifndef vtkPOutlineFilterInternals_h
#define vtkPOutlineFilterInternals_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkBoundingBox.h" //  needed for vtkBoundingBox.
#include <vector> // needed for std::vector

class vtkBoundingBox;
class vtkDataObject;
class vtkDataObjectTree;
class vtkDataSet;
class vtkInformation;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkOverlappingAMR;
class vtkPolyData;
class vtkUniformGridAMR;

class VTKFILTERSPARALLEL_EXPORT vtkPOutlineFilterInternals
{
public:

  vtkPOutlineFilterInternals();
  virtual ~vtkPOutlineFilterInternals();
  void SetController(vtkMultiProcessController*);
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);
  void SetCornerFactor(double cornerFactor);
  void SetIsCornerSource(bool value);

private:

  int RequestData(vtkOverlappingAMR* amr, vtkPolyData* output);
  int RequestData(vtkUniformGridAMR* amr, vtkPolyData* output);
  int RequestData(vtkDataObjectTree* cd, vtkPolyData* output);
  int RequestData(vtkDataSet* ds, vtkPolyData* output);


  void CollectCompositeBounds(vtkDataObject* input);

  std::vector<vtkBoundingBox> BoundsList;
  vtkMultiProcessController* Controller;

  bool IsCornerSource;
  double CornerFactor;
};

#endif
// VTK-HeaderTest-Exclude: vtkPOutlineFilterInternals.h
