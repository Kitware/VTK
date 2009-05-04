/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionLink.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectionLink - An algorithm for linking selections among objects
// .SECTION Description
// vtkSelectionLink is a simple source filter which outputs the selection
// object stored internally.  Multiple objects may share
// the same selection link filter and connect it to an internal pipeline so
// that if one object changes the selection, it will be pulled into all
// the other objects when their pipelines update.

#ifndef __vtkSelectionLink_h
#define __vtkSelectionLink_h

#include "vtkSelectionAlgorithm.h"

class vtkDataObjectCollection;
class vtkInformation;
class vtkInformationVector;
class vtkSelection;
class vtkTable;

class VTK_GRAPHICS_EXPORT vtkSelectionLink : public vtkSelectionAlgorithm
{
public:
  static vtkSelectionLink *New();
  vtkTypeRevisionMacro(vtkSelectionLink, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The selection to be shared.
  vtkGetObjectMacro(Selection, vtkSelection);
  void SetSelection(vtkSelection* selection);
  
  // Description:
  // The domain mappings.
  void AddDomainMap(vtkTable* map);
  void RemoveDomainMap(vtkTable* map);
  void RemoveAllDomainMaps();
  int GetNumberOfDomainMaps();
  vtkTable* GetDomainMap(int i);

  // Description:
  // Get the mtime of this object.
  virtual unsigned long GetMTime();
  
protected:
  vtkSelectionLink();
  ~vtkSelectionLink();
  
  // Description:
  // Shallow copy the internal selection to the output.
  virtual int RequestData(
    vtkInformation *info,
    vtkInformationVector **inVector,
    vtkInformationVector *outVector);

  // Description:
  // Set up output ports.
  virtual int FillOutputPortInformation(int, vtkInformation*);
  
  // Description:
  // The shared selection.
  vtkSelection* Selection;

  // Description:
  // The mappings between domains.
  vtkDataObjectCollection* DomainMaps;
  
private:
  vtkSelectionLink(const vtkSelectionLink&);  // Not implemented.
  void operator=(const vtkSelectionLink&);  // Not implemented.  
};

#endif
