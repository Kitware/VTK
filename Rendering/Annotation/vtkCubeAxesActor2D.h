/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCubeAxesActor2D
 * @brief   create a 2D plot of a bounding box edges - used for navigation
 *
 * vtkCubeAxesActor2D is a composite actor that draws three axes of the
 * bounding box of an input dataset. The axes include labels and titles
 * for the x-y-z axes. The algorithm selects the axes that are on the
 * "exterior" of the bounding box, exterior as determined from examining
 * outer edges of the bounding box in projection (display) space. Alternatively,
 * the edges closest to the viewer (i.e., camera position) can be drawn.
 *
 * To use this object you must define a bounding box and the camera used
 * to render the vtkCubeAxesActor2D. The camera is used to control the
 * scaling and position of the vtkCubeAxesActor2D so that it fits in the
 * viewport and always remains visible.)
 *
 * The font property of the axes titles and labels can be modified through the
 * AxisTitleTextProperty and AxisLabelTextProperty attributes. You may also
 * use the GetXAxisActor2D, GetYAxisActor2D or GetZAxisActor2D methods
 * to access each individual axis actor to modify their font properties.
 *
 * The bounding box to use is defined in one of three ways. First, if the Input
 * ivar is defined, then the input dataset's bounds is used. If the Input is
 * not defined, and the Prop (superclass of all actors) is defined, then the
 * Prop's bounds is used. If neither the Input or Prop is defined, then the
 * Bounds instance variable (an array of six doubles) is used.
 *
 * @sa
 * vtkActor2D vtkAxisActor2D vtkXYPlotActor vtkTextProperty
*/

#ifndef vtkCubeAxesActor2D_h
#define vtkCubeAxesActor2D_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkActor2D.h"

class vtkAlgorithmOutput;
class vtkAxisActor2D;
class vtkCamera;
class vtkCubeAxesActor2DConnection;
class vtkDataSet;
class vtkTextProperty;

class VTKRENDERINGANNOTATION_EXPORT vtkCubeAxesActor2D : public vtkActor2D
{
public:
  vtkTypeMacro(vtkCubeAxesActor2D,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Instantiate object with bold, italic, and shadow enabled; font family
   * set to Arial; and label format "6.3g". The number of labels per axis
   * is set to 3.
   */
  static vtkCubeAxesActor2D *New();

  //@{
  /**
   * Draw the axes as per the vtkProp superclass' API.
   */
  int RenderOverlay(vtkViewport*) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport*) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *) VTK_OVERRIDE {return 0;}
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  //@{
  /**
   * Use the bounding box of this input dataset to draw the cube axes. If this
   * is not specified, then the class will attempt to determine the bounds from
   * the defined Prop or Bounds.
   */
  virtual void SetInputConnection(vtkAlgorithmOutput*);
  virtual void SetInputData(vtkDataSet*);
  virtual vtkDataSet* GetInput();
  //@}

  //@{
  /**
   * Use the bounding box of this prop to draw the cube axes. The
   * ViewProp is used to determine the bounds only if the Input is not
   * defined.
   */
  void SetViewProp(vtkProp* prop);
  vtkGetObjectMacro(ViewProp, vtkProp);
  //@}

  //@{
  /**
   * Explicitly specify the region in space around which to draw the bounds.
   * The bounds is used only when no Input or Prop is specified. The bounds
   * are specified according to (xmin,xmax, ymin,ymax, zmin,zmax), making
   * sure that the min's are less than the max's.
   */
  vtkSetVector6Macro(Bounds,double);
  double *GetBounds() VTK_OVERRIDE;
  void GetBounds(double& xmin, double& xmax, double& ymin, double& ymax,
                 double& zmin, double& zmax);
  void GetBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Explicitly specify the range of values used on the bounds.
   * The ranges are specified according to (xmin,xmax, ymin,ymax, zmin,zmax),
   * making sure that the min's are less than the max's.
   */
  vtkSetVector6Macro(Ranges,double);
  double *GetRanges();
  void GetRanges(double& xmin, double& xmax, double& ymin, double& ymax,
                 double& zmin, double& zmax);
  void GetRanges(double ranges[6]);
  //@}

  //@{
  /**
   * Explicitly specify an origin for the axes. These usually intersect at one of the
   * corners of the bounding box, however users have the option to override this if
   * necessary
   */
  vtkSetMacro( XOrigin, double );
  vtkSetMacro( YOrigin, double );
  vtkSetMacro( ZOrigin, double );
  //@}

  //@{
  /**
   * Set/Get a flag that controls whether the axes use the data ranges
   * or the ranges set by SetRanges. By default the axes use the data
   * ranges.
   */
  vtkSetMacro(UseRanges,int);
  vtkGetMacro(UseRanges,int);
  vtkBooleanMacro(UseRanges,int);
  //@}

  //@{
  /**
   * Set/Get the camera to perform scaling and translation of the
   * vtkCubeAxesActor2D.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera,vtkCamera);
  //@}

  enum FlyMode
  {
    VTK_FLY_OUTER_EDGES = 0,
    VTK_FLY_CLOSEST_TRIAD = 1,
    VTK_FLY_NONE = 2
  };

  //@{
  /**
   * Specify a mode to control how the axes are drawn: either outer edges
   * or closest triad to the camera position, or you may also disable flying
   * of the axes.
   */
  vtkSetClampMacro(FlyMode, int, VTK_FLY_OUTER_EDGES, VTK_FLY_NONE);
  vtkGetMacro(FlyMode, int);
  void SetFlyModeToOuterEdges()
    {this->SetFlyMode(VTK_FLY_OUTER_EDGES);};
  void SetFlyModeToClosestTriad()
    {this->SetFlyMode(VTK_FLY_CLOSEST_TRIAD);};
  void SetFlyModeToNone()
    {this->SetFlyMode(VTK_FLY_NONE);};
  //@}

  //@{
  /**
   * Set/Get a flag that controls whether the axes are scaled to fit in
   * the viewport. If off, the axes size remains constant (i.e., stay the
   * size of the bounding box). By default scaling is on so the axes are
   * scaled to fit inside the viewport.
   */
  vtkSetMacro(Scaling,int);
  vtkGetMacro(Scaling,int);
  vtkBooleanMacro(Scaling,int);
  //@}

  //@{
  /**
   * Set/Get the number of annotation labels to show along the x, y, and
   * z axes. This values is a suggestion: the number of labels may vary
   * depending on the particulars of the data.
   */
  vtkSetClampMacro(NumberOfLabels, int, 0, 50);
  vtkGetMacro(NumberOfLabels, int);
  //@}

  //@{
  /**
   * Set/Get the labels for the x, y, and z axes. By default,
   * use "X", "Y" and "Z".
   */
  vtkSetStringMacro(XLabel);
  vtkGetStringMacro(XLabel);
  vtkSetStringMacro(YLabel);
  vtkGetStringMacro(YLabel);
  vtkSetStringMacro(ZLabel);
  vtkGetStringMacro(ZLabel);
  //@}

  /**
   * Retrieve handles to the X, Y and Z axis (so that you can set their text
   * properties for example)
   */
  vtkAxisActor2D *GetXAxisActor2D()
    {return this->XAxis;}
  vtkAxisActor2D *GetYAxisActor2D()
    {return this->YAxis;}
  vtkAxisActor2D *GetZAxisActor2D()
    {return this->ZAxis;}

  //@{
  /**
   * Set/Get the title text property of all axes. Note that each axis can
   * be controlled individually through the GetX/Y/ZAxisActor2D() methods.
   */
  virtual void SetAxisTitleTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(AxisTitleTextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the labels text property of all axes. Note that each axis can
   * be controlled individually through the GetX/Y/ZAxisActor2D() methods.
   */
  virtual void SetAxisLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(AxisLabelTextProperty,vtkTextProperty);
  //@}

  //@{
  /**
   * Set/Get the format with which to print the labels on each of the
   * x-y-z axes.
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set/Get the factor that controls the overall size of the fonts used
   * to label and title the axes.
   */
  vtkSetClampMacro(FontFactor, double, 0.1, 2.0);
  vtkGetMacro(FontFactor, double);
  //@}

  //@{
  /**
   * Set/Get the inertial factor that controls how often (i.e, how
   * many renders) the axes can switch position (jump from one axes
   * to another).
   */
  vtkSetClampMacro(Inertia, int, 1, VTK_INT_MAX);
  vtkGetMacro(Inertia, int);
  //@}

  //@{
  /**
   * Set/Get the variable that controls whether the actual
   * bounds of the dataset are always shown. Setting this variable
   * to 1 means that clipping is disabled and that the actual
   * value of the bounds is displayed even with corner offsets
   * Setting this variable to 0 means these axis will clip
   * themselves and show variable bounds (legacy mode)
   */
  vtkSetClampMacro(ShowActualBounds, int, 0, 1);
  vtkGetMacro(ShowActualBounds, int);
  //@}

  //@{
  /**
   * Specify an offset value to "pull back" the axes from the corner at
   * which they are joined to avoid overlap of axes labels. The
   * "CornerOffset" is the fraction of the axis length to pull back.
   */
  vtkSetMacro(CornerOffset, double);
  vtkGetMacro(CornerOffset, double);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  //@{
  /**
   * Turn on and off the visibility of each axis.
   */
  vtkSetMacro(XAxisVisibility,int);
  vtkGetMacro(XAxisVisibility,int);
  vtkBooleanMacro(XAxisVisibility,int);
  vtkSetMacro(YAxisVisibility,int);
  vtkGetMacro(YAxisVisibility,int);
  vtkBooleanMacro(YAxisVisibility,int);
  vtkSetMacro(ZAxisVisibility,int);
  vtkGetMacro(ZAxisVisibility,int);
  vtkBooleanMacro(ZAxisVisibility,int);
  //@}

  /**
   * Shallow copy of a CubeAxesActor2D.
   */
  void ShallowCopy(vtkCubeAxesActor2D *actor);

protected:
  vtkCubeAxesActor2D();
  ~vtkCubeAxesActor2D() VTK_OVERRIDE;

  vtkCubeAxesActor2DConnection* ConnectionHolder;

  vtkProp    *ViewProp;     //Define bounds from actor/assembly, or
  double      Bounds[6]; //Define bounds explicitly
  double      Ranges[6]; //Define ranges explicitly
  int        UseRanges; //Flag to use ranges or not

  vtkCamera *Camera;
  int FlyMode;
  int Scaling;

  vtkAxisActor2D *XAxis;
  vtkAxisActor2D *YAxis;
  vtkAxisActor2D *ZAxis;

  vtkTextProperty *AxisTitleTextProperty;
  vtkTextProperty *AxisLabelTextProperty;

  vtkTimeStamp  BuildTime;

  int   NumberOfLabels;
  char *XLabel;
  char *YLabel;
  char *ZLabel;
  char *Labels[3];

  int XAxisVisibility;
  int YAxisVisibility;
  int ZAxisVisibility;

  char  *LabelFormat;
  double FontFactor;
  double CornerOffset;
  int   Inertia;
  int   RenderCount;
  int   InertiaAxes[8];

  int RenderSomething;

  // Always show the actual bounds of the object
  int ShowActualBounds;

  double XOrigin;
  double YOrigin;
  double ZOrigin;

  // various helper methods
  void TransformBounds(vtkViewport *viewport, double bounds[6],
                       double pts[8][3]);
  int ClipBounds(vtkViewport *viewport, double pts[8][3], double bounds[6]);
  double EvaluatePoint(double planes[24], double x[3]);
  double EvaluateBounds(double planes[24], double bounds[6]);
  void AdjustAxes(double pts[8][3], double bounds[6],
                  int idx, int xIdx, int yIdx, int zIdx, int zIdx2,
                  int xAxes, int yAxes, int zAxes,
                  double xCoords[4], double yCoords[4], double zCoords[4],
                  double xRange[2], double yRange[2], double zRange[2]);

private:
  // hide the superclass' ShallowCopy() from the user and the compiler.
  void ShallowCopy(vtkProp *prop) VTK_OVERRIDE { this->vtkProp::ShallowCopy( prop ); };
private:
  vtkCubeAxesActor2D(const vtkCubeAxesActor2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCubeAxesActor2D&) VTK_DELETE_FUNCTION;
};


#endif
