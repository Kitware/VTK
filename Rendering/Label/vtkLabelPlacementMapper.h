/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelPlacementMapper.h

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
/**
 * @class   vtkLabelPlacementMapper
 * @brief   Places and renders non-overlapping labels.
 *
 *
 * To use this mapper, first send your data through vtkPointSetToLabelHierarchy,
 * which takes a set of points, associates special arrays to the points (label,
 * priority, etc.), and produces a prioritized spatial tree of labels.
 *
 * This mapper then takes that hierarchy (or hierarchies) as input, and every
 * frame will decide which labels and/or icons to place in order of priority,
 * and will render only those labels/icons. A label render strategy is used to
 * render the labels, and can use e.g. FreeType or Qt for rendering.
*/

#ifndef vtkLabelPlacementMapper_h
#define vtkLabelPlacementMapper_h

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkMapper2D.h"

class vtkCoordinate;
class vtkLabelRenderStrategy;
class vtkSelectVisiblePoints;

class VTKRENDERINGLABEL_EXPORT vtkLabelPlacementMapper : public vtkMapper2D
{
public:
  static vtkLabelPlacementMapper *New();
  vtkTypeMacro(vtkLabelPlacementMapper, vtkMapper2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Draw non-overlapping labels to the screen.
   */
  void RenderOverlay(vtkViewport *viewport, vtkActor2D *actor);

  //@{
  /**
   * Set the label rendering strategy.
   */
  virtual void SetRenderStrategy(vtkLabelRenderStrategy* s);
  vtkGetObjectMacro(RenderStrategy, vtkLabelRenderStrategy);
  //@}

  //@{
  /**
   * The maximum fraction of the screen that the labels may cover.
   * Label placement stops when this fraction is reached.
   */
  vtkSetClampMacro(MaximumLabelFraction,double,0.,1.);
  vtkGetMacro(MaximumLabelFraction,double);
  //@}

  //@{
  /**
   * The type of iterator used when traversing the labels.
   * May be vtkLabelHierarchy::FRUSTUM or vtkLabelHierarchy::FULL_SORT
   */
  vtkSetMacro(IteratorType,int);
  vtkGetMacro(IteratorType,int);
  //@}

  //@{
  /**
   * Set whether, or not, to use unicode strings.
   */
  vtkSetMacro(UseUnicodeStrings,bool);
  vtkGetMacro(UseUnicodeStrings,bool);
  vtkBooleanMacro(UseUnicodeStrings,bool);
  //@}

  //@{
  /**
   * Use label anchor point coordinates as normal vectors and eliminate those
   * pointing away from the camera. Valid only when points are on a sphere
   * centered at the origin (such as a 3D geographic view). Off by default.
   */
  vtkGetMacro(PositionsAsNormals,bool);
  vtkSetMacro(PositionsAsNormals,bool);
  vtkBooleanMacro(PositionsAsNormals,bool);
  //@}

  //@{
  /**
   * Enable drawing spokes (lines) to anchor point coordinates that were perturbed
   * for being coincident with other anchor point coordinates.
   */
  vtkGetMacro(GeneratePerturbedLabelSpokes,bool);
  vtkSetMacro(GeneratePerturbedLabelSpokes,bool);
  vtkBooleanMacro(GeneratePerturbedLabelSpokes,bool);
  //@}

  //@{
  /**
   * Use the depth buffer to test each label to see if it should not be displayed if
   * it would be occluded by other objects in the scene. Off by default.
   */
  vtkGetMacro(UseDepthBuffer,bool);
  vtkSetMacro(UseDepthBuffer,bool);
  vtkBooleanMacro(UseDepthBuffer,bool);
  //@}

  //@{
  /**
   * Tells the placer to place every label regardless of overlap.
   * Off by default.
   */
  vtkSetMacro(PlaceAllLabels, bool);
  vtkGetMacro(PlaceAllLabels, bool);
  vtkBooleanMacro(PlaceAllLabels, bool);
  //@}

  //@{
  /**
   * Whether to render traversed bounds. Off by default.
   */
  vtkSetMacro(OutputTraversedBounds, bool);
  vtkGetMacro(OutputTraversedBounds, bool);
  vtkBooleanMacro(OutputTraversedBounds, bool);
  //@}

  enum LabelShape {
    NONE,
    RECT,
    ROUNDED_RECT,
    NUMBER_OF_LABEL_SHAPES
  };

  //@{
  /**
   * The shape of the label background, should be one of the
   * values in the LabelShape enumeration.
   */
  vtkSetClampMacro(Shape, int, 0, NUMBER_OF_LABEL_SHAPES-1);
  vtkGetMacro(Shape, int);
  virtual void SetShapeToNone()
    { this->SetShape(NONE); }
  virtual void SetShapeToRect()
    { this->SetShape(RECT); }
  virtual void SetShapeToRoundedRect()
    { this->SetShape(ROUNDED_RECT); }
  //@}

  enum LabelStyle {
    FILLED,
    OUTLINE,
    NUMBER_OF_LABEL_STYLES
  };

  //@{
  /**
   * The style of the label background shape, should be one of the
   * values in the LabelStyle enumeration.
   */
  vtkSetClampMacro(Style, int, 0, NUMBER_OF_LABEL_STYLES-1);
  vtkGetMacro(Style, int);
  virtual void SetStyleToFilled()
    { this->SetStyle(FILLED); }
  virtual void SetStyleToOutline()
    { this->SetStyle(OUTLINE); }
  //@}

  //@{
  /**
   * The size of the margin on the label background shape.
   * Default is 5.
   */
  vtkSetMacro(Margin, double);
  vtkGetMacro(Margin, double);
  //@}

  //@{
  /**
   * The color of the background shape.
   */
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);
  //@}

  //@{
  /**
   * The opacity of the background shape.
   */
  vtkSetClampMacro(BackgroundOpacity, double, 0.0, 1.0);
  vtkGetMacro(BackgroundOpacity, double);
  //@}

  //@{
  /**
   * Get the transform for the anchor points.
   */
  vtkGetObjectMacro(AnchorTransform,vtkCoordinate);
  //@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkLabelPlacementMapper();
  ~vtkLabelPlacementMapper();

  virtual void SetAnchorTransform( vtkCoordinate* );

  virtual int FillInputPortInformation( int port, vtkInformation* info );

  class Internal;
  Internal* Buckets;

  vtkLabelRenderStrategy* RenderStrategy;
  vtkCoordinate* AnchorTransform;
  vtkSelectVisiblePoints* VisiblePoints;
  double MaximumLabelFraction;
  bool PositionsAsNormals;
  bool GeneratePerturbedLabelSpokes;
  bool UseDepthBuffer;
  bool UseUnicodeStrings;
  bool PlaceAllLabels;
  bool OutputTraversedBounds;

  int LastRendererSize[2];
  double LastCameraPosition[3];
  double LastCameraFocalPoint[3];
  double LastCameraViewUp[3];
  double LastCameraParallelScale;
  int IteratorType;

  int Style;
  int Shape;
  double Margin;
  double BackgroundOpacity;
  double BackgroundColor[3];

private:
  vtkLabelPlacementMapper(const vtkLabelPlacementMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLabelPlacementMapper&) VTK_DELETE_FUNCTION;
};

#endif

