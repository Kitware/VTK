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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
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
class vtkCommand;
class vtkDataObject;
class vtkDataRepresentation;
class vtkSelection;
class vtkSelectionLink;
class vtkStringArray;
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
  // Adds an i'th representation to the view.
  void AddRepresentation(int index, vtkDataRepresentation* rep);

  // Description:
  // Set the representation to the view.
  void SetRepresentation(vtkDataRepresentation* rep);
 
  // Description:
  // Set the i'th representation to the view.
  void SetRepresentation(int i, vtkDataRepresentation* rep); 

  // Description:
  // Set the j'th representation in the i'th Ports column to the view.
  void SetRepresentation(int i, int j, vtkDataRepresentation* rep);

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
  // connection and adds it to the view.
  // Returns the representation internally created.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* AddRepresentationFromInputConnection(int i, vtkAlgorithmOutput* conn);

  // Description:
  // Convenience method which sets the representation with the 
  // connection and adds it to the view.
  // Returns the representation internally created.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInputConnection(vtkAlgorithmOutput* conn);

  // Description:
  // Convenience method which sets the i'th representation with the 
  // connection and adds it to the view.
  // Returns the representation internally created.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInputConnection(int i, vtkAlgorithmOutput* conn);

  // Description:
  // Convenience method which sets the i,j'th representation with the 
  // connection and adds it to the view.
  // Returns the representation internally created.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInputConnection(int i, int j, vtkAlgorithmOutput* conn);

  // Description:
  // Convenience method which creates a simple representation with the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* AddRepresentationFromInput(vtkDataObject* input);

  // Description:
  // Convenience method which creates a simple representation with the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* AddRepresentationFromInput(int i, vtkDataObject* input);
  // Description:
  // Convenience method which sets the representation to the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInput(vtkDataObject* input);

  // Description:
  // Convenience method which sets the i'th representation with the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInput(int i, vtkDataObject* input);

  // Description:
  // Convenience method which sets the i,j 'th representation with the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInput(int i, int j, vtkDataObject* input);

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
  // Removes all representations from the Ports column i.
  void RemoveAllRepresentations(int i);
  
  // Description:
  // Returns the number of representations on Ports column 0 in this view.
  int GetNumberOfRepresentations();

  // Description:
  // The number of representations from the Ports column i.
  int GetNumberOfRepresentations(int i);
  
  // Description:
  // The representation at a specified index.
  vtkDataRepresentation* GetRepresentation(int index = 0);

  // Description:
  // The representation at a specified index.
  vtkDataRepresentation* GetRepresentation(int i, int j);
  
  // Description:
  // Check to see if a representation is present in the view.
  bool IsItemPresent(vtkDataRepresentation* rep);

  // Description:
  // Check to see if a representation is present in the i'th Ports column.
  bool IsItemPresent(int i, vtkDataRepresentation* rep);

  // Description:
  // Remove a representation from the Ports lists.
  void RemoveItem(vtkDataRepresentation* rep);

  // Description:
  // Update the view.
  virtual void Update() { }
  
  // Description:
  // Apply a theme to the view.
  virtual void ApplyViewTheme(vtkViewTheme* vtkNotUsed(theme)) { }

  // Description:
  // Set the selection type produced by this view.
  // This should be one of the content type constants defined in
  // vtkSelection.h. Common values are
  // vtkSelection::INDICES
  // vtkSelection::PEDIGREE_IDS
  // vtkSelection::VALUES
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

  //BTX
  // Description:
  // Returns the observer that the subclasses can use to listen to additional
  // events. Additionally these subclasses should override
  // ProcessEvents() to handle these events.
  vtkCommand* GetObserver();

  // Description:
  // A ptr to an instance of ViewProgressEventCallData is provided in the call
  // data when vtkCommand::ViewProgressEvent is fired.
  class ViewProgressEventCallData
    {
    const char* Message;
    double Progress;

  public:
    ViewProgressEventCallData(const char* msg, double progress)
      {
      this->Message = msg;
      this->Progress = progress;
      }
    ~ViewProgressEventCallData()
      {
      this->Message = 0;
      }

    // Description:
    // Get the message.
    const char* GetProgressMessage() const
      { return this->Message; }

    // Description:
    // Get the progress value in range [0.0, 1.0].
    double GetProgress() const
      { return this->Progress; }
    };
  //ETX
  
  // Description:
  // Meant for use by subclasses and vtkRepresentation subclasses.
  // Call this method to register vtkObjects (generally
  // vtkAlgorithm subclasses) which fire vtkCommand::ProgressEvent with the
  // view. The view listens to vtkCommand::ProgressEvent and fires
  // ViewProgressEvent with ViewProgressEventCallData containing the message and
  // the progress amount. If message is not provided, then the class name for
  // the algorithm is used.
  void RegisterProgress(vtkObject* algorithm, const char* message=NULL);

  // Description:
  // Unregister objects previously registered with RegisterProgress.
  void UnRegisterProgress(vtkObject* algorithm);

//BTX
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
  virtual void AddInputConnection(
    vtkAlgorithmOutput* vtkNotUsed(conn),
    vtkAlgorithmOutput* vtkNotUsed(selectionConn)) { }
  
  // Description:
  // Disconnects the internal pipeline.
  // Subclasses that handle tight integration between view and
  // representation should override this method.
  virtual void RemoveInputConnection(
    vtkAlgorithmOutput* vtkNotUsed(conn),
    vtkAlgorithmOutput* vtkNotUsed(selectionConn)) { }
  
  // Description:
  // The selection type created by the view.
  int SelectionType;

  // Description:
  // If a VALUES selection, the array names used in the selection.
  vtkStringArray* SelectionArrayNames;

  // Description:
  // If the Ports is not large enough to accomodate the port indicated,
  // then resize Ports.  If Ports sizing is adequate, do nothing.
  void SizePort(int i, int j);

  // Description:
  // Check to see if the port specified exists given current Ports sizes.  
  bool CheckPort(int i, int j);
  
private:
  vtkView(const vtkView&);  // Not implemented.
  void operator=(const vtkView&);  // Not implemented.

  class vtkImplementation;
  vtkImplementation* Implementation;

  class Command;
  friend class Command;
  Command* Observer;

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
