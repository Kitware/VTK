/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericProbeFilter - sample data values at specified point locations
// .SECTION Description
// vtkGenericProbeFilter is a filter that computes point attributes (e.g., scalars,
// vectors, etc.) at specified point positions. The filter has two inputs:
// the Input and Source. The Input geometric structure is passed through the
// filter. The point attributes are computed at the Input point positions
// by interpolating into the source data. For example, we can compute data
// values on a plane (plane specified as Input) from a volume (Source).

#ifndef __vtkGenericProbeFilter_h
#define __vtkGenericProbeFilter_h

#include "vtkDataSetToDataSetFilter.h"

class vtkIdTypeArray;
class vtkGenericDataSet;

class VTK_GENERIC_FILTERING_EXPORT vtkGenericProbeFilter : public vtkDataSetToDataSetFilter
{
public:
  static vtkGenericProbeFilter *New();
  vtkTypeRevisionMacro(vtkGenericProbeFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  void SetSource(vtkGenericDataSet *source);
  vtkGenericDataSet *GetSource();

  // Description:
  // This flag is used only when a piece is requested to update.  By default
  // the flag is off.  Because no spatial correspondence between input pieces
  // and source pieces is known, all of the source has to be requested no
  // matter what piece of the output is requested.  When there is a spatial 
  // correspondence, the user/application can set this flag.  This hint allows
  // the breakup of the probe operation to be much more efficient.  When piece
  // m of n is requested for update by the user, then only n of m needs to
  // be requested of the source. 
  vtkSetMacro(SpatialMatch, int);
  vtkGetMacro(SpatialMatch, int);
  vtkBooleanMacro(SpatialMatch, int);

  // Description:
  // Get the list of point ids in the output that contain attribute data
  // interpolated from the source.
  vtkGetObjectMacro(ValidPoints, vtkIdTypeArray);
  
protected:
  vtkGenericProbeFilter();
  ~vtkGenericProbeFilter();

  int SpatialMatch;

  virtual void Execute();
  //void ExecuteInformation();
  //virtual void ComputeInputUpdateExtents(vtkDataObject *output);

  vtkIdTypeArray *ValidPoints;
private:
  vtkGenericProbeFilter(const vtkGenericProbeFilter&);  // Not implemented.
  void operator=(const vtkGenericProbeFilter&);  // Not implemented.
};

#endif
