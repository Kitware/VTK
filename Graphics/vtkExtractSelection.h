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
// vtkExtractSelectedLocations or a vtkExtractSelectedThreshold to perform
// the extraction.
// .SECTION See Also
// vtkSelection vtkExtractSelectedIds vtkExtractSelectedFrustum
// vtkExtractSelectedLocations vtkExtractSelectedThresholds

#ifndef __vtkExtractSelection_h
#define __vtkExtractSelection_h

#include "vtkDataSetAlgorithm.h"

class vtkExtractSelectedIds;
class vtkExtractSelectedFrustum;
class vtkExtractSelectedLocations;
class vtkExtractSelectedThresholds;
class vtkSelection;

class VTK_GRAPHICS_EXPORT vtkExtractSelection : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelection,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with NULL extractfilter
  static vtkExtractSelection *New();

  // Description:
  // Convenience method to specify the selection connection (2nd input
  // port)
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

protected:
  vtkExtractSelection();
  ~vtkExtractSelection();

  //sets up output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);
 
  // Usual data generation method
  virtual int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);


  int ExtractIds(vtkSelection *s, vtkDataSet *i, vtkDataSet *o);
  int ExtractFrustum(vtkSelection *s, vtkDataSet *i, vtkDataSet *o);
  int ExtractLocations(vtkSelection *s, vtkDataSet *i, vtkDataSet *o);
  int ExtractThresholds(vtkSelection *s, vtkDataSet *i, vtkDataSet *o);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkExtractSelectedIds* IdsFilter;
  vtkExtractSelectedFrustum* FrustumFilter;
  vtkExtractSelectedLocations* LocationsFilter;
  vtkExtractSelectedThresholds* ThresholdsFilter;

private:
  vtkExtractSelection(const vtkExtractSelection&);  // Not implemented.
  void operator=(const vtkExtractSelection&);  // Not implemented.
};

#endif
