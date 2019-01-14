/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotationLink.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAnnotationLink
 * @brief   An algorithm for linking annotations among objects
 *
 * vtkAnnotationLink is a simple source filter which outputs the
 * vtkAnnotationLayers object stored internally.  Multiple objects may share
 * the same annotation link filter and connect it to an internal pipeline so
 * that if one object changes the annotation set, it will be pulled into all
 * the other objects when their pipelines update.
 *
 * The shared vtkAnnotationLayers object (a collection of annotations) is
 * shallow copied to output port 0.
 *
 * vtkAnnotationLink can also store a set of domain maps. A domain map is
 * simply a table associating values between domains. The domain of each
 * column is defined by the array name of the column. The domain maps are
 * sent to a multi-block dataset in output port 1.
 *
 * Output ports 0 and 1 can be set as input ports 0 and 1 to
 * vtkConvertSelectionDomain, which can use the domain maps to convert the
 * domains of selections in the vtkAnnotationLayers to match a particular
 * data object (set as port 2 on vtkConvertSelectionDomain).
 *
 * The shared vtkAnnotationLayers object also stores a "current selection"
 * normally interpreted as the interactive selection of an application.
 * As a convenience, this selection is sent to output port 2 so that it
 * can be connected to pipelines requiring a vtkSelection.
*/

#ifndef vtkAnnotationLink_h
#define vtkAnnotationLink_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkAnnotationLayersAlgorithm.h"

class vtkCommand;
class vtkDataObjectCollection;
class vtkInformation;
class vtkInformationVector;
class vtkSelection;
class vtkTable;

class VTKFILTERSGENERAL_EXPORT vtkAnnotationLink : public vtkAnnotationLayersAlgorithm
{
public:
  static vtkAnnotationLink *New();
  vtkTypeMacro(vtkAnnotationLink, vtkAnnotationLayersAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The annotations to be shared.
   */
  vtkGetObjectMacro(AnnotationLayers, vtkAnnotationLayers);
  virtual void SetAnnotationLayers(vtkAnnotationLayers* layers);
  //@}

  //@{
  /**
   * Set or get the current selection in the annotation layers.
   */
  virtual void SetCurrentSelection(vtkSelection* sel);
  virtual vtkSelection* GetCurrentSelection();
  //@}

  //@{
  /**
   * The domain mappings.
   */
  void AddDomainMap(vtkTable* map);
  void RemoveDomainMap(vtkTable* map);
  void RemoveAllDomainMaps();
  int GetNumberOfDomainMaps();
  vtkTable* GetDomainMap(int i);
  //@}

  /**
   * Get the mtime of this object.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkAnnotationLink();
  ~vtkAnnotationLink() override;

  /**
   * Called to process modified events from its vtkAnnotationLayers.
   */
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId,
    void* callData);

  /**
   * Set up input ports.
   */
  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * Set up output ports.
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Copy the data to the output objects.
   */
  void ShallowCopyToOutput(
    vtkAnnotationLayers* input,
    vtkAnnotationLayers* output,
    vtkSelection* sel);

  /**
   * Shallow copy the internal selection to the output.
   */
  int RequestData(
    vtkInformation *info,
    vtkInformationVector **inVector,
    vtkInformationVector *outVector) override;

  /**
   * The shared selection.
   */
  vtkAnnotationLayers* AnnotationLayers;

  /**
   * The mappings between domains.
   */
  vtkDataObjectCollection* DomainMaps;


private:
  vtkAnnotationLink(const vtkAnnotationLink&) = delete;
  void operator=(const vtkAnnotationLink&) = delete;

  class Command;
  friend class Command;
  Command* Observer;

};

#endif
