/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelCoordinatesActor.h
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORSS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkParallelCoordinatesActor - create parallel coordinate display from input field
// .SECTION Description
// vtkParallelCoordinatesActor generates a parallel coordinates plot from an
// input field (i.e., vtkDataObject). Parallel coordinates represent
// N-dimensional data by using a set of N parallel axes (not orthogonal like
// the usual x-y-z Cartesian axes). Each N-dimensional point is plotted as a
// polyline, were each of the N components of the point lie on one of the
// N axes, and the components are connected by straight lines.
//
// To use this class, you must specify an input data object. You'll probably
// also want to specify the position of the plot be setting the Position and
// Position2 instance variables, which define a rectangle in which the plot
// lies. Another important parameter is the IndependentVariables ivar, which
// tells the instance how to interpret the field data (independent variables
// as the rows or columns of the field). There are also many other instance
// variables that control the look of the plot includes its title, font 
// attributes, number of ticks on the axes, etc.

// .SECTION Caveats
// Field data is not necessarily "rectangular" in shape. In these cases, some
// of the data may not be plotted.
//
// The early implementation lacks many features that could be added in the future.
// This includes the ability to "brush" data (choose regions along an axis and
// highlight any points/lines passing through the region); efficiency is really
// bad; more control over the properties of the plot (separate properties for
// each axes,title,etc.; and using the labels found in the field to label each
// of the axes.

#ifndef __vtkParallelCoordinatesActor_h
#define __vtkParallelCoordinatesActor_h

#include "vtkAxisActor2D.h"
#include "vtkDataObject.h"

#define VTK_IV_COLUMN 0
#define VTK_IV_ROW    1

class VTK_RENDERING_EXPORT vtkParallelCoordinatesActor : public vtkActor2D
{
public:
  vtkTypeMacro(vtkParallelCoordinatesActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with autorange computation; bold, italic, and shadows
  // on; arial font family; the number of labels set to 5 for the x and y
  // axes; a label format of "%-#6.3g"; and x coordinates computed from point
  // ids.
  static vtkParallelCoordinatesActor *New();

  // Description:
  // Specify whether to use the rows or columns as independent variables.
  // If columns, then each row represents a separate point. If rows, then 
  // each column represents a separate point.
  vtkSetClampMacro(IndependentVariables,int,VTK_IV_COLUMN, VTK_IV_ROW);
  vtkGetMacro(IndependentVariables,int);
  void SetIndependentVariablesToColumns()
    {this->SetIndependentVariables(VTK_IV_COLUMN);};
  void SetIndependentVariablesToRows()
    {this->SetIndependentVariables(VTK_IV_ROW);};

  // Description:
  // Set/Get the title of the parallel coordinates plot.
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);

  // Description:
  // Set/Get the number of annotation labels to show along each axis.
  // This values is a suggestion: the number of labels may vary depending
  // on the particulars of the data.
  vtkSetClampMacro(NumberOfLabels, int, 0, 50);
  vtkGetMacro(NumberOfLabels, int);
  
  // Description:
  // Enable/Disable bolding annotation text.
  vtkSetMacro(Bold, int);
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/Disable italicizing annotation text.
  vtkSetMacro(Italic, int);
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/Disable creating shadows on the annotation text. Shadows make 
  // the text easier to read.
  vtkSetMacro(Shadow, int);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);

  // Description:
  // Set/Get the font family for the annotation text. Three font types 
  // are available: Arial (VTK_ARIAL), Courier (VTK_COURIER), and 
  // Times (VTK_TIMES).
  vtkSetMacro(FontFamily, int);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {this->SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {this->SetFontFamily(VTK_TIMES);};

  // Description:
  // Set/Get the format with which to print the labels on the axes.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Draw the parallel coordinates plot.
  int RenderOpaqueGeometry(vtkViewport*);
  int RenderOverlay(vtkViewport*);
  int RenderTranslucentGeometry(vtkViewport *) {return 0;}

  // Description:
  // Set the input to the parallel coordinates actor.
  vtkSetObjectMacro(Input,vtkDataObject);

  // Description:
  // Remove a dataset from the list of data to append.
  vtkGetObjectMacro(Input,vtkDataObject);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkParallelCoordinatesActor();
  ~vtkParallelCoordinatesActor();

private:
  vtkDataObject *Input; //list of data sets to plot

  int IndependentVariables; //use column or row
  vtkIdType N; //the number of independent variables
  vtkAxisActor2D **Axes; //an array of axes
  float *Mins; //minimum data value along this row/column
  float *Maxs; //maximum data value along this row/column
  int   *Xs; //axes x-values (in viewport coordinates)
  int   YMin; //axes y-min-value (in viewport coordinates)
  int   YMax; //axes y-max-value (in viewport coordinates)

  char *Title;
  vtkTextMapper *TitleMapper;
  vtkActor2D    *TitleActor;

  vtkPolyData         *PlotData; //the lines drawn within the axes
  vtkPolyDataMapper2D *PlotMapper;
  vtkActor2D          *PlotActor;
  
  // font characteristics
  int   NumberOfLabels; //along each axis
  int	Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;
  char  *LabelFormat;

  vtkTimeStamp  BuildTime;

  void Initialize();
  int PlaceAxes(vtkViewport *viewport, int *size);
private:
  vtkParallelCoordinatesActor(const vtkParallelCoordinatesActor&);  // Not implemented.
  void operator=(const vtkParallelCoordinatesActor&);  // Not implemented.
};


#endif

