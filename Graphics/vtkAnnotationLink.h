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
// .NAME vtkAnnotationLink - An algorithm for linking annotations among objects
// .SECTION Description
// vtkAnnotationLink is a simple source filter which outputs the
// vtkAnnotationLayers object stored internally.  Multiple objects may share
// the same annotation link filter and connect it to an internal pipeline so
// that if one object changes the annotation set, it will be pulled into all
// the other objects when their pipelines update.

#ifndef __vtkAnnotationLink_h
#define __vtkAnnotationLink_h

#include "vtkAnnotationLayersAlgorithm.h"

class vtkDataObjectCollection;
class vtkInformation;
class vtkInformationVector;
class vtkSelection;
class vtkTable;

class VTK_GRAPHICS_EXPORT vtkAnnotationLink : public vtkAnnotationLayersAlgorithm
{
public:
  static vtkAnnotationLink *New();
  vtkTypeRevisionMacro(vtkAnnotationLink, vtkAnnotationLayersAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The annotations to be shared.
  vtkGetObjectMacro(AnnotationLayers, vtkAnnotationLayers);
  virtual void SetAnnotationLayers(vtkAnnotationLayers* layers);
  
protected:
  vtkAnnotationLink();
  ~vtkAnnotationLink();
  
  // Description:
  // Shallow copy the internal selection to the output.
  virtual int RequestData(
    vtkInformation *info,
    vtkInformationVector **inVector,
    vtkInformationVector *outVector);

  // Description:
  // The shared selection.
  vtkAnnotationLayers* AnnotationLayers;

private:
  vtkAnnotationLink(const vtkAnnotationLink&);  // Not implemented.
  void operator=(const vtkAnnotationLink&);  // Not implemented.  
};

#endif
