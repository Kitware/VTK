/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.h
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
// .NAME vtkSpatialRepresentationFilter - generate polygonal model of spatial search object (i.e., a vtkLocator)
// .SECTION Description
// vtkSpatialRepresentationFilter generates an polygonal representation of a
// spatial search (vtkLocator) object. The representation varies depending
// upon the nature of the spatial search object. For example, the
// representation for vtkOBBTree is a collection of oriented bounding
// boxes. Ths input to this filter is a dataset of any type, and the output
// is polygonal data. You must also specify the spatial search object to
// use.
//
// Generally spatial search objects are used for collision detection and
// other geometric operations, but in this filter one or more levels of
// spatial searchers can be generated to form a geometric approximation to
// the input data. This is a form of data simplification, generally used to
// accelerate the rendering process. Or, this filter can be used as a
// debugging/ visualization aid for spatial search objects.
// 
// This filter can generate one or more output vtkPolyData corresponding to
// different levels in the spatial search tree. The output data is retrieved 
// using the GetOutput(id) method, where id ranges from 0 (root level) 
// to Level. Note that the output for level "id" is not computed unless a 
// GetOutput(id) method is issued. Thus, if you desire three levels of output 
// (say 2,4,7), you would have to invoke GetOutput(2), GetOutput(4), and 
// GetOutput(7). (Also note that the Level ivar is computed automatically 
// depending on the size and nature of the input data.) There is also 
// another GetOutput() method that takes no parameters. This method returns 
// the leafs of the spatial search tree, which may be at different levels.

// .SECTION Caveats
// You can specify the number of levels of to generate with the MaxLevels
// ivar. However, when the spatial search tree is built, this number of levels 
// may not actually be generated. The actual number available can be found in 
// the Levels ivar. Note that the value of Levels may change after filter
// execution.

// .SECTION See Also
// vtkLocator vtkPointLocator vtkCellLocator vtkOBBTree 

#ifndef __vtkSpatialRepresentationFilter_h
#define __vtkSpatialRepresentationFilter_h

#include "vtkPolyDataSource.h"
#include "vtkLocator.h"

#define VTK_MAX_SPATIAL_REP_LEVEL 24

class VTK_EXPORT vtkSpatialRepresentationFilter : public vtkPolyDataSource
{
public:
  static vtkSpatialRepresentationFilter *New();
  vtkTypeMacro(vtkSpatialRepresentationFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the locator that will be used to generate the representation.
  vtkSetObjectMacro(SpatialRepresentation,vtkLocator);
  vtkGetObjectMacro(SpatialRepresentation,vtkLocator);

  // Description:
  // Get the maximum number of outputs actually available.
  vtkGetMacro(Level,int);
  
  // Description:
  // A special form of the GetOutput() method that returns multiple outputs.
  vtkPolyData *GetOutput(int level);

  // Description:
  // Output of terminal nodes/leaves.
  vtkPolyData *GetOutput();  
  
  // Description:
  // Reset requested output levels
  void ResetOutput();
  
  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();

protected:
  vtkSpatialRepresentationFilter();
  ~vtkSpatialRepresentationFilter();
  vtkSpatialRepresentationFilter(const vtkSpatialRepresentationFilter&);
  void operator=(const vtkSpatialRepresentationFilter&);

  void Execute();
  void GenerateOutput();

  int Level;
  int TerminalNodesRequested;

  vtkLocator *SpatialRepresentation;
};

#endif


