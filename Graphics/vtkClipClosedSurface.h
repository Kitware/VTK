/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipClosedSurface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClipClosedSurface - Clip a closed surface with a plane collection
// .SECTION Description
// vtkClipClosedSurface will clip a closed polydata surface with a
// collection of clipping planes.  It will produce a new closed surface
// by creating new polygonal faces where the input data was clipped.
// If GenerateOutline is on, it will also generate an outline wherever
// the clipping planes intersect the data.  The GenerateColorScalars option
// will add color scalars to the output, so that the generated faces
// can be visualized in a different color from the original surface.
// .SECTION See Also
// vtkOutlineFilter vtkOutlineSource vtkVolumeOutlineSource
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef __vtkClipClosedSurface_h
#define __vtkClipClosedSurface_h

#include "vtkPolyDataAlgorithm.h"

class vtkPlaneCollection;
class vtkUnsignedCharArray;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkCellArray;
class vtkPointData;
class vtkCellData;
class vtkIncrementalPointLocator;
class vtkGenericCell;
class vtkPolygon;
class vtkIdList;

class VTK_GRAPHICS_EXPORT vtkClipClosedSurface : public vtkPolyDataAlgorithm
{
public:
  static vtkClipClosedSurface *New();
  vtkTypeRevisionMacro(vtkClipClosedSurface,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the vtkPlaneCollection that holds the clipping planes.
  virtual void SetClippingPlanes(vtkPlaneCollection *planes);
  vtkGetObjectMacro(ClippingPlanes,vtkPlaneCollection);

  // Description:
  // Set whether to add cell scalars, so that the new faces and
  // outlines can be distinguished from the original faces and outlines.
  // This is off by default.
  vtkSetMacro(GenerateColorScalars, int);
  vtkBooleanMacro(GenerateColorScalars, int);
  vtkGetMacro(GenerateColorScalars, int);

  // Description:
  // Set whether to generate an outline wherever an input face was
  // cut by a plane.  This is off by default. 
  vtkSetMacro(GenerateOutline, int);
  vtkBooleanMacro(GenerateOutline, int);
  vtkGetMacro(GenerateOutline, int);

  // Description:
  // Set whether to generate polygonal faces for the output.  This is
  // on by default.  If it is off, then the output will have no polys.
  vtkSetMacro(GenerateFaces, int);
  vtkBooleanMacro(GenerateFaces, int);
  vtkGetMacro(GenerateFaces, int);

  // Description:
  // Set the color for all cells were part of the original geometry.
  // If the the input data already has color cell scalars, then those
  // values will be used and parameter will be ignored.  The default color
  // is red.  Requires GenerateColorScalars to be on.
  vtkSetVector3Macro(BaseColor, double);
  vtkGetVector3Macro(BaseColor, double);

  // Description:
  // Set the color for any new geometry, either faces or outlines, that are
  // created as a result of the clipping. The default color is orange.
  // Requires GenerateColorScalars to be on.
  vtkSetVector3Macro(ClipColor, double);
  vtkGetVector3Macro(ClipColor, double);

  // Description:
  // Set the active plane, so that the clipping from that plane can be
  // displayed in a different color.  Set this to -1 if there is no active
  // plane.  The default value is -1.
  vtkSetMacro(ActivePlaneId, int);
  vtkGetMacro(ActivePlaneId, int);

  // Description:
  // Set the color for any new geometry produced by clipping with the
  // ActivePlane, if ActivePlaneId is set.  Default is yellow.  Requires
  // GenerateColorScalars to be on.
  vtkSetVector3Macro(ActivePlaneColor, double);
  vtkGetVector3Macro(ActivePlaneColor, double);

protected:
  vtkClipClosedSurface();
  ~vtkClipClosedSurface();

  vtkPlaneCollection *ClippingPlanes;

  int GenerateColorScalars;
  int GenerateOutline;
  int GenerateFaces;
  int ActivePlaneId;
  double BaseColor[3];
  double ClipColor[3];
  double ActivePlaneColor[3];

  vtkIncrementalPointLocator *Locator;

  vtkDoubleArray *CellClipScalars;
  vtkIdList *IdList;
  vtkCellArray *CellArray;
  vtkPolygon *Polygon;
  vtkGenericCell *Cell;

  virtual int ComputePipelineMTime(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, int requestFromOutputPort,
    unsigned long* mtime);

  virtual int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Combine clipping and contouring of cells into one single method.
  // The inputCellDimensionality must be set to the dimensionality of
  // the inputCells (1 or 2), and the inputCell must be either
  // lines/polylines or polys/quads/tris. 
  void ClipAndContourCells(
    vtkPoints *points, vtkDoubleArray *pointScalars,
    vtkIncrementalPointLocator *locator, int inputCellDimensionality,
    vtkCellArray *inputCells, vtkCellArray *outputPolys,
    vtkCellArray *outputLines, vtkPointData *inPointData,
    vtkPointData *outPointData, vtkCellData *inCellData,
    vtkCellData *outPolyData, vtkCellData *outLineData);

  // Description:
  // Given some closed contour lines, create a triangle mesh that
  // fills those lines.  The input lines must be single-segment lines,
  // not polylines.  The input lines do not have to be in order.
  // Only lines from firstLine onward will be used.  Specify the normal
  // of the clip plane, which will be opposite the the normals
  // of the polys that will be produced.  If outCD has scalars, then color
  // scalars will be added for each poly that is created.
  void MakeCutPolys(
    vtkPoints *points, vtkCellArray *inputLines, vtkIdType firstLine,
    vtkCellArray *outputPolys, const double normal[3], vtkCellData *outCD,
    const unsigned char color[3]);

  // Description:
  // Break polylines into individual lines, copying scalar values from
  // inputScalars starting at firstLineScalar.  If inputScalars is zero,
  // then scalars will be set to color.  If scalars is zero, then no
  // scalars will be generated.
  static void BreakPolylines(
    vtkCellArray *inputLines, vtkCellArray *outputLines,
    vtkUnsignedCharArray *inputScalars, vtkIdType firstLineScalar,
    vtkUnsignedCharArray *outputScalars, const unsigned char color[3]);

  // Description:
  // Copy polygons and their associated scalars to a new array.
  // If inputScalars is set to zero, set polyScalars to color instead.
  // If polyScalars is set to zero, don't generate scalars.
  static void CopyPolygons(
    vtkCellArray *inputPolys, vtkCellArray *outputPolys,
    vtkUnsignedCharArray *inputScalars, vtkIdType firstPolyScalar,
    vtkUnsignedCharArray *outputScalars, const unsigned char color[3]);

  // Description:
  // Break triangle strips and add the triangles to the output. See
  // CopyPolygons for more information.
  static void BreakTriangleStrips(
    vtkCellArray *inputStrips, vtkCellArray *outputPolys,
    vtkUnsignedCharArray *inputScalars, vtkIdType firstStripScalar,
    vtkUnsignedCharArray *outputScalars, const unsigned char color[3]);

  // Description:
  // Take three colors as doubles, and convert to unsigned char.
  static void CreateColorValues(
    const double color1[3], const double color2[3], const double color3[3],
    unsigned char colors[3][3]);

private:
  vtkClipClosedSurface(const vtkClipClosedSurface&);  // Not implemented.
  void operator=(const vtkClipClosedSurface&);  // Not implemented.
};

#endif
