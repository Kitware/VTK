/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedThresholds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectedThresholds - extract a cells or points from a 
// dataset that have values within a set of thresholds.

// .SECTION Description
// vtkExtractSelectedThresholds extracts all cells and points with attribute 
// values that lie within a vtkSelection's THRESHOLD contents. The selecion
// can specify to threshold a particular array within either the point or cell
// attribute data of the input. This is similar to vtkThreshold
// but allows mutliple thresholds ranges.
// This filter adds a scalar array called vtkOriginalCellIds that says what 
// input cell produced each output cell. This is an example of a Pedigree ID 
// which helps to trace back results.

// .SECTION See Also
// vtkSelection vtkExtractSelection vtkThreshold 

#ifndef __vtkExtractSelectedThresholds_h
#define __vtkExtractSelectedThresholds_h

#include "vtkExtractSelectionBase.h"

class vtkDataArray;
class vtkDoubleArray;
class vtkSelection;
class vtkSelectionNode;

class VTK_GRAPHICS_EXPORT vtkExtractSelectedThresholds : public vtkExtractSelectionBase
{
public:
  vtkTypeRevisionMacro(vtkExtractSelectedThresholds, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkExtractSelectedThresholds *New();

  // Description:
  // Function for determining whether a value in a data array passes
  // the threshold test(s) provided in lims.  Returns 1 if the value
  // passes at least one of the threshold tests.
  static int EvaluateValue(vtkDataArray *scalars, vtkIdType id, vtkDoubleArray *lims);

  // Description:
  // Function for determining whether a value in a data array passes
  // the threshold test(s) provided in lims.  Returns 1 if the value
  // passes at least one of the threshold tests.  Also returns in
  // AboveCount, BelowCount and InsideCount the number of tests where
  // the value was above, below or inside the interval.
  static int EvaluateValue(vtkDataArray *scalars, vtkIdType id, vtkDoubleArray *lims, int *AboveCount, int *BelowCount, int *InsideCount);

protected:
  vtkExtractSelectedThresholds();
  ~vtkExtractSelectedThresholds();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  int ExtractCells(vtkSelectionNode *sel, vtkDataSet *input, 
                   vtkDataSet *output,
                   int usePointScalars);
  int ExtractPoints(vtkSelectionNode *sel, vtkDataSet *input, 
                    vtkDataSet *output);

private:
  vtkExtractSelectedThresholds(const vtkExtractSelectedThresholds&);  // Not implemented.
  void operator=(const vtkExtractSelectedThresholds&);  // Not implemented.
};

#endif
