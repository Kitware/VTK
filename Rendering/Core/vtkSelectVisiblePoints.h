/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectVisiblePoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectVisiblePoints - extract points that are visible (based on z-buffer calculation)
// .SECTION Description
// vtkSelectVisiblePoints is a filter that selects points based on
// whether they are visible or not. Visibility is determined by
// accessing the z-buffer of a rendering window. (The position of each
// input point is converted into display coordinates, and then the
// z-value at that point is obtained. If within the user-specified
// tolerance, the point is considered visible.)
//
// Points that are visible (or if the ivar SelectInvisible is on,
// invisible points) are passed to the output. Associated data
// attributes are passed to the output as well.
//
// This filter also allows you to specify a rectangular window in display
// (pixel) coordinates in which the visible points must lie. This can be
// used as a sort of local "brushing" operation to select just data within
// a window.
//

// .SECTION Caveats
// You must carefully synchronize the execution of this filter. The
// filter refers to a renderer, which is modified every time a render
// occurs. Therefore, the filter is always out of date, and always
// executes. You may have to perform two rendering passes, or if you
// are using this filter in conjunction with vtkLabeledDataMapper,
// things work out because 2D rendering occurs after the 3D rendering.

#ifndef __vtkSelectVisiblePoints_h
#define __vtkSelectVisiblePoints_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkRenderer;
class vtkMatrix4x4;

class VTKRENDERINGCORE_EXPORT vtkSelectVisiblePoints : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSelectVisiblePoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with no renderer; window selection turned off;
  // tolerance set to 0.01; and select invisible off.
  static vtkSelectVisiblePoints *New();

  // Description:
  // Specify the renderer in which the visibility computation is to be
  // performed.
  void SetRenderer(vtkRenderer* ren)
    {
      if (this->Renderer != ren)
        {
        this->Renderer = ren;
        this->Modified();
        }
    }
  vtkRenderer* GetRenderer() { return this->Renderer; }

  // Description:
  // Set/Get the flag which enables selection in a rectangular display
  // region.
  vtkSetMacro(SelectionWindow,int);
  vtkGetMacro(SelectionWindow,int);
  vtkBooleanMacro(SelectionWindow,int);

  // Description:
  // Specify the selection window in display coordinates. You must specify
  // a rectangular region using (xmin,xmax,ymin,ymax).
  vtkSetVector4Macro(Selection,int);
  vtkGetVectorMacro(Selection,int,4);

  // Description:
  // Set/Get the flag which enables inverse selection; i.e., invisible points
  // are selected.
  vtkSetMacro(SelectInvisible,int);
  vtkGetMacro(SelectInvisible,int);
  vtkBooleanMacro(SelectInvisible,int);

  // Description:
  // Set/Get a tolerance to use to determine whether a point is visible. A
  // tolerance is usually required because the conversion from world space
  // to display space during rendering introduces numerical round-off.
  vtkSetClampMacro(Tolerance,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance,double);

  // Description:
  // Requires the renderer to be set. Populates the composite perspective transform
  // and returns a pointer to the Z-buffer (that must be deleted) if getZbuff is set.
  float * Initialize(bool getZbuff);

  // Description:
  // Tests if a point x is being occluded or not against the Z-Buffer array passed in by
  // zPtr. Call Initialize before calling this method.
  bool IsPointOccluded(const double x[3], const float *zPtr);

  // Description:
  // Return MTime also considering the renderer.
  unsigned long GetMTime();

protected:
  vtkSelectVisiblePoints();
  ~vtkSelectVisiblePoints();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkRenderer *Renderer;
  vtkMatrix4x4 *CompositePerspectiveTransform;

  int SelectionWindow;
  int Selection[4];
  int InternalSelection[4];
  int SelectInvisible;
  double Tolerance;

private:
  vtkSelectVisiblePoints(const vtkSelectVisiblePoints&);  // Not implemented.
  void operator=(const vtkSelectVisiblePoints&);  // Not implemented.
};

#endif
