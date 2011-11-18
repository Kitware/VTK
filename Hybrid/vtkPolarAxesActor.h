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
// .NAME vtkPolarAxesActor - create an actor of a polar axes -
// 
// .SECTION Description
// vtkPolarAxesActor is a composite actor that draws polar axes in a 
// specified plane for a give pole. 
// Currently the plane has to be the xy plane.
//
// .SECTION Thanks
// This class was written by:
// Philippe Pébay, Kitware SAS 2011.
//
// .section See Also
// vtkActor vtkAxisActor vtkPolarAxesActor

#ifndef __vtkPolarAxesActor_h
#define __vtkPolarAxesActor_h

#define VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES 50
#define VTK_DEFAULT_NUMBER_OF_RADIAL_AXES 5
#define VTK_MAXIMUM_NUMBER_OF_POLAR_AXIS_TICKS 200
#define VTK_DEFAULT_MAXIMUM_POLAR_ANGLE 90.0
#define VTK_POLAR_ARC_RESOLUTION_PER_DEG 0.2

#include "vtkActor.h"

class vtkAxisActor;
class vtkCamera;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkTextProperty;

class VTK_HYBRID_EXPORT vtkPolarAxesActor : public vtkActor
{
public:
  vtkTypeMacro(vtkPolarAxesActor,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with label format "6.3g" and the number of labels
  // per axis set to 3.
  static vtkPolarAxesActor *New();

  // Description:
  // Draw the polar axes
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*) {return 0;};

  // Description:
  // Explicitly specify the coordinate of the pole.
  // The default coordinates are (0,0,0).
  vtkSetVector3Macro( Pole, double );
  vtkGetVector3Macro( Pole, double );

  // Description:
  // Gets/Sets the number of radial axes
  // Default: VTK_DEFAULT_NUMBER_OF_RADIAL_AXES
  vtkSetClampMacro( NumberOfRadialAxes, vtkIdType, 2, VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES );
  vtkGetMacro( NumberOfRadialAxes, vtkIdType );

  // Description:
  // Gets/Sets the number of ticks and labels along polar axis
  // NB: will be overriden if AutoSubdividePolarAxis is true
  vtkSetClampMacro( NumberOfPolarAxisTicks, vtkIdType, 0, VTK_MAXIMUM_NUMBER_OF_POLAR_AXIS_TICKS );
  vtkGetMacro( NumberOfPolarAxisTicks, vtkIdType );

  // Description:
  // Set/Get whether the number of polar axis ticks and arcs should be automatically calculated
  // Default: true
  vtkSetMacro( AutoSubdividePolarAxis, bool );
  vtkGetMacro( AutoSubdividePolarAxis, bool );

  // Description:
  //  Set/Get the maximum radius of the polar coordinates.
  // Default: VTK_DOUBLE_MAX
  vtkSetClampMacro( MaximumRadius, double, 0., VTK_DOUBLE_MAX );
  vtkGetMacro( MaximumRadius, double );

  // Description:
  // Turn on and off the auto-scaling of the maximum radius.
  // Default: false
  vtkSetMacro( AutoScaleRadius,bool );
  vtkGetMacro( AutoScaleRadius,bool );

  // Description:
  //  Set/Get the minimum radius of the polar coordinates (in degrees).
  // Default: 0.
  vtkSetClampMacro( MinimumAngle, double, 0., 360. );
  vtkGetMacro( MinimumAngle, double );

  // Description:
  //  Set/Get the maximum radius of the polar coordinates (in degrees).
  // Default: VTK_DEFAULT_MAXIMUM_POLAR_ANGLE
  vtkSetClampMacro( MaximumAngle, double, 0., 360. );
  vtkGetMacro( MaximumAngle, double );

  // Description: Set/Get whether angle units (degrees) are used to label radial axes 
  // Default: true
  vtkSetMacro( RadialUnits, bool ); 
  vtkGetMacro( RadialUnits, bool );

  // Description:
  // Explicitly specify the screen size of title and label text.
  // ScreenSize detemines the size of the text in terms of screen
  // pixels. 
  // Default: 10.0.
  void SetScreenSize( double screenSize );
  vtkGetMacro( ScreenSize, double );

  // Description:
  // Set/Get the camera to perform scaling and translation of the
  // vtkPolarAxesActor.
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro( Camera,vtkCamera );

  // Description:
  // Set/Get the labels for the polar axis.
  // Default: "Radial Distance".
  vtkSetStringMacro( PolarAxisTitle );
  vtkGetStringMacro( PolarAxisTitle );

  // Description:
  // Set/Get the format with which to print the polar axis labels
  vtkSetStringMacro( PolarLabelFormat );
  vtkGetStringMacro( PolarLabelFormat );

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources( vtkWindow* );

  // Description:
  // Turn on and off the visibility of the polar axis.
  vtkSetMacro( PolarAxisVisibility,int );
  vtkGetMacro( PolarAxisVisibility,int );
  vtkBooleanMacro( PolarAxisVisibility,int );

  // Description:
  // Turn on and off the visibility of titles for polar axis.
  vtkSetMacro( PolarTitleVisibility,int );
  vtkGetMacro( PolarTitleVisibility,int );
  vtkBooleanMacro( PolarTitleVisibility,int );

  // Description:
  // Turn on and off the visibility of labels for polar axis.
  vtkSetMacro( PolarLabelVisibility,int );
  vtkGetMacro( PolarLabelVisibility,int );
  vtkBooleanMacro( PolarLabelVisibility,int );

  // Description:
  // Turn on and off the visibility of ticks for polar axis.
  vtkSetMacro( PolarTickVisibility, int );
  vtkGetMacro( PolarTickVisibility, int );
  vtkBooleanMacro( PolarTickVisibility, int );

  // Description:
  // Turn on and off the visibility of non-polar radial axes.
  vtkSetMacro( RadialAxesVisibility,int );
  vtkGetMacro( RadialAxesVisibility,int );
  vtkBooleanMacro( RadialAxesVisibility,int );

  // Description:
  // Turn on and off the visibility of titles for non-polar radial axes.
  vtkSetMacro( RadialTitleVisibility,int );
  vtkGetMacro( RadialTitleVisibility,int );
  vtkBooleanMacro( RadialTitleVisibility,int );

  // Description:
  // Turn on and off the visibility of arcs for polar axis.
  vtkSetMacro( PolarArcsVisibility, int );
  vtkGetMacro( PolarArcsVisibility, int );
  vtkBooleanMacro( PolarArcsVisibility, int );

  // Description:
  // Set/Get the polar axis title text property. 
  virtual void SetPolarAxisTitleTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(PolarAxisTitleTextProperty,vtkTextProperty);

  // Description:
  // Set/Get the polar axis labels text property.
  virtual void SetPolarAxisLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(PolarAxisLabelTextProperty,vtkTextProperty);
  
  // Description:
  // Get/Set polar axis actor properties.
  virtual void SetPolarAxisProperty(vtkProperty *);
  vtkProperty* GetPolarAxisProperty();

  // Description:
  // Get/Set radial axes actors properties.
  virtual void SetRadialAxesProperty(vtkProperty *);
  vtkProperty* GetRadialAxesProperty();

  // Description:
  // Get/Set polar arcs actors property
  virtual void SetPolarArcsProperty(vtkProperty *);
  vtkProperty* GetPolarArcsProperty();

  // Description:
  // Explicitly specify the region in space around which to draw the bounds.
  // The bounds are used only when no Input or Prop is specified. The bounds
  // are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
  // sure that the min's are less than the max's.
  vtkSetVector6Macro(Bounds,double);
  double *GetBounds();
  void GetBounds(double& xmin, double& xmax, double& ymin, double& ymax,
                 double& zmin, double& zmax);
  void GetBounds(double bounds[6]);

protected:
  vtkPolarAxesActor();
  ~vtkPolarAxesActor();

  // Description:
  // Transform the bounding box to display coordinates.
  void  TransformBounds( vtkViewport*, double* );

  // Description:
  // Build the axes. 
  // Determine coordinates, position, etc.
  void  BuildAxes( vtkViewport * );

  // Description:
  // Send attributes which are common to all axes, both polar and radial
  void  SetCommonAxisAttributes( vtkAxisActor* );

  // Description:
  // Prepare ticks on polar axis with respect to coordinate offset
  void  BuildPolarAxisTicks( double );

  // Description:
  // Build polar axis labels and arcs with respect to specified pole.
  void  BuildPolarAxisLabelsArcs( double* );

  int Digits(double min, double max );

  double MaxOf(double, double );

  double FFix(double );
  double FSign(double, double );

  // Description:
  // Automatically rescale titles and labels 
  // NB: Current implementation only for perspective projections.
  void AutoScale( vtkViewport* viewport );

  // Description:
  // Coordinates of the pole
  double Pole[3]; 

  // Description:
  // Number of radial axes
  int NumberOfRadialAxes;

  // Description:
  // Number of polar arcs
  int NumberOfPolarAxisTicks;

  // Description:
  // Whether the number of polar axis ticks and arcs should be automatically calculated
  // Default: TRUE
  bool AutoSubdividePolarAxis;

  // Description:
  // Maximum polar radius (minimum is always 0)
  double MaximumRadius;

  // Description:
  // Auto-scale polar radius (with respect to average length scale of x-y bounding box)
  bool AutoScaleRadius;

  // Description:
  // Minimum polar angle
  double MinimumAngle;

  // Description:
  // Maximum polar angle
  double MaximumAngle;

  // Description:
  // Explicit actor bounds
  double Bounds[6];

  // Description:
  // Structures for polar arcs
  vtkPolyData        *PolarArcs;
  vtkPolyDataMapper  *PolarArcsMapper;
  vtkActor           *PolarArcsActor;

  // Description:
  // Camera attached to the polar axes system
  vtkCamera *Camera;

  // Description:
  // Control variables for polar axis
  vtkAxisActor* PolarAxis;

  // Description:
  // Control variables for non-polar radial axes
  vtkAxisActor** RadialAxes;

  // Description:
  // Title to be used for the polar axis
  // NB: Non-polar radial axes use the polar angle as title and have no labels
  char *PolarAxisTitle;
  char  *PolarLabelFormat;

  // Description:
  // Use angle units (degrees) to label radial axes
  bool RadialUnits;

  int TickLocation;

  // Visibility of polar axis and its title, labels, ticks (major only)
  int PolarAxisVisibility;
  int PolarTitleVisibility;
  int PolarLabelVisibility;
  int PolarTickVisibility;

  // Visibility of radial axes and their titles
  int RadialAxesVisibility;
  int RadialTitleVisibility;

  // Visibility of polar arcs
  int PolarArcsVisibility;

  int   RenderCount;

  int RenderSomething;

  double LabelScreenOffset;

  // Text properties of polar axis title and labels
  vtkTextProperty   *PolarAxisTitleTextProperty;
  vtkTextProperty   *PolarAxisLabelTextProperty;

  // General properties of polar axis
  vtkProperty* PolarAxisProperty;

  // General properties of radial axes
  vtkProperty* RadialAxesProperty;

  vtkTimeStamp BuildTime;

  double LabelScale;
  double TitleScale;

  double ScreenSize;

private:
  vtkPolarAxesActor( const vtkPolarAxesActor& ); // Not implemented
  void operator=( const vtkPolarAxesActor& ); // Not implemented
};


#endif
