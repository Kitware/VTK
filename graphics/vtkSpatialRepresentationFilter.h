/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
// Generally spatial search objects are used for collision detection and other 
// geometric operations, but in this filter one or more levels of spatial 
// searchers can be generated to form a geometric approximation to the 
// input data. This is a form of data simplification, generally used to 
// accelerate the rendering process. Or, this filter can be used as a debugging/
// visualization aid for spatial search objects.
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

#include "vtkDataSetFilter.h"
#include "vtkPolyData.h"
#include "vtkLocator.h"

#define VTK_MAX_SPATIAL_REP_LEVEL 24

class VTK_EXPORT vtkSpatialRepresentationFilter : public vtkDataSetFilter
{
public:
  vtkSpatialRepresentationFilter();
  ~vtkSpatialRepresentationFilter();
  static vtkSpatialRepresentationFilter *New() {return new vtkSpatialRepresentationFilter;};
  const char *GetClassName() {return "vtkSpatialRepresentationFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the locator that will be used to generate the representation.
  vtkSetObjectMacro(SpatialRepresentation,vtkLocator);
  vtkGetObjectMacro(SpatialRepresentation,vtkLocator);

  // Description:
  // Get the maximum number of outputs actually available.
  vtkGetMacro(Level,int);

  // returns leaf nodes of the spatial representation.
  vtkPolyData *GetOutput();

  // special form of GetOutput() method returns multiple outputs
  vtkPolyData *GetOutput(int level);

  // reset requested output levels
  void ResetOutput();

  void Update();

protected:
  void Execute();
  void GenerateOutput();

  int Level;
  int TerminalNodesRequested;

  vtkLocator *SpatialRepresentation;
  vtkPolyData *OutputList[VTK_MAX_SPATIAL_REP_LEVEL+1];
};

#endif


