/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Kathleen Bonnell, B Division, Lawrence Livermore Nat'l Laboratory

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen
All rights reserve
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkCubeAxesActor - create a  plot of a bounding box edges -
// used for navigation
// .SECTION Description
// vtkCubeAxesActor is a composite actor that draws axes of the
// bounding box of an input dataset. The axes include labels and titles
// for the x-y-z axes. The algorithm selects which axes to draw based
// on the user-defined 'fly' mode.  (STATIC is default).
// 'STATIC' constructs axes from all edges of the bounding box.
// 'CLOSEST_TRIAD' consists of the three axes x-y-z forming a triad that
// lies closest to the specified camera.
// 'FURTHEST_TRIAD' consists of the three axes x-y-z forming a triad that
// lies furthest from the specified camera.
// 'OUTER_EDGES' is constructed from edges that are on the "exterior" of the
// bounding box, exterior as determined from examining outer edges of the
// bounding box in projection (display) space.
//
// To use this object you must define a bounding box and the camera used
// to render the vtkCubeAxesActor. You can optionally turn on/off labels,
// ticks, gridlines, and set tick location, number of labels, and text to
// use for axis-titles.  A 'corner offset' can also be set.  This allows
// the axes to be set partially away from the actual bounding box to perhaps
// prevent overlap of labels between the various axes.
//
// The Bounds instance variable (an array of six doubles) is used to determine
// the bounding box.
//
// .SECTION Thanks
// This class was written by:
// Hank Childs, Kathleen Bonnell, Amy Squillacote, Brad Whitlock, Will Schroeder, 
// Eric Brugger, Daniel Aguilera, Claire Guilbaud, Nicolas Dolegieviez, 
// Aashish Chaudhary, Philippe Pebay, David Gobbi, David Partyka, Utkarsh Ayachit
// David Cole, Francois Bertel, and Mark Olesen
//
// .section See Also
// vtkActor vtkAxisActor vtkCubeAxesActor2D

#ifndef __vtkCubeAxesActor_h
#define __vtkCubeAxesActor_h

#define VTK_FLY_OUTER_EDGES     0
#define VTK_FLY_CLOSEST_TRIAD   1
#define VTK_FLY_FURTHEST_TRIAD  2
#define VTK_FLY_STATIC_TRIAD    3
#define VTK_FLY_STATIC_EDGES    4

#define VTK_TICKS_INSIDE        0
#define VTK_TICKS_OUTSIDE       1
#define VTK_TICKS_BOTH          2

#define NUMBER_OF_ALIGNED_AXIS 4

#include "vtkActor.h"

class vtkAxisActor;
class vtkCamera;
class vtkTextProperty;

class VTK_HYBRID_EXPORT vtkCubeAxesActor : public vtkActor
{
public:
  vtkTypeMacro(vtkCubeAxesActor,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with label format "6.3g" and the number of labels
  // per axis set to 3.
  static vtkCubeAxesActor *New();

  // Description:
  // Draw the axes as per the vtkProp superclass' API.
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int RenderOverlay(vtkViewport*);
  int HasTranslucentPolygonalGeometry();

  // Description:
  // Gets/Sets the RebuildAxes flag
  vtkSetMacro( RebuildAxes, bool );
  vtkGetMacro( RebuildAxes, bool );

  // Description:
  // Explicitly specify the region in space around which to draw the bounds.
  // The bounds is used only when no Input or Prop is specified. The bounds
  // are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
  // sure that the min's are less than the max's.
  vtkSetVector6Macro(Bounds,double);
  double *GetBounds();
  void GetBounds(double& xmin, double& xmax, double& ymin, double& ymax,
                 double& zmin, double& zmax);
  void GetBounds(double bounds[6]);

  // Description:
  // Explicitly specify the range of each axes that's used to define the prop.
  // The default, (if you do not use these methods) is to use the bounds
  // specified, or use the bounds of the Input Prop if one is specified. This
  // method allows you to separate the notion of extent of the axes in physical
  // space (bounds) and the extent of the values it represents. In other words,
  // you can have the ticks and labels show a different range.
  vtkSetVector2Macro( XAxisRange, double );
  vtkSetVector2Macro( YAxisRange, double );
  vtkSetVector2Macro( ZAxisRange, double );
  vtkGetVector2Macro( XAxisRange, double );
  vtkGetVector2Macro( YAxisRange, double );
  vtkGetVector2Macro( ZAxisRange, double );

  // Description:
  // Explicitly specify the screen size of title and label text.
  // ScreenSize detemines the size of the text in terms of screen
  // pixels. Default is 10.0.
  void SetScreenSize(double screenSize);
  vtkGetMacro(ScreenSize, double);

  // Description:
  // Set/Get the camera to perform scaling and translation of the
  // vtkCubeAxesActor.
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera,vtkCamera);

  // Description:
  // Specify a mode to control how the axes are drawn: either static,
  // closest triad, furthest triad or outer edges in relation to the
  // camera position.
  vtkSetClampMacro(FlyMode, int, VTK_FLY_OUTER_EDGES, VTK_FLY_STATIC_EDGES);
  vtkGetMacro(FlyMode, int);
  void SetFlyModeToOuterEdges()
    {this->SetFlyMode(VTK_FLY_OUTER_EDGES);};
  void SetFlyModeToClosestTriad()
    {this->SetFlyMode(VTK_FLY_CLOSEST_TRIAD);};
  void SetFlyModeToFurthestTriad()
    {this->SetFlyMode(VTK_FLY_FURTHEST_TRIAD);};
  void SetFlyModeToStaticTriad()
    {this->SetFlyMode(VTK_FLY_STATIC_TRIAD);};
  void SetFlyModeToStaticEdges()
    {this->SetFlyMode(VTK_FLY_STATIC_EDGES);};

  // Description:
  // Set/Get the labels for the x, y, and z axes. By default,
  // use "X-Axis", "Y-Axis" and "Z-Axis".
  vtkSetStringMacro(XTitle);
  vtkGetStringMacro(XTitle);
  vtkSetStringMacro(XUnits);
  vtkGetStringMacro(XUnits);
  vtkSetStringMacro(YTitle);
  vtkGetStringMacro(YTitle);
  vtkSetStringMacro(YUnits);
  vtkGetStringMacro(YUnits);
  vtkSetStringMacro(ZTitle);
  vtkGetStringMacro(ZTitle);
  vtkSetStringMacro(ZUnits);
  vtkGetStringMacro(ZUnits);

  // Description:
  // Set/Get the format with which to print the labels on each of the
  // x-y-z axes.
  vtkSetStringMacro(XLabelFormat);
  vtkGetStringMacro(XLabelFormat);
  vtkSetStringMacro(YLabelFormat);
  vtkGetStringMacro(YLabelFormat);
  vtkSetStringMacro(ZLabelFormat);
  vtkGetStringMacro(ZLabelFormat);

  // Description:
  // Set/Get the inertial factor that controls how often (i.e, how
  // many renders) the axes can switch position (jump from one axes
  // to another).
  vtkSetClampMacro(Inertia, int, 1, VTK_LARGE_INTEGER);
  vtkGetMacro(Inertia, int);

  // Description:
  // Specify an offset value to "pull back" the axes from the corner at
  // which they are joined to avoid overlap of axes labels. The
  // "CornerOffset" is the fraction of the axis length to pull back.
  vtkSetMacro(CornerOffset, double);
  vtkGetMacro(CornerOffset, double);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Enable and disable the use of distance based LOD for titles and labels. 
  vtkSetMacro( EnableDistanceLOD, int );
  vtkGetMacro( EnableDistanceLOD, int );
  
  // Description:a
  // Set distance LOD threshold [0.0 - 1.0] for titles and labels. 
  vtkSetClampMacro( DistanceLODThreshold, double, 0.0, 1.0 );
  vtkGetMacro( DistanceLODThreshold, double);

  // Description:
  // Enable and disable the use of view angle based LOD for titles and labels. 
  vtkSetMacro( EnableViewAngleLOD, int );
  vtkGetMacro( EnableViewAngleLOD, int );
  
  // Description:
  // Set view angle LOD threshold [0.0 - 1.0] for titles and labels.
  vtkSetClampMacro( ViewAngleLODThreshold, double, 0., 1. );
  vtkGetMacro( ViewAngleLODThreshold, double );

  // Description:
  // Turn on and off the visibility of each axis.
  vtkSetMacro(XAxisVisibility,int);
  vtkGetMacro(XAxisVisibility,int);
  vtkBooleanMacro(XAxisVisibility,int);
  vtkSetMacro(YAxisVisibility,int);
  vtkGetMacro(YAxisVisibility,int);
  vtkBooleanMacro(YAxisVisibility,int);
  vtkSetMacro(ZAxisVisibility,int);
  vtkGetMacro(ZAxisVisibility,int);
  vtkBooleanMacro(ZAxisVisibility,int);

  // Description:
  // Turn on and off the visibility of labels for each axis.
  vtkSetMacro(XAxisLabelVisibility,int);
  vtkGetMacro(XAxisLabelVisibility,int);
  vtkBooleanMacro(XAxisLabelVisibility,int);

  vtkSetMacro(YAxisLabelVisibility,int);
  vtkGetMacro(YAxisLabelVisibility,int);
  vtkBooleanMacro(YAxisLabelVisibility,int);

  vtkSetMacro(ZAxisLabelVisibility,int);
  vtkGetMacro(ZAxisLabelVisibility,int);
  vtkBooleanMacro(ZAxisLabelVisibility,int);

  // Description:
  // Turn on and off the visibility of ticks for each axis.
  vtkSetMacro(XAxisTickVisibility,int);
  vtkGetMacro(XAxisTickVisibility,int);
  vtkBooleanMacro(XAxisTickVisibility,int);

  vtkSetMacro(YAxisTickVisibility,int);
  vtkGetMacro(YAxisTickVisibility,int);
  vtkBooleanMacro(YAxisTickVisibility,int);

  vtkSetMacro(ZAxisTickVisibility,int);
  vtkGetMacro(ZAxisTickVisibility,int);
  vtkBooleanMacro(ZAxisTickVisibility,int);

  // Description:
  // Turn on and off the visibility of minor ticks for each axis.
  vtkSetMacro(XAxisMinorTickVisibility,int);
  vtkGetMacro(XAxisMinorTickVisibility,int);
  vtkBooleanMacro(XAxisMinorTickVisibility,int);

  vtkSetMacro(YAxisMinorTickVisibility,int);
  vtkGetMacro(YAxisMinorTickVisibility,int);
  vtkBooleanMacro(YAxisMinorTickVisibility,int);

  vtkSetMacro(ZAxisMinorTickVisibility,int);
  vtkGetMacro(ZAxisMinorTickVisibility,int);
  vtkBooleanMacro(ZAxisMinorTickVisibility,int);

  vtkSetMacro(DrawXGridlines,int);
  vtkGetMacro(DrawXGridlines,int);
  vtkBooleanMacro(DrawXGridlines,int);

  vtkSetMacro(DrawYGridlines,int);
  vtkGetMacro(DrawYGridlines,int);
  vtkBooleanMacro(DrawYGridlines,int);

  vtkSetMacro(DrawZGridlines,int);
  vtkGetMacro(DrawZGridlines,int);
  vtkBooleanMacro(DrawZGridlines,int);

  vtkSetMacro(DrawXInnerGridlines,int);
  vtkGetMacro(DrawXInnerGridlines,int);
  vtkBooleanMacro(DrawXInnerGridlines,int);

  vtkSetMacro(DrawYInnerGridlines,int);
  vtkGetMacro(DrawYInnerGridlines,int);
  vtkBooleanMacro(DrawYInnerGridlines,int);

  vtkSetMacro(DrawZInnerGridlines,int);
  vtkGetMacro(DrawZInnerGridlines,int);
  vtkBooleanMacro(DrawZInnerGridlines,int);

  vtkSetMacro(DrawXGridpolys,int);
  vtkGetMacro(DrawXGridpolys,int);
  vtkBooleanMacro(DrawXGridpolys,int);

  vtkSetMacro(DrawYGridpolys,int);
  vtkGetMacro(DrawYGridpolys,int);
  vtkBooleanMacro(DrawYGridpolys,int);

  vtkSetMacro(DrawZGridpolys,int);
  vtkGetMacro(DrawZGridpolys,int);
  vtkBooleanMacro(DrawZGridpolys,int);

  // Description:
  // Returns the text property for the title on an axis.
  vtkTextProperty *GetTitleTextProperty(int);

  // Description:
  // Returns the text property for the labels on an axis.
  vtkTextProperty *GetLabelTextProperty(int);

  // Description:
  // Get/Set axes actors properties.
  void SetXAxesLinesProperty(vtkProperty *);
  vtkProperty* GetXAxesLinesProperty();
  void SetYAxesLinesProperty(vtkProperty *);
  vtkProperty* GetYAxesLinesProperty();
  void SetZAxesLinesProperty(vtkProperty *);
  vtkProperty* GetZAxesLinesProperty();

  // Description:
  // Get/Set axes (outer) gridlines actors properties.
  void SetXAxesGridlinesProperty(vtkProperty *);
  vtkProperty* GetXAxesGridlinesProperty();
  void SetYAxesGridlinesProperty(vtkProperty *);
  vtkProperty* GetYAxesGridlinesProperty();
  void SetZAxesGridlinesProperty(vtkProperty *);
  vtkProperty* GetZAxesGridlinesProperty();
 
  // Description:
  // Get/Set axes inner gridlines actors properties.
  void SetXAxesInnerGridlinesProperty(vtkProperty *);
  vtkProperty* GetXAxesInnerGridlinesProperty();
  void SetYAxesInnerGridlinesProperty(vtkProperty *);
  vtkProperty* GetYAxesInnerGridlinesProperty();
  void SetZAxesInnerGridlinesProperty(vtkProperty *);
  vtkProperty* GetZAxesInnerGridlinesProperty();

  // Description:
  // Get/Set axes gridPolys actors properties.
  void SetXAxesGridpolysProperty(vtkProperty *);
  vtkProperty* GetXAxesGridpolysProperty();
  void SetYAxesGridpolysProperty(vtkProperty *);
  vtkProperty* GetYAxesGridpolysProperty();
  void SetZAxesGridpolysProperty(vtkProperty *);
  vtkProperty* GetZAxesGridpolysProperty();

  // Description:
  // Set/Get the location of ticks marks.
  vtkSetClampMacro(TickLocation, int, VTK_TICKS_INSIDE, VTK_TICKS_BOTH);
  vtkGetMacro(TickLocation, int);

  void SetTickLocationToInside(void)
    { this->SetTickLocation(VTK_TICKS_INSIDE); };
  void SetTickLocationToOutside(void)
    { this->SetTickLocation(VTK_TICKS_OUTSIDE); };
  void SetTickLocationToBoth(void)
    { this->SetTickLocation(VTK_TICKS_BOTH); };

  void SetLabelScaling(bool, int, int, int);

  // Description:
  // Get/Set 2D mode
  // NB: Use vtkTextActor for titles in 2D instead of vtkAxisFollower
  void SetUse2DMode( int val );
  int GetUse2DMode();

  // Description:
  // For 2D mode only: save axis title positions for later use
  void SetSaveTitlePosition( int val );
      
protected:
  vtkCubeAxesActor();
  ~vtkCubeAxesActor();

  int LabelExponent(double min, double max);

  int Digits(double min, double max);

  double MaxOf(double, double);
  double MaxOf(double, double, double, double);

  double FFix(double);
  double FSign(double, double);

  void UpdateLabels(vtkAxisActor **axis, int index);

  double Bounds[6]; //Define bounds explicitly

  vtkCamera *Camera;

  int FlyMode;

  // Description:
  // If enabled the actor will not be visible at a certain distance from the camera.
  // Default is true
  int EnableDistanceLOD;

  // Description:
  // Default is 0.80
  // This determines at what fraction of camera far clip range, actor is not visible.
  double DistanceLODThreshold;

  // Description:
  // If enabled the actor will not be visible at a certain view angle.
  // Default is true.
  int EnableViewAngleLOD;

  // Description:
  // This determines at what view angle to geometry will make the geometry not visibile.
  // Default is 0.3.
  double ViewAngleLODThreshold;

  // Description:
  // Control variables for all axes
  // NB: [0] always for 'Major' axis during non-static fly modes.
  vtkAxisActor *XAxes[NUMBER_OF_ALIGNED_AXIS];
  vtkAxisActor *YAxes[NUMBER_OF_ALIGNED_AXIS];
  vtkAxisActor *ZAxes[NUMBER_OF_ALIGNED_AXIS];

  bool RebuildAxes;

  char *XTitle;
  char *XUnits;
  char *YTitle;
  char *YUnits;
  char *ZTitle;
  char *ZUnits;

  char *ActualXLabel;
  char *ActualYLabel;
  char *ActualZLabel;

  int TickLocation;

  int XAxisVisibility;
  int YAxisVisibility;
  int ZAxisVisibility;

  int XAxisTickVisibility;
  int YAxisTickVisibility;
  int ZAxisTickVisibility;

  int XAxisMinorTickVisibility;
  int YAxisMinorTickVisibility;
  int ZAxisMinorTickVisibility;

  int XAxisLabelVisibility;
  int YAxisLabelVisibility;
  int ZAxisLabelVisibility;

  int DrawXGridlines;
  int DrawYGridlines;
  int DrawZGridlines;

  int DrawXInnerGridlines;
  int DrawYInnerGridlines;
  int DrawZInnerGridlines;

  int DrawXGridpolys;
  int DrawYGridpolys;
  int DrawZGridpolys;

  char  *XLabelFormat;
  char  *YLabelFormat;
  char  *ZLabelFormat;

  double CornerOffset;

  int   Inertia;

  int   RenderCount;

  int   InertiaLocs[3];

  int RenderSomething;

  double LabelScreenOffset;
  double TitleScreenOffset;

  vtkTextProperty* TitleTextProperty[3];
  vtkTextProperty* LabelTextProperty[3];

  vtkProperty  *XAxesLinesProperty;
  vtkProperty  *YAxesLinesProperty;
  vtkProperty  *ZAxesLinesProperty;
  vtkProperty  *XAxesGridlinesProperty;
  vtkProperty  *YAxesGridlinesProperty;
  vtkProperty  *ZAxesGridlinesProperty;
  vtkProperty  *XAxesInnerGridlinesProperty;
  vtkProperty  *YAxesInnerGridlinesProperty;
  vtkProperty  *ZAxesInnerGridlinesProperty;
  vtkProperty  *XAxesGridpolysProperty;
  vtkProperty  *YAxesGridpolysProperty;
  vtkProperty  *ZAxesGridpolysProperty;

private:
  vtkCubeAxesActor(const vtkCubeAxesActor&); // Not implemented
  void operator=(const vtkCubeAxesActor&); // Not implemented

  vtkSetStringMacro(ActualXLabel);
  vtkSetStringMacro(ActualYLabel);
  vtkSetStringMacro(ActualZLabel);

  vtkTimeStamp BuildTime;
  int LastXPow;
  int LastYPow;
  int LastZPow;

  int UserXPow;
  int UserYPow;
  int UserZPow;

  bool AutoLabelScaling;

  int LastXAxisDigits;
  int LastYAxisDigits;
  int LastZAxisDigits;

  double LastXRange[2];
  double LastYRange[2];
  double LastZRange[2];

  int    LastFlyMode;

  int   RenderAxesX[NUMBER_OF_ALIGNED_AXIS];
  int   RenderAxesY[NUMBER_OF_ALIGNED_AXIS];
  int   RenderAxesZ[NUMBER_OF_ALIGNED_AXIS];

  int   NumberOfAxesX;
  int   NumberOfAxesY;
  int   NumberOfAxesZ;

  bool MustAdjustXValue;
  bool MustAdjustYValue;
  bool MustAdjustZValue;

  bool ForceXLabelReset;
  bool ForceYLabelReset;
  bool ForceZLabelReset;

  double XAxisRange[2];
  double YAxisRange[2];
  double ZAxisRange[2];

  double LabelScale;
  double TitleScale;

  double ScreenSize;

  // Description:
  // Major start and delta values, in each direction.
  // These values are needed for inner grid lines generation
  double MajorStart[3];
  double DeltaMajor[3];

  void  TransformBounds(vtkViewport *viewport, const double bounds[6],
                        double pts[8][3]);
  void  AdjustAxes(double bounds[6],
                   double xCoords[NUMBER_OF_ALIGNED_AXIS][6],
                   double yCoords[NUMBER_OF_ALIGNED_AXIS][6],
                   double zCoords[NUMBER_OF_ALIGNED_AXIS][6],
                   double xRange[2], double yRange[2], double zRange[2]);

  bool  ComputeTickSize(double bounds[6]);
  void  AdjustValues(const double xRange[2],
                     const double yRange[2],
                     const double zRange[2]);
  void  AdjustRange(const double bounds[6]);
  void  BuildAxes(vtkViewport *);
  void  DetermineRenderAxes(vtkViewport *);
  void  SetNonDependentAttributes(void);
  void  BuildLabels(vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS]);
  void  AdjustTicksComputeRange(vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS],
      double rangeMin, double rangeMax);

  void    AutoScale(vtkViewport *viewport);
  void    AutoScale(vtkViewport *viewport, vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS]);
  double  AutoScale(vtkViewport *viewport, double screenSize, double position[3]);
};


#endif
