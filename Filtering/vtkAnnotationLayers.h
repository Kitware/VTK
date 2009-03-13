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

#ifndef __vtkAnnotationLayers_h
#define __vtkAnnotationLayers_h

#include "vtkDataObject.h"

class vtkAnnotation;

class VTK_FILTERING_EXPORT vtkAnnotationLayers : public vtkDataObject
{
public:
  vtkTypeRevisionMacro(vtkAnnotationLayers, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkAnnotationLayers* New();

  // Description:
  // The number of layers of annotations.
  unsigned int GetNumberOfLayers();

  // Description:
  // The number of annotations in a specific layer.
  unsigned int GetNumberOfAnnotations(unsigned int layer);

  // Description:
  // Retrieve an annotation from a layer.
  vtkAnnotation* GetAnnotation(unsigned int layer, unsigned int idx);

  // Description:
  // Add an annotation to a layer.
  void AddAnnotation(unsigned int layer, vtkAnnotation* ann);

  // Description:
  // Remove an annotation from a layer.
  void RemoveAnnotation(unsigned int layer, vtkAnnotation* ann);

  // Description:
  // Add an empty annotation layer to the top of the layer stack.
  void AddLayer()
    { this->InsertLayer(this->GetNumberOfLayers()); }

  // Description:
  // Insert an empty annotation layer at the specified index.
  // This increases the index of existing layers with index >= layer
  // by 1.
  void InsertLayer(unsigned int layer);

  // Description:
  // Remove an annotation layer.
  // This decreases the index of existing layers with index >= layer
  // by 1.
  void RemoveLayer(unsigned int layer);

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

//BTX
protected:
  vtkAnnotationLayers();
  ~vtkAnnotationLayers();

  class Internals;
  Internals* Implementation;

private:
  vtkAnnotationLayers(const vtkAnnotationLayers&);  // Not implemented.
  void operator=(const vtkAnnotationLayers&);  // Not implemented.
//ETX
};

#endif

