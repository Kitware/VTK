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

/**
 * @class   vtkAnnotationLayers
 * @brief   Stores a ordered collection of annotation sets
 *
 *
 * vtkAnnotationLayers stores a vector of annotation layers. Each layer
 * may contain any number of vtkAnnotation objects. The ordering of the
 * layers introduces a prioritization of annotations. Annotations in
 * higher layers may obscure annotations in lower layers.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAnnotationLayers* New();

  //@{
  /**
   * The current annotation associated with this annotation link.
   */
  virtual void SetCurrentAnnotation(vtkAnnotation* ann);
  vtkGetObjectMacro(CurrentAnnotation, vtkAnnotation);
  //@}

  //@{
  /**
   * The current selection associated with this annotation link.
   * This is simply the selection contained in the current annotation.
   */
  virtual void SetCurrentSelection(vtkSelection* sel);
  virtual vtkSelection* GetCurrentSelection();
  //@}

  /**
   * The number of annotations in a specific layer.
   */
  unsigned int GetNumberOfAnnotations();

  /**
   * Retrieve an annotation from a layer.
   */
  vtkAnnotation* GetAnnotation(unsigned int idx);

  /**
   * Add an annotation to a layer.
   */
  void AddAnnotation(vtkAnnotation* ann);

  /**
   * Remove an annotation from a layer.
   */
  void RemoveAnnotation(vtkAnnotation* ann);

  /**
   * Initialize the data structure to an empty state.
   */
  void Initialize() override;

  /**
   * Copy data from another data object into this one
   * which references the same member annotations.
   */
  void ShallowCopy(vtkDataObject* other) override;

  /**
   * Copy data from another data object into this one,
   * performing a deep copy of member annotations.
   */
  void DeepCopy(vtkDataObject* other) override;

  //@{
  /**
   * Retrieve a vtkAnnotationLayers stored inside an information object.
   */
  static vtkAnnotationLayers* GetData(vtkInformation* info);
  static vtkAnnotationLayers* GetData(vtkInformationVector* v, int i=0);
  //@}

  /**
   * The modified time for this object.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkAnnotationLayers();
  ~vtkAnnotationLayers() override;

  class Internals;
  Internals* Implementation;
  vtkAnnotation* CurrentAnnotation;

private:
  vtkAnnotationLayers(const vtkAnnotationLayers&) = delete;
  void operator=(const vtkAnnotationLayers&) = delete;

};

#endif
