/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelection - extract a subset from a vtkDataSet.
// .SECTION Description
// vtkExtractSelection extracts some subset of cells and points from
// its input dataset. The subset is described by the contents of the
// vtkSelection on its first input port. The dataset is given on its 
// second input port. Depending on the content of the vtkSelection,
// this will use either a vtkExtractSelectedIds, vtkExtractSelectedFrustum
// vtkExtractSelectedPoints or a vtkExtractSelectedThreshold to perform
// the extraction.
// .SECTION See Also
// vtkSelection vtkExtractSelectedIds vtkExtractSelectedFrustum
// vtkExtractSelectedPoints vtkExtractSelectedThresholds

#ifndef __vtkExtractSelection_h
#define __vtkExtractSelection_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkExtractSelectedIds;
class vtkExtractSelectedFrustum;
class vtkExtractSelectedPoints;
class vtkExtractSelectedThresholds;
class vtkSelection;

class VTK_GRAPHICS_EXPORT vtkExtractSelection : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelection,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with NULL extractfilter
  static vtkExtractSelection *New();

protected:
  vtkExtractSelection();
  ~vtkExtractSelection();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);


  int ExtractCellIds(vtkSelection *s, vtkDataSet *i, vtkUnstructuredGrid *o);
  int ExtractFrustum(vtkSelection *s, vtkDataSet *i, vtkUnstructuredGrid *o);
  int ExtractPoints(vtkSelection *s, vtkDataSet *i, vtkUnstructuredGrid *o);
  int ExtractThresholds(vtkSelection *s, vtkDataSet *i, vtkUnstructuredGrid *o);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkExtractSelectedIds* IdsFilter;
  vtkExtractSelectedFrustum* FrustumFilter;
  vtkExtractSelectedPoints* PointsFilter;
  vtkExtractSelectedThresholds* ThresholdsFilter;

private:
  vtkExtractSelection(const vtkExtractSelection&);  // Not implemented.
  void operator=(const vtkExtractSelection&);  // Not implemented.
};

#endif
