/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataRepresentation.h

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
// .NAME vtkDataRepresentation - The superclass for all representations
// .SECTION Description
// vtkDataRepresentation the superclass for representations of data objects.
// This class itself may be instantiated and used as a representation
// that simply holds a connection to a pipeline.  This is used when there
// is tight integration between view and representation, and the view
// mangages all representation pipleline / logic.
//
// If there is looser integration between representation and view and/or
// there are multiple representations present in a view, you should use
// a subclass of vtkDataRepresentation.  The representation should be
// responsible for taking the input pipeline connection and converting it
// to an object usable by a view.  In the most common case, the representation
// will contain the pipeline necessary to convert a data object into an actor
// or set of actors.

#ifndef __vtkDataRepresentation_h
#define __vtkDataRepresentation_h

#include "vtkObject.h"

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkSelection;
class vtkSelectionLink;
class vtkView;

class VTK_VIEWS_EXPORT vtkDataRepresentation : public vtkObject
{
public:
  static vtkDataRepresentation *New();
  vtkTypeRevisionMacro(vtkDataRepresentation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // A convenience method that calls SetInputConnection on the producer's
  // connection.
  void SetInput(vtkDataObject* input);
  
  // Description:
  // Sets the input pipeline connection to this representation.
  // This method should be overridden by subclasses to connect to the
  // internal pipeline.
  virtual void SetInputConnection(vtkAlgorithmOutput* conn);
  vtkGetObjectMacro(InputConnection, vtkAlgorithmOutput);
  
  // Description:
  // Handle a selection a view.  Subclasses should not generally override this.
  // This function calls ConvertSelection() to (possibly) convert the selection
  // into something appropriate for linked representations.
  // This function then calls UpdateSelection() with the result of the
  // conversion.
  void Select(vtkView* view, vtkSelection* selection);
  
  // Description:
  // Copies the selection into the representation's linked selection
  // and triggers a SelectionChanged event.
  void UpdateSelection(vtkSelection* selection);
  
  // Description:
  // The selection linking object.
  // Set the same selection link on multiple representations to link views.
  // Subclasses should override SetSelectionLink to additionally connect
  // the selection link into the internal selection pipeline.
  vtkGetObjectMacro(SelectionLink, vtkSelectionLink);
  virtual void SetSelectionLink(vtkSelectionLink* link);
  
protected:
  vtkDataRepresentation();
  ~vtkDataRepresentation();
  
  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  virtual bool AddToView(vtkView* vtkNotUsed(view)) { return true; }
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  virtual bool RemoveFromView(vtkView* vtkNotUsed(view)) { return true; }
  
  // Description:
  // Convert the selection to a type appropriate for sharing with other
  // representations through vtkSelectionLink, possibly using the view.
  // For the superclass, we just return the same selection.
  // Subclasses may do something more fancy, like convert the selection
  // from a frustrum to a list of pedigree ids.  If the selection cannot
  // be applied to this representation, return NULL.
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);
  
  // The input connection.
  vtkAlgorithmOutput* InputConnection;
  
  // The linked selection.
  vtkSelectionLink* SelectionLink;
  
  //BTX
  friend class vtkView;
  //ETX

private:
  vtkDataRepresentation(const vtkDataRepresentation&);  // Not implemented.
  void operator=(const vtkDataRepresentation&);  // Not implemented.
};

#endif
