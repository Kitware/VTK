// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkInteractorStyleTreeMapHover
 * @brief   An interactor style for a tree map view
 *
 *
 * The vtkInteractorStyleTreeMapHover specifically works with pipelines
 * that create a tree map.  Such pipelines will have a vtkTreeMapLayout
 * filter and a vtkTreeMapToPolyData filter, both of which must be passed
 * to this interactor style for it to function correctly.
 * This interactor style allows only 2D panning and zooming, and additionally
 * provides a balloon containing the name of the vertex hovered over,
 * and allows the user to highlight a vertex by clicking on it.
 */

#ifndef vtkInteractorStyleTreeMapHover_h
#define vtkInteractorStyleTreeMapHover_h

#include "vtkInteractorStyleImage.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBalloonRepresentation;
class vtkPoints;
class vtkRenderer;
class vtkTree;
class vtkTreeMapLayout;
class vtkTreeMapToPolyData;
class vtkWorldPointPicker;

class VTKVIEWSINFOVIS_EXPORT vtkInteractorStyleTreeMapHover : public vtkInteractorStyleImage
{
public:
  static vtkInteractorStyleTreeMapHover* New();
  vtkTypeMacro(vtkInteractorStyleTreeMapHover, vtkInteractorStyleImage);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Must be set to the vtkTreeMapLayout used to compute the bounds of each vertex
   * for the tree map.
   */
  void SetLayout(vtkTreeMapLayout* layout);
  vtkGetObjectMacro(Layout, vtkTreeMapLayout);
  ///@}

  ///@{
  /**
   * Must be set to the vtkTreeMapToPolyData used to convert the tree map
   * into polydata.
   */
  void SetTreeMapToPolyData(vtkTreeMapToPolyData* filter);
  vtkGetObjectMacro(TreeMapToPolyData, vtkTreeMapToPolyData);
  ///@}

  ///@{
  /**
   * The name of the field to use when displaying text in the hover balloon.
   */
  vtkSetStringMacro(LabelField);
  vtkGetStringMacro(LabelField);
  ///@}

  ///@{
  /**
   * Overridden from vtkInteractorStyleImage to provide the desired
   * interaction behavior.
   */
  void OnMouseMove() override;
  void OnLeftButtonUp() override;
  ///@}

  ///@{
  /**
   * Highlights a specific vertex.
   */
  void HighLightItem(vtkIdType id);
  void HighLightCurrentSelectedItem();
  ///@}

  void SetInteractor(vtkRenderWindowInteractor* rwi) override;

  /**
   * Set the color used to highlight the hovered vertex.
   */
  void SetHighLightColor(double r, double g, double b);

  /**
   * Set the color used to highlight the selected vertex.
   */
  void SetSelectionLightColor(double r, double g, double b);

  ///@{
  /**
   * The width of the line around the hovered vertex.
   */
  void SetHighLightWidth(double lw);
  double GetHighLightWidth();
  ///@}

  ///@{
  /**
   * The width of the line around the selected vertex.
   */
  void SetSelectionWidth(double lw);
  double GetSelectionWidth();
  ///@}

protected:
  vtkInteractorStyleTreeMapHover();
  ~vtkInteractorStyleTreeMapHover() override;

private:
  vtkInteractorStyleTreeMapHover(const vtkInteractorStyleTreeMapHover&) = delete;
  void operator=(const vtkInteractorStyleTreeMapHover&) = delete;

  // These methods are used internally
  vtkIdType GetTreeMapIdAtPos(int x, int y);
  void GetBoundingBoxForTreeMapItem(vtkIdType id, float* binfo);

  vtkWorldPointPicker* Picker;
  vtkBalloonRepresentation* Balloon;
  vtkActor* HighlightActor;
  vtkActor* SelectionActor;
  vtkPoints* HighlightPoints;
  vtkPoints* SelectionPoints;
  vtkTreeMapLayout* Layout;
  vtkTreeMapToPolyData* TreeMapToPolyData;
  char* LabelField;
  vtkIdType CurrentSelectedId;
};

VTK_ABI_NAMESPACE_END
#endif
