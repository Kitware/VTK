/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkXYPlotRepresentation
 * @brief   represent XY plot for vtkXYPlotWidget
 *
 *
 *
 * This class represents a XY plot for a vtkXYPlotWidget.  This class
 * provides support for interactively placing a XY plot on the 2D overlay
 * plane.  The XY plot is defined by an instance of vtkXYPlotActor.
 *
 * @sa
 * vtkXYPlotWidget vtkWidgetRepresentation vtkXYPlotActor
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2012
*/

#ifndef vtkXYPlotRepresentation_h
#define vtkXYPlotRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkXYPlotActor;

class VTKINTERACTIONWIDGETS_EXPORT vtkXYPlotRepresentation : public vtkBorderRepresentation
{
public:
  vtkTypeMacro(vtkXYPlotRepresentation, vtkBorderRepresentation);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkXYPlotRepresentation *New();

  //@{
  /**
   * The prop that is placed in the renderer.
   */
  vtkGetObjectMacro(XYPlotActor, vtkXYPlotActor);
  virtual void SetXYPlotActor(vtkXYPlotActor *);
  //@}

  //@{
  /**
   * Satisfy the superclass' API.
   */
  virtual void BuildRepresentation();
  virtual void WidgetInteraction(double eventPos[2]);
  virtual void GetSize(double size[2])
    {size[0]=2.0; size[1]=2.0;}
  //@}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  virtual int GetVisibility();
  virtual void SetVisibility(int);
  virtual void GetActors2D(vtkPropCollection *collection);
  virtual void ReleaseGraphicsResources(vtkWindow *window);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();
  //@}

  //@{
  /**
   * Set glyph properties
   */
  void SetGlyphSize(double x);
  void SetPlotGlyphType(int curve, int glyph);
  //@}

  //@{
  /**
   * Set title properties
   */
  void SetTitle(const char* title);
  void SetTitleColor(double r, double g, double b);
  void SetTitleFontFamily(int x);
  void SetTitleBold(int x);
  void SetTitleItalic(int x);
  void SetTitleShadow(int x);
  void SetTitleFontSize(int x);
  void SetTitleJustification(int x);
  void SetTitleVerticalJustification(int x);
  void SetAdjustTitlePosition(int x);
  void SetTitlePosition(double x, double y);
  //@}

  //@{
  /**
   * Set/Get axis properties
   */
  void SetXAxisColor(double r, double g, double b);
  void SetYAxisColor(double r, double g, double b);
  void SetXTitle( const char* ytitle );
  char* GetXTitle();
  void SetXRange(double min, double max);
  void SetYTitle( const char* ytitle );
  char* GetYTitle();
  void SetYRange(double min, double max);
  void SetYTitlePosition (int pos);
  int GetYTitlePosition() const;
  void SetXValues(int x);
  //@}

  //@{
  /**
   * Set axis title properties
   */
  void SetAxisTitleColor(double r, double g, double b);
  void SetAxisTitleFontFamily(int x);
  void SetAxisTitleBold(int x);
  void SetAxisTitleItalic(int x);
  void SetAxisTitleShadow(int x);
  void SetAxisTitleFontSize(int x);
  void SetAxisTitleJustification(int x);
  void SetAxisTitleVerticalJustification(int x);
  //@}

  //@{
  /**
   * Set axis label properties
   */
  void SetAxisLabelColor(double r, double g, double b);
  void SetAxisLabelFontFamily(int x);
  void SetAxisLabelBold(int x);
  void SetAxisLabelItalic(int x);
  void SetAxisLabelShadow(int x);
  void SetAxisLabelFontSize(int x);
  void SetAxisLabelJustification(int x);
  void SetAxisLabelVerticalJustification(int x);
  void SetXLabelFormat(const char* _arg);
  void SetYLabelFormat(const char* _arg);
  //@}

  //@{
  /**
   * Set various properties
   */
  void SetBorder(int x);
  void RemoveAllActiveCurves();
  void AddUserCurvesPoint( double c, double x, double y);
  void SetLegend(int x);
  void SetLegendBorder(int b);
  void SetLegendBox(int b);
  void SetLegendBoxColor(double r, double g, double b);
  void SetLegendPosition(double x, double y);
  void SetLegendPosition2(double x, double y);
  void SetLineWidth(double w);
  void SetPlotColor(int i, int r, int g, int b);
  void SetPlotLines(int i);
  void SetPlotPoints(int i);
  void SetPlotLabel(int i, const char* label);
  //@}

protected:
  vtkXYPlotRepresentation();
  ~vtkXYPlotRepresentation();

  vtkXYPlotActor *XYPlotActor;
private:
  vtkXYPlotRepresentation(const vtkXYPlotRepresentation &) VTK_DELETE_FUNCTION;
  void operator=(const vtkXYPlotRepresentation &) VTK_DELETE_FUNCTION;
};

#endif //vtkXYPlotRepresentation_h
