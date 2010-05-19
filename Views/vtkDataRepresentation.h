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
// setting the selection on its vtkAnnotationLink.  Other representations
// sharing the same selection link instance will get the same selection
// from the selection link when the view is updated.  The application is
// responsible for linking representations as appropriate by setting the
// same vtkAnnotationLink on each linked representation.

#ifndef __vtkDataRepresentation_h
#define __vtkDataRepresentation_h

#include "vtkPassInputTypeAlgorithm.h"

class vtkAlgorithmOutput;
class vtkAnnotationLink;
class vtkDataObject;
class vtkSelection;
class vtkStringArray;
class vtkView;
class vtkViewTheme;

class VTK_VIEWS_EXPORT vtkDataRepresentation : public vtkPassInputTypeAlgorithm
{
public:
  static vtkDataRepresentation *New();
  vtkTypeMacro(vtkDataRepresentation, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience override method for obtaining the input connection
  // without specifying the port or index.
  vtkAlgorithmOutput* GetInputConnection(int port = 0, int index = 0)
    { return this->Superclass::GetInputConnection(port, index); }
  
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
  // The optional third argument specifies whether the selection should be
  // added to the previous selection on this representation.
  void Select(vtkView* view, vtkSelection* selection)
    { this->Select(view, selection, false); }
  void Select(vtkView* view, vtkSelection* selection, bool extend);

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
  // The optional second argument specifies whether the selection should be
  // added to the previous selection on this representation.
  void UpdateSelection(vtkSelection* selection)
    { this->UpdateSelection(selection, false); }
  void UpdateSelection(vtkSelection* selection, bool extend);

  // Description:
  // The output port that contains the annotations whose selections are
  // localized for a particular input data object.
  // This should be used when connecting the internal pipelines.
  virtual vtkAlgorithmOutput* GetInternalAnnotationOutputPort()
    { return this->GetInternalAnnotationOutputPort(0); }
  virtual vtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port)
    { return this->GetInternalAnnotationOutputPort(port, 0); }
  virtual vtkAlgorithmOutput* GetInternalAnnotationOutputPort(int port, int conn);

  // Description:
  // The output port that contains the selection associated with the
  // current annotation (normally the interactive selection).
  // This should be used when connecting the internal pipelines.
  virtual vtkAlgorithmOutput* GetInternalSelectionOutputPort()
    { return this->GetInternalSelectionOutputPort(0); }
  virtual vtkAlgorithmOutput* GetInternalSelectionOutputPort(int port)
    { return this->GetInternalSelectionOutputPort(port, 0); }
  virtual vtkAlgorithmOutput* GetInternalSelectionOutputPort(int port, int conn);

  // Description:
  // Retrieves an output port for the input data object at the specified port
  // and connection index. This may be connected to the representation's
  // internal pipeline.
  virtual vtkAlgorithmOutput* GetInternalOutputPort()
    { return this->GetInternalOutputPort(0); }
  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port)
    { return this->GetInternalOutputPort(port, 0); }
  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port, int conn);
 
  // Description:
  // Set the selection type produced by this view.
  // This should be one of the content type constants defined in
  // vtkSelectionNode.h. Common values are
  // vtkSelectionNode::INDICES
  // vtkSelectionNode::PEDIGREEIDS
  // vtkSelectionNode::VALUES
  vtkSetMacro(SelectionType, int);
  vtkGetMacro(SelectionType, int);

  // Description:
  // If a VALUES selection, the arrays used to produce a selection.
  virtual void SetSelectionArrayNames(vtkStringArray* names);
  vtkGetObjectMacro(SelectionArrayNames, vtkStringArray);

  // Description:
  // If a VALUES selection, the array used to produce a selection.
  virtual void SetSelectionArrayName(const char* name);
  virtual const char* GetSelectionArrayName();

protected:
  vtkDataRepresentation();
  ~vtkDataRepresentation();
  
  // Description:
  // Subclasses should override this to connect inputs to the internal pipeline
  // as necessary. Since most representations are "meta-filters" (i.e. filters
  // containing other filters), you should create shallow copies of your input
  // before connecting to the internal pipeline. The convenience method
  // GetInternalOutputPort will create a cached shallow copy of a specified
  // input for you. The related helper functions GetInternalAnnotationOutputPort,
  // GetInternalSelectionOutputPort should be used to obtain a selection or
  // annotation port whose selections are localized for a particular input data object.
  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*)
    { return 1; }

  // Description:
  // Clear the input shallow copy caches if the algorithm is in "release data" mode.
  virtual void ProcessEvents(vtkObject *caller, unsigned long eventId, void *callData);

  // Description:
  // The annotation link for this representation.
  virtual void SetAnnotationLinkInternal(vtkAnnotationLink* link);
  vtkAnnotationLink* AnnotationLinkInternal;

  // Whether is represenation can handle a selection.
  bool Selectable;

  // Description:
  // The selection type created by the view.
  int SelectionType;

  // Description:
  // If a VALUES selection, the array names used in the selection.
  vtkStringArray* SelectionArrayNames;

  //BTX
  friend class vtkView;
  friend class vtkRenderView;
  class Command;
  friend class Command;
  Command* Observer;
  //ETX

  // ------------------------------------------------------------------------
  // Methods to override in subclasses
  // ------------------------------------------------------------------------

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
  // representations through vtkAnnotationLink, possibly using the view.
  // For the superclass, we just return the same selection.
  // Subclasses may do something more fancy, like convert the selection
  // from a frustrum to a list of pedigree ids.  If the selection cannot
  // be applied to this representation, return NULL.
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);

private:
  vtkDataRepresentation(const vtkDataRepresentation&);  // Not implemented.
  void operator=(const vtkDataRepresentation&);  // Not implemented.

  //BTX
  class Internals;
  Internals* Implementation;
  //ETX
};

#endif
