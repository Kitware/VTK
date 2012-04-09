/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelPlacer.h

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
// .NAME vtkLabelPlacer - place a prioritized hierarchy of labels in screen space
//
// .SECTION Description
// <b>This class is deprecated and will be removed from VTK in a future
// release. Use vtkLabelPlacementMapper instead.</b>
//
// This should probably be a mapper unto itself (given that
// the polydata output could be large and will realistically
// always be iterated over exactly once before being tossed
// for the next frame of the render).
//
// In any event, it takes as input one (or more, eventually)
// vtkLabelHierarchies that represent prioritized lists of
// labels sorted by their placement in space. As output, it
// provides vtkPolyData containing only VTK_QUAD cells, each
// representing a single label from the input. Each quadrilateral
// has cell data indicating what label in the input it
// corresponds to (via an array named "LabelId").

#ifndef __vtkLabelPlacer_h
#define __vtkLabelPlacer_h

#include "vtkPolyDataAlgorithm.h"

class vtkRenderer;
class vtkCoordinate;
class vtkSelectVisiblePoints;

class VTK_RENDERING_EXPORT vtkLabelPlacer : public vtkPolyDataAlgorithm
{
public:
  static vtkLabelPlacer* New();
  vtkTypeMacro(vtkLabelPlacer,vtkPolyDataAlgorithm);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  vtkGetObjectMacro(Renderer,vtkRenderer);
  virtual void SetRenderer( vtkRenderer* );

  vtkGetObjectMacro(AnchorTransform,vtkCoordinate);

  //BTX
  /// Specifications for the placement of the label relative to an anchor point.
  enum LabelGravity
    {
    VerticalBottomBit   = 1,
    VerticalBaselineBit = 2,
    VerticalCenterBit   = 4,
    VerticalTopBit      = 8,
    HorizontalLeftBit   = 16,
    HorizontalCenterBit = 32,
    HorizontalRightBit  = 64,
    VerticalBitMask     = 15,
    HorizontalBitMask   = 112,

    LowerLeft=17,      //!< The anchor is at the lower left corner of the label's bounding box.
    LowerCenter=33,    //!< The anchor is centered left-to-right at the lower edge of the bounding box.
    LowerRight=65,     //!< The anchor is at the lower right corner of the label's bounding box.

    BaselineLeft=18,   //!< The anchor is on the text baseline (or bottom for images) at the left edge of the label's bounding box.
    BaselineCenter=34, //!< The anchor is centered left-to-right at the text baseline of the bounding box, or the bottom for images.
    BaselineRight=66,  //!< The anchor is on the text baseline (or bottom for images) at the right edge of the label's bounding box.

    CenterLeft=20,     //!< The anchor is at the far left edge of the label at the vertical center of the bounding box.
    CenterCenter=36,   //!< The anchor is centered left-to-right at the vertical midpoint of the bounding box.
    CenterRight=68,    //!< The anchor is at the far right edge of the label at the vertical center of the bounding box.

    UpperLeft=24,      //!< The anchor is at the upper left corner of the label's bounding box.
    UpperCenter=40,    //!< The anchor is centered left-to-right at the top edge of the bounding box.
    UpperRight=72      //!< The anchor is at the upper right corner of the label's bounding box.
    };

  /// Coordinate systems that output dataset may use.
  enum OutputCoordinates
    {
    WORLD=0,           //!< Output 3-D world-space coordinates for each label anchor.
    DISPLAY=1          //!< Output 2-D display coordinates for each label anchor (3 components but only 2 are significant).
    };
  //ETX

  // Description:
  // The placement of the label relative to the anchor point.
  virtual void SetGravity( int gravity );
  vtkGetMacro(Gravity,int);

  // Description:
  // The maximum amount of screen space labels can take up before placement
  // terminates.
  vtkSetClampMacro(MaximumLabelFraction,double,0.,1.);
  vtkGetMacro(MaximumLabelFraction,double);

  // Description:
  // The type of iterator used when traversing the labels.
  // May be vtkLabelHierarchy::FRUSTUM or vtkLabelHierarchy::FULL_SORT.
  vtkSetMacro(IteratorType,int);
  vtkGetMacro(IteratorType,int);

  // Description:
  // Set whether, or not, to use unicode strings.
  vtkSetMacro(UseUnicodeStrings,bool);
  vtkGetMacro(UseUnicodeStrings,bool);
  vtkBooleanMacro(UseUnicodeStrings,bool);

  virtual unsigned long GetMTime();

  // Description:
  // Use label anchor point coordinates as normal vectors and eliminate those
  // pointing away from the camera. Valid only when points are on a sphere
  // centered at the origin (such as a 3D geographic view). Off by default.
  vtkGetMacro(PositionsAsNormals,bool);
  vtkSetMacro(PositionsAsNormals,bool);
  vtkBooleanMacro(PositionsAsNormals,bool);

  // Description:
  // Enable drawing spokes (lines) to anchor point coordinates that were perturbed
  // for being coincident with other anchor point coordinates.
  vtkGetMacro(GeneratePerturbedLabelSpokes,bool);
  vtkSetMacro(GeneratePerturbedLabelSpokes,bool);
  vtkBooleanMacro(GeneratePerturbedLabelSpokes,bool);

  // Description:
  // Use the depth buffer to test each label to see if it should not be displayed if
  // it would be occluded by other objects in the scene. Off by default.
  vtkGetMacro(UseDepthBuffer,bool);
  vtkSetMacro(UseDepthBuffer,bool);
  vtkBooleanMacro(UseDepthBuffer,bool);

  // Description:
  // In the second output, output the geometry of the traversed octree nodes.
  vtkGetMacro(OutputTraversedBounds,bool);
  vtkSetMacro(OutputTraversedBounds,bool);
  vtkBooleanMacro(OutputTraversedBounds,bool);

  // Description:
  // Set/get the coordinate system used for output labels.
  // The output datasets may have point coordinates reported in the world space or display space.
  vtkGetMacro(OutputCoordinateSystem,int);
  vtkSetClampMacro(OutputCoordinateSystem,int,WORLD,DISPLAY);
  void OutputCoordinateSystemWorld() { this->SetOutputCoordinateSystem( vtkLabelPlacer::WORLD ); }
  void OutputCoordinateSystemDisplay() { this->SetOutputCoordinateSystem( vtkLabelPlacer::DISPLAY ); }

protected:
  vtkLabelPlacer();
  virtual ~vtkLabelPlacer();

  virtual void SetAnchorTransform( vtkCoordinate* );

  int FillInputPortInformation( int port, vtkInformation* info );
  virtual int RequestData( vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector );

  //BTX
  class Internal;
  Internal* Buckets;
  //ETX

  vtkRenderer* Renderer;
  vtkCoordinate* AnchorTransform;
  vtkSelectVisiblePoints* VisiblePoints;
  int Gravity;
  double MaximumLabelFraction;
  bool PositionsAsNormals;
  bool OutputTraversedBounds;
  bool GeneratePerturbedLabelSpokes;
  bool UseDepthBuffer;
  bool UseUnicodeStrings;

  int LastRendererSize[2];
  double LastCameraPosition[3];
  double LastCameraFocalPoint[3];
  double LastCameraViewUp[3];
  double LastCameraParallelScale;
  int IteratorType;
  int OutputCoordinateSystem;
  
private:
  vtkLabelPlacer( const vtkLabelPlacer& ); // Not implemented.
  void operator = ( const vtkLabelPlacer& ); // Not implemented.
};

#endif // __vtkLabelPlacer_h
