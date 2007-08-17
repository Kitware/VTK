/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkView - The superclass for all views.
//
// .SECTION Description
// vtkView is the superclass for views.  A view is generally an area of an
// application's canvas devoted to displaying one or more VTK data objects.
// Associated representations (subclasses of vtkDataRepresentation) are
// responsible for converting the data into a displayable format.  These
// representations are then added to the view.
//
// For views which display only one data object at a time you may set a
// data object or pipeline connection directly on the view itself (e.g.
// vtkGraphLayoutView, vtkTreeLayoutView, vtkLandscapeView, vtkTreeMapView).
// The view will internally create a vtkDataRepresentation for the data.
//
// A view has the concept of linked selection.  If the same data is displayed
// in multiple views, their selections may be linked by setting the same
// vtkSelectionLink on their representations (see vtkDataRepresentation).

#ifndef __vtkView_h
#define __vtkView_h

#include "vtkObject.h"

class vtkAlgorithmOutput;
class vtkCollection;
class vtkCommand;
class vtkDataObject;
class vtkDataRepresentation;
class vtkSelection;
class vtkSelectionLink;
class vtkViewTheme;

class VTK_VIEWS_EXPORT vtkView : public vtkObject
{
public:
  static vtkView *New();
  vtkTypeRevisionMacro(vtkView, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Adds the representation to the view.
  void AddRepresentation(vtkDataRepresentation* rep);
  
  // Description:
  // Convenience method which creates a simple representation with the 
  // connection and adds it to the view.
  // Returns the representation internally created.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* AddRepresentationFromInputConnection(vtkAlgorithmOutput* conn);

  // Description:
  // Convenience method which creates a simple representation with the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* AddRepresentationFromInput(vtkDataObject* input);

  // Description:
  // Removes the representation from the view.
  void RemoveRepresentation(vtkDataRepresentation* rep);

  // Description:
  // Removes any representation with this connection from the view.
  void RemoveRepresentation(vtkAlgorithmOutput* rep);
  
  // Description:
  // Removes all representations from the view.
  void RemoveAllRepresentations();

  // Description:
  // The number of representations in this view.
  int GetNumberOfRepresentations();

  // Description:
  // The representation at a specified index.
  vtkDataRepresentation* GetRepresentation(int index = 0);
  
  // Description:
  // Update the view.
  virtual void Update() { }
  
  // Description:
  // Apply a theme to the view.
  virtual void ApplyViewTheme(vtkViewTheme* vtkNotUsed(theme)) { }

  //BTX
  // Description:
  // Returns the observer that the subclasses can use to listen to additional
  // events. Additionally these subclasses should override
  // ProcessEvents() to handle these events.
  vtkCommand* GetObserver();
  //ETX

protected:
  vtkView();
  ~vtkView();
  
  // Description:
  // Called to process events.
  // The superclass processes selection changed events from its representations.
  // This may be overridden by subclasses to process additional events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
    
  // Description:
  // Connects to the internal pipeline.
  // Subclasses that handle tight integration between view and
  // representation should override this method.
  virtual void AddInputConnection(vtkAlgorithmOutput* vtkNotUsed(conn)) { }
  
  // Description:
  // Disconnects the internal pipeline.
  // Subclasses that handle tight integration between view and
  // representation should override this method.
  virtual void RemoveInputConnection(vtkAlgorithmOutput* vtkNotUsed(conn)) { }
  
  // Description:
  // Is called when the representation's selection link changes.
  // Subclasses that handle tight integration between view and
  // representation should override this method.
  virtual void SetSelectionLink(vtkSelectionLink* vtkNotUsed(link)) { }
  
  vtkCollection* Representations;

private:
  vtkView(const vtkView&);  // Not implemented.
  void operator=(const vtkView&);  // Not implemented.

  //BTX
  class Command;
  friend class Command;
  Command* Observer;
  //ETX
};

#endif
