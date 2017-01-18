/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToGlyphs.h

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
 * @class   vtkGraphToGlyphs
 * @brief   create glyphs for graph vertices
 *
 *
 * Converts a vtkGraph to a vtkPolyData containing a glyph for each vertex.
 * This assumes that the points
 * of the graph have already been filled (perhaps by vtkGraphLayout).
 * The glyphs will automatically be scaled to be the same size in screen
 * coordinates. To do this the filter requires a pointer to the renderer
 * into which the glyphs will be rendered.
*/

#ifndef vtkGraphToGlyphs_h
#define vtkGraphToGlyphs_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // for SP ivars

class vtkDistanceToCamera;
class vtkGraphToPoints;
class vtkGlyph3D;
class vtkGlyphSource2D;
class vtkRenderer;
class vtkSphereSource;

class VTKRENDERINGCORE_EXPORT vtkGraphToGlyphs : public vtkPolyDataAlgorithm
{
public:
  static vtkGraphToGlyphs *New();
  vtkTypeMacro(vtkGraphToGlyphs, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  enum
  {
    VERTEX = 1,
    DASH,
    CROSS,
    THICKCROSS,
    TRIANGLE,
    SQUARE,
    CIRCLE,
    DIAMOND,
    SPHERE
  };

  //@{
  /**
   * The glyph type, specified as one of the enumerated values in this
   * class. VERTEX is a special glyph that cannot be scaled, but instead
   * is rendered as an OpenGL vertex primitive. This may appear as a box
   * or circle depending on the hardware.
   */
  vtkSetMacro(GlyphType, int);
  vtkGetMacro(GlyphType, int);
  //@}

  //@{
  /**
   * Whether to fill the glyph, or to just render the outline.
   */
  vtkSetMacro(Filled, bool);
  vtkGetMacro(Filled, bool);
  vtkBooleanMacro(Filled, bool);
  //@}

  //@{
  /**
   * Set the desired screen size of each glyph. If you are using scaling,
   * this will be the size of the glyph when rendering an object with
   * scaling value 1.0.
   */
  vtkSetMacro(ScreenSize, double);
  vtkGetMacro(ScreenSize, double);
  //@}

  //@{
  /**
   * The renderer in which the glyphs will be placed.
   */
  virtual void SetRenderer(vtkRenderer* ren);
  virtual vtkRenderer* GetRenderer();
  //@}

  //@{
  /**
   * Whether to use the input array to process in order to scale the
   * vertices.
   */
  virtual void SetScaling(bool b);
  virtual bool GetScaling();
  //@}

  /**
   * The modified time of this filter.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkGraphToGlyphs();
  ~vtkGraphToGlyphs() VTK_OVERRIDE;

  /**
   * Convert the vtkGraph into vtkPolyData.
   */
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  /**
   * Set the input type of the algorithm to vtkGraph.
   */
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  vtkSmartPointer<vtkGraphToPoints> GraphToPoints;
  vtkSmartPointer<vtkGlyphSource2D> GlyphSource;
  vtkSmartPointer<vtkSphereSource> Sphere;
  vtkSmartPointer<vtkGlyph3D> Glyph;
  vtkSmartPointer<vtkDistanceToCamera> DistanceToCamera;
  int GlyphType;
  bool Filled;
  double ScreenSize;

private:
  vtkGraphToGlyphs(const vtkGraphToGlyphs&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphToGlyphs&) VTK_DELETE_FUNCTION;
};

#endif
