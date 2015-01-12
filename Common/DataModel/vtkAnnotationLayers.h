/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnnotationLayers.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkAnnotationLayers - Stores a ordered collection of annotation sets
//
// .SECTION Description
// vtkAnnotationLayers stores a vector of annotation layers. Each layer
// may contain any number of vtkAnnotation objects. The ordering of the
// layers introduces a prioritization of annotations. Annotations in
// higher layers may obscure annotations in lower layers.

#ifndef vtkAnnotationLayers_h
#define vtkAnnotationLayers_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

class vtkAnnotation;
class vtkSelection;

class VTKCOMMONDATAMODEL_EXPORT vtkAnnotationLayers : public vtkDataObject
{
public:
  vtkTypeMacro(vtkAnnotationLayers, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkAnnotationLayers* New();

  // Description:
  // The current annotation associated with this annotation link.
  virtual void SetCurrentAnnotation(vtkAnnotation* ann);
  vtkGetObjectMacro(CurrentAnnotation, vtkAnnotation);

  // Description:
  // The current selection associated with this annotation link.
  // This is simply the selection contained in the current annotation.
  virtual void SetCurrentSelection(vtkSelection* sel);
  virtual vtkSelection* GetCurrentSelection();

  // Description:
  // The number of annotations in a specific layer.
  unsigned int GetNumberOfAnnotations();

  // Description:
  // Retrieve an annotation from a layer.
  vtkAnnotation* GetAnnotation(unsigned int idx);

  // Description:
  // Add an annotation to a layer.
  void AddAnnotation(vtkAnnotation* ann);

  // Description:
  // Remove an annotation from a layer.
  void RemoveAnnotation(vtkAnnotation* ann);

  // Description:
  // Initialize the data structure to an empty state.
  virtual void Initialize();

  // Description:
  // Copy data from another data object into this one
  // which references the same member annotations.
  virtual void ShallowCopy(vtkDataObject* other);

  // Description:
  // Copy data from another data object into this one,
  // performing a deep copy of member annotations.
  virtual void DeepCopy(vtkDataObject* other);

  // Description:
  // Retrieve a vtkAnnotationLayers stored inside an information object.
  static vtkAnnotationLayers* GetData(vtkInformation* info);
  static vtkAnnotationLayers* GetData(vtkInformationVector* v, int i=0);

  // Description:
  // The modified time for this object.
  virtual unsigned long GetMTime();

//BTX
protected:
  vtkAnnotationLayers();
  ~vtkAnnotationLayers();

  class Internals;
  Internals* Implementation;
  vtkAnnotation* CurrentAnnotation;

private:
  vtkAnnotationLayers(const vtkAnnotationLayers&);  // Not implemented.
  void operator=(const vtkAnnotationLayers&);  // Not implemented.
//ETX
};

#endif
