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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkDataRepresentation - The superclass for all representations
//
// .SECTION Description
// vtkDataRepresentation the superclass for representations of data objects.
// This class itself may be instantiated and used as a representation
// that simply holds a connection to a pipeline.
//
// If there are multiple representations present in a view, you should use
// a subclass of vtkDataRepresentation.  The representation is
// responsible for taking the input pipeline connection and converting it
// to an object usable by a view.  In the most common case, the representation
// will contain the pipeline necessary to convert a data object into an actor
// or set of actors.
//
// The representation has a concept of a selection.  If the user performs a
// selection operation on the view, the view forwards this on to its
// representations.  The representation is responsible for displaying that
// selection in an appropriate way.
//
// Representation selections may also be linked.  The representation shares
// the selection by converting it into a view-independent format, then
// setting the selection on its vtkSelectionLink.  Other representations
// sharing the same selection link instance will get the same selection
// from the selection link when the view is updated.  The application is
// responsible for linking representations as appropriate by setting the
// same vtkSelectionLink on each linked representation.

#ifndef __vtkDataRepresentation_h
#define __vtkDataRepresentation_h

#include "vtkPassInputTypeAlgorithm.h"

class vtkAlgorithmOutput;
class vtkAnnotationLink;
class vtkDataObject;
class vtkSelection;
class vtkSelectionLink;
class vtkView;
class vtkViewTheme;

class VTK_VIEWS_EXPORT vtkDataRepresentation : public vtkPassInputTypeAlgorithm
{
public:
  static vtkDataRepresentation *New();
  vtkTypeRevisionMacro(vtkDataRepresentation, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience override method for obtaining the input connection
  // without specifying the port or index.
  vtkAlgorithmOutput* GetInputConnection(int port = 0, int index = 0)
    { return this->Superclass::GetInputConnection(port, index); }
  
  // Description:
  // The selection link for this representation.
  // To link selections, set the same vtkSelectionLink object in
  // multiple representations.
  vtkSelectionLink* GetSelectionLink()
    { return this->SelectionLinkInternal; }
  void SetSelectionLink(vtkSelectionLink* link);

  // Description:
  // The annotation link for this representation.
  // To link annotations, set the same vtkAnnotationLink object in
  // multiple representations.
  vtkAnnotationLink* GetAnnotationLink()
    { return this->AnnotationLinkInternal; }
  void SetAnnotationLink(vtkAnnotationLink* link);

  // Description:
  // Apply a theme to this representation.
  // Subclasses should override this method.
  virtual void ApplyViewTheme(vtkViewTheme* vtkNotUsed(theme)) { }

  // Description:
  // The view calls this method when a selection occurs.
  // The representation takes this selection and converts it into
  // a selection on its data by calling ConvertSelection,
  // then calls UpdateSelection with the converted selection.
  // Subclasses should not overrride this method, but should instead
  // override ConvertSelection.
  void Select(vtkView* view, vtkSelection* selection);

  // Description:
  // Whether this representation is able to handle a selection.
  // Default is true.
  vtkSetMacro(Selectable, bool);
  vtkGetMacro(Selectable, bool);
  vtkBooleanMacro(Selectable, bool);
  
  // Description:
  // Updates the selection in the selection link and fires a selection
  // change event. Subclasses should not overrride this method,
  // but should instead override ConvertSelection.
  void UpdateSelection(vtkSelection* selection);

  // Description:
  // The output port that contains the selection after it has gone through
  // the domain map. ExtractSelection filters in views/representations
  // should use this port for the selection input.
  virtual vtkAlgorithmOutput* GetSelectionConnection(
    int port = 0, int conn = 0);

  // Description:
  // The output port that contains the annotations for this representation.
  virtual vtkAlgorithmOutput* GetAnnotationConnection(
    int port = 0, int conn = 0);

  // Description:
  // Retrieves the input data object at the specified port
  // and connection index. This may be connected to the representation's
  // internal pipeline.
  virtual vtkDataObject* GetInput(int port = 0, int conn = 0);
 
protected:
  vtkDataRepresentation();
  ~vtkDataRepresentation();
  
  // Description:
  // Creates shallow copies of all inputs, which are available to subclasses
  // through GetInput(), then calls PrepareInputConnections() on the subclass.
  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  // Description:
  // The linked selection.
  virtual void SetSelectionLinkInternal(vtkSelectionLink* link);
  vtkSelectionLink* SelectionLinkInternal;

  // Description:
  // The annotation link for this representation.
  virtual void SetAnnotationLinkInternal(vtkAnnotationLink* link);
  vtkAnnotationLink* AnnotationLinkInternal;

  // Whether is represenation can handle a selection.
  bool Selectable;

  //BTX
  friend class vtkView;
  friend class vtkRenderView;
  //ETX

  // ------------------------------------------------------------------------
  // Methods to override in subclasses
  // ------------------------------------------------------------------------

  // Description:
  virtual void SetInputConnectionInternal(vtkAlgorithmOutput* vtkNotUsed(conn))
    { }

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* vtkNotUsed(view)) { return true; }
  
  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* vtkNotUsed(view)) { return true; }
  
  // Description:
  // Convert the selection to a type appropriate for sharing with other
  // representations through vtkSelectionLink, possibly using the view.
  // For the superclass, we just return the same selection.
  // Subclasses may do something more fancy, like convert the selection
  // from a frustrum to a list of pedigree ids.  If the selection cannot
  // be applied to this representation, return NULL.
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);

  // Description:
  // Subclasses should override this method, calling GetInput(),
  // GetSelectionConnection(), and GetAnnotationConnection() in order
  // to use possibly new input data objects.
  virtual void PrepareInputConnections() { }

private:
  vtkDataRepresentation(const vtkDataRepresentation&);  // Not implemented.
  void operator=(const vtkDataRepresentation&);  // Not implemented.

  //BTX
  class Internals;
  Internals* Implementation;
  //ETX
};

#endif
