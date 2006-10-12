/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTreeMapHover.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleTreeMapHover - An interactor style for a tree map view
//
// .SECTION Description

#ifndef __vtkInteractorStyleTreeMapHover_h
#define __vtkInteractorStyleTreeMapHover_h

#include "vtkInteractorStyleImage.h"

class vtkBalloonRepresentation;
class vtkPoints;
class vtkRenderer;
class vtkTree;
class vtkTreeMapLayout;
class vtkTreeMapToPolyData;
class vtkWorldPointPicker;

class VTK_INFOVIS_EXPORT vtkInteractorStyleTreeMapHover : public vtkInteractorStyleImage
{
public:
  static vtkInteractorStyleTreeMapHover* New();
  vtkTypeRevisionMacro(vtkInteractorStyleTreeMapHover,vtkInteractorStyleImage);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetLayout(vtkTreeMapLayout* layout);
  vtkGetObjectMacro(Layout, vtkTreeMapLayout);

  void SetTreeMapToPolyData(vtkTreeMapToPolyData* filter);
  vtkGetObjectMacro(TreeMapToPolyData, vtkTreeMapToPolyData);

  vtkSetStringMacro(LabelField);
  vtkGetStringMacro(LabelField);
  
  void OnMouseMove();
  void OnLeftButtonUp();
  
  void HighLightItem(vtkIdType id);
  void HighLightCurrentSelectedItem();

  virtual void SetInteractor(vtkRenderWindowInteractor *rwi);
  void SetHighLightColor(double r, double g, double b);
  void SetSelectionLightColor(double r, double g, double b);
  void SetHighLightWidth(double lw);
  void SetSelectionWidth(double lw);

protected:
  vtkInteractorStyleTreeMapHover();
  ~vtkInteractorStyleTreeMapHover();

private:
  vtkInteractorStyleTreeMapHover(const vtkInteractorStyleTreeMapHover&); // Not implemented
  void operator=(const vtkInteractorStyleTreeMapHover&); // Not implemented

  // These methods are used internally
  vtkIdType GetTreeMapIdAtPos(int x, int y);
  void GetBoundingBoxForTreeMapItem(vtkIdType id, float *binfo);
  
  vtkWorldPointPicker* Picker;
  vtkBalloonRepresentation* Balloon;
  vtkActor *HighlightActor;
  vtkActor *SelectionActor;
  vtkPoints *HighlightPoints;
  vtkPoints *SelectionPoints;
  vtkTreeMapLayout* Layout;
  vtkTreeMapToPolyData* TreeMapToPolyData;
  char *LabelField;
  vtkIdType CurrentSelectedId;
};

#endif
