/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArcPlotter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkArcPlotter - plot data along an arbitrary polyline
// .SECTION Description
// vtkArcPlotter performs plotting of attribute data along polylines defined
// with an input vtkPolyData data object. Any type of attribute data can be
// plotted including scalars, vectors, tensors, normals, texture coordinates,
// and field data. Either one or multiple data components can be plotted.
// 
// To use this class you must specify an input data set that contains one or
// more polylines, and some attribute data including which component of the
// attribute data. (By default, this class processes the first component of
// scalar data.) You will also need to set an offset radius (the distance
// of the polyline to the median line of the plot), a width for the plot
// (the distance that the minimum and maximum plot values are mapped into),
// an possibly an offset (used to offset attribute data with multiple
// components).
// 
// Normally the filter automatically computes normals for generating the
// offset arc plot. However, you can specify a default normal and use that
// instead.

// .SECTION See Also
// vtkXYPlotActor

#ifndef __vtkArcPlotter_h
#define __vtkArcPlotter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkCamera.h"

#define VTK_PLOT_SCALARS    1
#define VTK_PLOT_VECTORS    2
#define VTK_PLOT_NORMALS    3
#define VTK_PLOT_TCOORDS    4
#define VTK_PLOT_TENSORS    5
#define VTK_PLOT_FIELD_DATA 6

class VTK_HYBRID_EXPORT vtkArcPlotter : public vtkPolyDataToPolyDataFilter 
{
public:
  // Description:
  // Instantiate with no default camera and plot mode set to
  // VTK_SCALARS.
  static vtkArcPlotter *New() {return new vtkArcPlotter;};

  vtkTypeMacro(vtkArcPlotter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify a camera used to orient the plot along the arc. If no camera
  // is specified, then the orientation of the plot is arbitrary.
  vtkSetObjectMacro(Camera,vtkCamera);
  vtkGetObjectMacro(Camera,vtkCamera);

  // Description:
  // Specify which data to plot: scalars, vectors, normals, texture coords,
  // tensors, or field data. If the data has more than one component, use
  // the method SetPlotComponent to control which component to plot.
  vtkSetMacro(PlotMode, int);
  vtkGetMacro(PlotMode, int);
  void SetPlotModeToPlotScalars() {this->SetPlotMode(VTK_PLOT_SCALARS);};
  void SetPlotModeToPlotVectors() {this->SetPlotMode(VTK_PLOT_VECTORS);};
  void SetPlotModeToPlotNormals() {this->SetPlotMode(VTK_PLOT_NORMALS);};
  void SetPlotModeToPlotTCoords() {this->SetPlotMode(VTK_PLOT_TCOORDS);};
  void SetPlotModeToPlotTensors() {this->SetPlotMode(VTK_PLOT_TENSORS);};
  void SetPlotModeToPlotFieldData()
            {this->SetPlotMode(VTK_PLOT_FIELD_DATA);};

  // Description:
  // Set/Get the component number to plot if the data has more than one 
  // component. If the value of the plot component is == (-1), then all
  // the components will be plotted.
  vtkSetMacro(PlotComponent,int);
  vtkGetMacro(PlotComponent,int);

  // Description:
  // Set the radius of the "median" value of the first plotted component.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Set the height of the plot. (The radius combined with the height
  // define the location of the plot relative to the generating polyline.)
  vtkSetClampMacro(Height,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Height,float);

  // Description:
  // Specify an offset that translates each subsequent plot (if there is
  // more than one component plotted) from the defining arc (i.e., polyline).
  vtkSetClampMacro(Offset, float, 0.0, VTK_LARGE_FLOAT);
  vtkGetMacro(Offset, float);

  // Description:
  // Set a boolean to control whether to use default normals.
  // By default, normals are automatically computed from the generating
  // polyline and camera.
  vtkSetMacro(UseDefaultNormal,int);
  vtkGetMacro(UseDefaultNormal,int);
  vtkBooleanMacro(UseDefaultNormal,int);

  // Description:
  // Set the default normal to use if you do not wish automatic normal
  // calculation. The arc plot will be generated using this normal.
  vtkSetVector3Macro(DefaultNormal,float);
  vtkGetVectorMacro(DefaultNormal,float,3);

  // Description:
  // Set/Get the field data array to plot. This instance variable is
  // only applicable if field data is plotted.
  vtkSetClampMacro(FieldDataArray,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(FieldDataArray,int);

  // Description:
  // New GetMTime because of camera dependency.
  unsigned long GetMTime();

protected:
  vtkArcPlotter();
  ~vtkArcPlotter();
  vtkArcPlotter(const vtkArcPlotter&);
  void operator=(const vtkArcPlotter&);

  void Execute();
  int  OffsetPoint(vtkIdType ptId, vtkPoints *inPts, float n[3],
                   vtkPoints *newPts, float offset, float *range, float val);
  int  ProcessComponents(vtkIdType numPts, vtkPointData *pd);

  vtkCamera *Camera;
  int       PlotMode;
  int       PlotComponent;
  float     Radius;
  float     Height;
  float     Offset;
  float     DefaultNormal[3];
  int       UseDefaultNormal;
  int       FieldDataArray;
  
private:
  vtkDataArray *Data;
  float    *DataRange;
  float    *Tuple;
  int       NumberOfComponents;
  int       ActiveComponent;
  int       StartComp;
  int       EndComp;
  
};

#endif
