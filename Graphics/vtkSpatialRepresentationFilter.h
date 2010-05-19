/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#define VTK_MAX_SPATIAL_REP_LEVEL 24

class vtkLocator;
class vtkDataSet;

class VTK_GRAPHICS_EXPORT vtkSpatialRepresentationFilter : public vtkPolyDataSource
{
public:
  static vtkSpatialRepresentationFilter *New();
  vtkTypeMacro(vtkSpatialRepresentationFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the locator that will be used to generate the representation.
  virtual void SetSpatialRepresentation(vtkLocator*);
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

  void Execute();
  void GenerateOutput();

  int Level;
  int TerminalNodesRequested;

  vtkLocator *SpatialRepresentation;

  virtual void ReportReferences(vtkGarbageCollector*);
  virtual int FillInputPortInformation(int, vtkInformation*);
private:
  vtkSpatialRepresentationFilter(const vtkSpatialRepresentationFilter&);  // Not implemented.
  void operator=(const vtkSpatialRepresentationFilter&);  // Not implemented.
};

#endif


