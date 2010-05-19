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
// its input dataset. The dataset is given on its first input port. 
// The subset is described by the contents of the vtkSelection on its 
// second input port. Depending on the content of the vtkSelection,
// this will use either a vtkExtractSelectedIds, vtkExtractSelectedFrustum
// vtkExtractSelectedLocations or a vtkExtractSelectedThreshold to perform
// the extraction.
// .SECTION See Also
// vtkSelection vtkExtractSelectedIds vtkExtractSelectedFrustum
// vtkExtractSelectedLocations vtkExtractSelectedThresholds

#ifndef __vtkExtractSelection_h
#define __vtkExtractSelection_h

#include "vtkExtractSelectionBase.h"

class vtkExtractSelectedBlock;
class vtkExtractSelectedFrustum;
class vtkExtractSelectedIds;
class vtkExtractSelectedLocations;
class vtkExtractSelectedRows;
class vtkExtractSelectedThresholds;
class vtkProbeSelectedLocations;
class vtkSelection;
class vtkSelectionNode;

class VTK_GRAPHICS_EXPORT vtkExtractSelection : public vtkExtractSelectionBase
{
public:
  static vtkExtractSelection *New();
  vtkTypeMacro(vtkExtractSelection, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When On, this returns an unstructured grid that outlines selection area.
  // Off is the default. Applicable only to Frustum selection extraction.
  vtkSetMacro(ShowBounds,int);
  vtkGetMacro(ShowBounds,int);
  vtkBooleanMacro(ShowBounds,int);

  // Description:
  // When On, vtkProbeSelectedLocations is used for extracting selections of
  // content type vtkSelection::LOCATIONS. Default is off and then
  // vtkExtractSelectedLocations is used.
  vtkSetMacro(UseProbeForLocations, int);
  vtkGetMacro(UseProbeForLocations, int);
  vtkBooleanMacro(UseProbeForLocations, int);

protected:
  vtkExtractSelection();
  ~vtkExtractSelection();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  //sets up empty output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  // runs the algorithm and fills the output with results
  virtual int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  // used for composite, non-hierarhical input.
  vtkDataObject* RequestDataInternal(
    unsigned int composite_index,
    vtkDataObject* non_composite_input, vtkSelection* sel,
    vtkInformation* outInfo);

  // Used for hierarchical input.
  vtkDataObject* RequestDataInternal(
    unsigned int composite_index,
    unsigned int level,
    unsigned int index,
    vtkDataObject* non_composite_input, vtkSelection* sel,
    vtkInformation* outInfo);


  // called for non-composite input or for a block in a composite dataset.
  vtkDataObject* RequestDataFromBlock(vtkDataObject* input, 
    vtkSelectionNode* sel, vtkInformation* outInfo);

  vtkExtractSelectedBlock* BlockFilter;
  vtkExtractSelectedFrustum* FrustumFilter;
  vtkExtractSelectedIds* IdsFilter;
  vtkExtractSelectedLocations* LocationsFilter;
  vtkExtractSelectedRows* RowsFilter;
  vtkExtractSelectedThresholds* ThresholdsFilter;
  vtkProbeSelectedLocations* ProbeFilter;

  int UseProbeForLocations;
  int ShowBounds;
private:
  vtkExtractSelection(const vtkExtractSelection&);  // Not implemented.
  void operator=(const vtkExtractSelection&);  // Not implemented.
};

#endif
