/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContourRepresentation - represent the vtkContourWidget
// .SECTION Description
// The vtkContourRepresentation is a superclass for various types of
// representations for the vtkContourWidget.

// .SECTION See Also
// vtkContourWidget vtkHandleRepresentation 


#ifndef __vtkContourRepresentation_h
#define __vtkContourRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkHandleRepresentation;

class vtkContourRepresentationInternals;
class vtkPointPlacer;
class vtkContourLineInterpolator;
class vtkComputeContourStatistics;
class vtkPolyData;
class vtkContourStatistics;

class VTK_WIDGETS_EXPORT vtkContourRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Standard VTK methods.
  vtkTypeRevisionMacro(vtkContourRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual int AddNodeAtWorldPosition( double worldPos[3] );
  virtual int AddNodeAtWorldPosition( double worldPos[3],
                                      double worldOrient[9] );
  virtual int AddNodeAtDisplayPosition( double displayPos[2] );
  virtual int AddNodeAtDisplayPosition( int displayPos[2] );
  virtual int AddNodeAtDisplayPosition( int X, int Y );

  virtual int ActivateNode( double displayPos[2] );
  virtual int ActivateNode( int displayPos[2] );
  virtual int ActivateNode( int X, int Y );
  
  virtual int SetActiveNodeToWorldPosition( double pos[3] );
  virtual int SetActiveNodeToWorldPosition( double pos[3],
                                            double orient[9] );
  virtual int SetActiveNodeToDisplayPosition( double pos[2] );
  virtual int SetActiveNodeToDisplayPosition( int pos[2] );
  virtual int SetActiveNodeToDisplayPosition( int X, int Y );
  virtual int GetActiveNodeWorldPosition( double pos[3] );
  virtual int GetActiveNodeWorldOrientation( double orient[9] );
  virtual int GetActiveNodeDisplayPosition( double pos[2] );

  virtual int GetNumberOfNodes();
  virtual int GetNthNodeDisplayPosition( int n, double pos[2] );
  virtual int GetNthNodeWorldPosition( int n, double pos[3] );
  virtual int GetNthNodeWorldOrientation( int n, double orient[9] );
  virtual int SetNthNodeDisplayPosition( int n, int X, int Y );
  virtual int SetNthNodeDisplayPosition( int n, int pos[2] );
  virtual int SetNthNodeDisplayPosition( int n, double pos[2] );
  virtual int SetNthNodeWorldPosition( int n, double pos[3] );
  virtual int SetNthNodeWorldPosition( int n, double pos[3],
                                       double orient[9] );
  virtual int  GetNthNodeSlope( int idx, double slope[3] );
  
  virtual int GetNumberOfIntermediatePoints( int n );
  virtual int GetIntermediatePointWorldPosition( int n, 
                                                 int idx, double point[3] );
  virtual int AddIntermediatePointWorldPosition( int n, 
                                                 double point[3] );

  virtual int DeleteLastNode();
  virtual int DeleteActiveNode();
  virtual int DeleteNthNode( int n );

  virtual int AddNodeOnContour( int X, int Y );
  
  // Description:
  // The tolerance representing the distance to the widget (in pixels) in
  // which the cursor is considered near enough to the end points of
  // the widget to be active.
  vtkSetClampMacro(PixelTolerance,int,1,100);
  vtkGetMacro(PixelTolerance,int);

  vtkSetClampMacro(WorldTolerance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(WorldTolerance, double);

//BTX -- used to communicate about the state of the representation
  enum {
    Outside=0,
    Nearby
  };
  
  enum {
    Inactive = 0,
    Translate
  };
//ETX

  vtkGetMacro( CurrentOperation, int );
  vtkSetClampMacro( CurrentOperation, int, 
                    vtkContourRepresentation::Inactive,
                    vtkContourRepresentation::Translate );
  void SetCurrentOperationToInactive()
    { this->SetCurrentOperation( vtkContourRepresentation::Inactive ); }
  void SetCurrentOperationToTranslate()
    { this->SetCurrentOperation( vtkContourRepresentation::Translate ); }

  void SetPointPlacer( vtkPointPlacer * );
  vtkGetObjectMacro( PointPlacer, vtkPointPlacer );
  
  void SetLineInterpolator( vtkContourLineInterpolator *);
  vtkGetObjectMacro( LineInterpolator, vtkContourLineInterpolator );
  
  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void BuildRepresentation()=0;
  virtual int ComputeInteractionState(int X, int Y, int modified=0)=0;
  virtual void StartWidgetInteraction(double e[2])=0;
  virtual void WidgetInteraction(double e[2])=0;

  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w)=0;
  virtual int RenderOverlay(vtkViewport *viewport)=0;
  virtual int RenderOpaqueGeometry(vtkViewport *viewport)=0;
  virtual int RenderTranslucentGeometry(vtkViewport *viewport)=0;

  void SetClosedLoop( int val );
  vtkGetMacro( ClosedLoop, int );
  vtkBooleanMacro( ClosedLoop, int );
  
  // Description:
  // Get the points in this contour as a vtkPolyData. 
  virtual const vtkPolyData * const GetContourRepresentationAsPolyData() const = 0;

  // Description:
  // Should we compute statistics or not ? 1 = yes, 0 = no.
  // If true, the contour statistics will be calculated. Typically the user 
  // will need to set this flag to on and check for 
  // vtkCommand::WidgetValueChangedEvent (signalled when the contour is first 
  // closed) or vtkCommand::EndInteractionEvent (signalled every time the contour
  // is interacted with) to compute the statistics. If the contour is not closed,
  // values returned are 0. 
  void ComputeStatisticsOn();
  void ComputeStatisticsOff();
  void SetComputeStatistics( int );
  vtkGetMacro( ComputeStatistics, int );

  // Description::
  // Get the area of the contour. A value of 0 is returned if the contour
  // is not closed.
  double GetArea();

  // Description::
  // Get the area of the contour. A value of 0 is returned if the contour
  // is not closed.
  double GetPerimeter();

  // Description::
  // Get the NSI of the contour. A value of 0 is returned if the contour
  // is not closed. See vtkMassProperties for info on NSI.
  double GetNormalizedShapeIndex();

protected:
  vtkContourRepresentation();
  ~vtkContourRepresentation();

//BTX  
  friend class vtkContourWidget;
//ETX
  
  // Selection tolerance for the handles
  int    PixelTolerance;
  double WorldTolerance;

  vtkPointPlacer             *PointPlacer;
  vtkContourLineInterpolator *LineInterpolator;
  
  int ActiveNode;
  
  int CurrentOperation;
  int ClosedLoop;
  
  vtkContourRepresentationInternals *Internal;

  void AddNodeAtWorldPositionInternal( double worldPos[3],
                                       double worldOrient[9] );
  void SetNthNodeWorldPositionInternal( int n, double worldPos[3],
                                        double worldOrient[9] );

  void UpdateLines( int index );
  void UpdateLine( int idx1, int idx2 );

  int FindClosestPointOnContour( int X, int Y, 
                                 double worldPos[3],
                                 int *idx );
  
  virtual void BuildLines()=0;

  void ComputeMidpoint( double p1[3], double p2[3], double mid[3] )
    {
      mid[0] = (p1[0] + p2[0])/2;
      mid[1] = (p1[1] + p2[1])/2;
      mid[2] = (p1[2] + p2[2])/2;
    }
  
  // Description:
  // Convenience method to pass the poly data to the ContourStatistics 
  // calculator. 
  void AssignPolyDataToStatisticsCalculator();

  // helper class that computes statistics on this countour
  vtkContourStatistics *ContourStatistics;
  
  // Contour statistics are computed if this ivar is 1. (0 by default.)
  int ComputeStatistics; 
  
private:
  vtkContourRepresentation(const vtkContourRepresentation&);  //Not implemented
  void operator=(const vtkContourRepresentation&);  //Not implemented
};

#endif
