/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProbeFilter - sample data values at specified point locations
// .SECTION Description
// vtkProbeFilter is a filter that computes point attributes (e.g., scalars,
// vectors, etc.) at specified point positions. The filter has two inputs:
// the Input and Source. The Input geometric structure is passed through the
// filter. The point attributes are computed at the Input point positions
// by interpolating into the source data. For example, we can compute data
// values on a plane (plane specified as Input) from a volume (Source).

#ifndef __vtkProbeFilter_h
#define __vtkProbeFilter_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkIdTypeArray.h"

class VTK_GRAPHICS_EXPORT vtkProbeFilter : public vtkDataSetToDataSetFilter
{
public:
  static vtkProbeFilter *New();
  vtkTypeRevisionMacro(vtkProbeFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  void SetSource(vtkDataSet *source);
  vtkDataSet *GetSource();

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
  vtkProbeFilter();
  ~vtkProbeFilter();

  int SpatialMatch;

  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *output);

  vtkIdTypeArray *ValidPoints;
private:
  vtkProbeFilter(const vtkProbeFilter&);  // Not implemented.
  void operator=(const vtkProbeFilter&);  // Not implemented.
};

#endif
