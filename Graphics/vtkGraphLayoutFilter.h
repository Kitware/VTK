/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Scott Hill of RPI for developing this class
             Mark Lacy for Procter & Gamble for support

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
// .NAME vtkGraphLayoutFilter - nice layout of undirected graphs in 3D
// .SECTION Description
// vtkGraphLayoutFilter will reposition a network of nodes, connected by
// lines, into a more pleasing arrangement. The class implements a simple
// force-directed placement algorithm (Fruchterman & Reingold "Graph Drawing 
// by Force-directed Placement" Software-Practice and Experience 21(11) 1991).
//
// The input to the filter is a vtkPolyData representing the undirected 
// graphs. A graph is represented by a set of polylines. The output is
// also a vtkPolyData, where the point positions have been modified.
// To use the filter, specify whether you wish the layout to occur in
// 2D or 3D; the bounds in which the graph should lie (note that you
// can just use automatic bounds computation); and modify the cool down
// rate (controls the final process of simulated annealing).

#ifndef __vtkGraphLayoutFilter_h
#define __vtkGraphLayoutFilter_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkGraphLayoutFilter : public vtkPolyDataToPolyDataFilter 
{
public:
  static vtkGraphLayoutFilter *New();

  vtkTypeMacro(vtkGraphLayoutFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off automatic length/width calculation. If this
  // boolean is off, then the manually specified Length, Width,
  // and Height is used.
  vtkSetMacro(AutomaticBoundsComputation, int);
  vtkGetMacro(AutomaticBoundsComputation, int);
  vtkBooleanMacro(AutomaticBoundsComputation, int);

  // Description:
  // Set/Get the maximum number of iterations to be used.
  // The higher this number, the more iterations through the algorithm
  // is possible, and thus, the more the graph gets modified.
  vtkSetClampMacro(MaxNumberOfIterations, int, 0, VTK_LARGE_INTEGER);
  vtkGetMacro(MaxNumberOfIterations, int);

  // Description:
  // Set/Get the Cool-down rate.
  // The higher this number is, the longer it will take to "cool-down",
  // and thus, the more the graph will be modified.
  vtkSetClampMacro(CoolDownRate, float, 0.01, VTK_LARGE_FLOAT);
  vtkGetMacro(CoolDownRate, float);

  // Turn on/off layout of graph in three dimensions. If off, graph
  // layout occurs in two dimensions.
  vtkSetMacro(ThreeDimensionalLayout, int);
  vtkGetMacro(ThreeDimensionalLayout, int);
  vtkBooleanMacro(ThreeDimensionalLayout, int);

  // Description:
  // The bounds factor is used when the ivar AutomaticBoundsComputation is
  // set to "on".  When it is, this factor determines how much larger the
  // dimensions of the graph are in relation to how big the graph is itself.
  vtkSetClampMacro(BoundsFactor, float, 0, VTK_LARGE_FLOAT);
  vtkGetMacro(BoundsFactor, float);

protected:
  vtkGraphLayoutFilter();
  ~vtkGraphLayoutFilter() {}
  vtkGraphLayoutFilter(const vtkGraphLayoutFilter&) {}
  void operator=(const vtkGraphLayoutFilter&) {}

  void Execute();

  float GraphBounds[6];
  int MaxNumberOfIterations;  //Maximum number of iterations.
  float CoolDownRate;  //Cool-down rate.  Note:  Higher # = Slower rate.
  int AutomaticBoundsComputation;  //Boolean to calculate bounds automatically.
  int ThreeDimensionalLayout;  //Boolean for a third dimension.
  float BoundsFactor;  //Factor to extent the bounds.  0.1 = 10%, etc.
};

#endif
