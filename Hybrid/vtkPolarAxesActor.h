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

#define VTK_DEFAULT_NUMBER_OF_RADIAL_AXES 5
#define VTK_DEFAULT_MINIMUM_POLAR_ANGLE 0.0
#define VTK_DEFAULT_MAXIMUM_POLAR_ANGLE 90.0

#include "vtkActor.h"

class vtkAxisActor;
class vtkCamera;

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
  double *GetPole();
  void GetPole( double& x, double& y, double& z );
  void GetPole( double bounds[3] );

  // Description:
  // Gets/Sets the number of radial axes
  // Default: VTK_DEFAULT_NUMBER_OF_RADIAL_AXES
  vtkSetMacro( NumberOfRadialAxes, vtkIdType );
  vtkGetMacro( NumberOfRadialAxes, vtkIdType );

  // Description:
  //  Set/Get the maximum radiuss of the polar coordinates.
  // Default: VTK_DOUBLE_MAX
  vtkSetMacro( MaximumRadius, double );
  vtkGetMacro( MaximumRadius, double );

  // Description:
  // Set/Get the angular range of the polar coordinates.
  // Default: from VTK_DEFAULT_MINIMUM_POLAR_ANGLE to VTK_DEFAULT_MAXIMUM_POLAR_ANGLE
  vtkSetVector2Macro( AngularRange, double );
  vtkGetVector2Macro( AngularRange, double );

  // Description:
  // Set/Get the RebuildAxes flag
  vtkSetMacro( RebuildAxes, bool );
  vtkGetMacro( RebuildAxes, bool );

  // Description:
  // Explicitly specify the screen size of title and label text.
  // ScreenSize detemines the size of the text in terms of screen
  // pixels. 
  // Default is 10.0.
  void SetScreenSize( double screenSize );
  vtkGetMacro( ScreenSize, double );

  // Description:
  // Set/Get the camera to perform scaling and translation of the
  // vtkPolarAxesActor.
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro( Camera,vtkCamera );

  // Description:
  // Set/Get the format with which to print the labels on each of the
  // radial axes.
  vtkSetStringMacro( RadialLabelFormat );
  vtkGetStringMacro( RadialLabelFormat );

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Implementation of ShallowCopy() of a vtkPolarAxesActor instance
  void ShallowCopy(vtkPolarAxesActor *actor);

  // Description:
  // Implementation of ShallowCopy() of a vtkPolarAxesActor property instance
  void ShallowCopy(vtkProp *prop) { this->vtkProp::ShallowCopy( prop ); };

  // Description:
  // Turn on and off the visibility of radial axes.
  vtkSetMacro( RadialAxesVisibility,int );
  vtkGetMacro( RadialAxesVisibility,int );
  vtkBooleanMacro( RadialAxesVisibility,int );

  // Description:
  // Turn on and off the visibility of labels for radial axes.
  vtkSetMacro( RadialLabelVisibility,int );
  vtkGetMacro( RadialLabelVisibility,int );
  vtkBooleanMacro( RadialLabelVisibility,int );

  // Description:
  // Turn on and off the visibility of ticks for radial axes.
  vtkSetMacro( RadialTickVisibility, int );
  vtkGetMacro( RadialTickVisibility, int );
  vtkBooleanMacro( RadialTickVisibility, int );

  // Description:
  // Get/Set radial axes actors properties.
  void SetRadialAxesProperty(vtkProperty *);
  vtkProperty* GetRadialAxesProperty();

  void SetLabelScaling(bool, int, int, int);

  void  TransformBounds(vtkViewport *viewport, const double bounds[6],
                        double pts[8][3]);

  bool  ComputeTickSize(double bounds[6]);
  void  AdjustValues(const double xRange[2],
                     const double yRange[2],
                     const double zRange[2]);
  void  AdjustRange(const double bounds[6]);
  void  BuildAxes(vtkViewport *);
  void  SetNonDependentAttributes(void);
  void  BuildLabels( vtkAxisActor** axes );
  void  AdjustTicksComputeRange(vtkAxisActor** axes, double rangeMin, double rangeMax);

  void    AutoScale(vtkViewport *viewport);
  void    AutoScale(vtkViewport *viewport, vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS]);
  double  AutoScale(vtkViewport *viewport, double screenSize, double position[3]);

protected:
  vtkPolarAxesActor();
  ~vtkPolarAxesActor();

  int LabelExponent(double min, double max);

  int Digits(double min, double max);

  double MaxOf(double, double);
  double MaxOf(double, double, double, double);

  double FFix(double);
  double FSign(double, double);

  void UpdateLabels(vtkAxisActor **axis, int index);

  // Description:
  // Coordinates of the pole
  double Pole[3]; 

  // Description:
  // Number of radial axes
  int NumberOfRadialAxes;

  // Description:
  // Maximum polar radius (minimum is always 0)
  double MaximumRadius;
  double LastMaximumRadius;

  vtkCamera *Camera;

  // Description:
  // Control variables for radial axes
  vtkAxisActor** RadialAxes;

  bool RebuildAxes;

  char *RadialUnits;

  char *ActualRadialLabel;

  int TickLocation;

  // Visibility of radial axe, ticks, and labels
  int RadialAxesVisibility;
  int RadialLabelVisibility;
  int RadialTickVisibility;

  char  *RadialLabelFormat;

  int   RenderCount;

  int RenderSomething;

  double LabelScreenOffset;

  vtkProperty* RadialAxesProperty;

  vtkSetStringMacro( ActualRadialLabel );

  vtkTimeStamp BuildTime;
  int LastRadialPow;

  int UserRadialPow;

  bool AutoLabelScaling;

  int LastRadialAxesDigits;

  bool MustAdjustRadialValue;

  bool ForceRadialLabelReset;

  double LabelScale;

  double ScreenSize;

private:
  vtkPolarAxesActor( const vtkPolarAxesActor& ); // Not implemented
  void operator=( const vtkPolarAxesActor& ); // Not implemented
};


#endif
