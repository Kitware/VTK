/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectionLegacy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSelectionLegacy
 * @brief   extract a subset from a vtkDataSet.
 *
 * vtkExtractSelectionLegacy extracts some subset of cells and points from
 * its input dataset. The dataset is given on its first input port.
 * The subset is described by the contents of the vtkSelection on its
 * second input port. Depending on the content of the vtkSelection,
 * this will use either a vtkExtractSelectedIds, vtkExtractSelectedFrustum
 * vtkExtractSelectedLocations or a vtkExtractSelectedThreshold to perform
 * the extraction.
 *
 * This class used to be called vtkExtractSelection but it was replaced in
 * VTK 8.2.  This is only provided for backwards compatibility and is therefore
 * deprecated.
 *
 * @sa
 * vtkSelection vtkExtractSelectedIds vtkExtractSelectedFrustum
 * vtkExtractSelectedLocations vtkExtractSelectedThresholds
*/

#ifndef vtkExtractSelectionLegacy_h
#define vtkExtractSelectionLegacy_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkExtractSelectionBase.h"

#ifndef VTK_LEGACY_REMOVE

class vtkExtractSelectedBlock;
class vtkExtractSelectedFrustum;
class vtkExtractSelectedIds;
class vtkExtractSelectedLocations;
class vtkExtractSelectedRows;
class vtkExtractSelectedThresholds;
class vtkProbeSelectedLocations;
class vtkSelection;
class vtkSelectionNode;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectionLegacy : public vtkExtractSelectionBase
{
public:
  static vtkExtractSelectionLegacy *New();
  vtkTypeMacro(vtkExtractSelectionLegacy, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * When On, this returns an unstructured grid that outlines selection area.
   * Off is the default. Applicable only to Frustum selection extraction.
   */
  vtkSetMacro(ShowBounds,vtkTypeBool);
  vtkGetMacro(ShowBounds,vtkTypeBool);
  vtkBooleanMacro(ShowBounds,vtkTypeBool);
  //@}

  //@{
  /**
   * When On, vtkProbeSelectedLocations is used for extracting selections of
   * content type vtkSelection::LOCATIONS. Default is off and then
   * vtkExtractSelectedLocations is used.
   */
  vtkSetMacro(UseProbeForLocations, vtkTypeBool);
  vtkGetMacro(UseProbeForLocations, vtkTypeBool);
  vtkBooleanMacro(UseProbeForLocations, vtkTypeBool);
  //@}

protected:
  vtkExtractSelectionLegacy();
  ~vtkExtractSelectionLegacy() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  //sets up empty output dataset
  int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) override;

  // runs the algorithm and fills the output with results
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) override;

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

  vtkTypeBool UseProbeForLocations;
  vtkTypeBool ShowBounds;
private:
  vtkExtractSelectionLegacy(const vtkExtractSelectionLegacy&) = delete;
  void operator=(const vtkExtractSelectionLegacy&) = delete;
};

#endif // VTK_LEGACY_REMOVE

#endif
