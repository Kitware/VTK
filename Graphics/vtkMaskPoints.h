/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMaskPoints - selectively filter points
// .SECTION Description
// vtkMaskPoints is a filter that passes through points and point attributes 
// from input dataset. (Other geometry is not passed through.) It is 
// possible to mask every nth point, and to specify an initial offset
// to begin masking from. A special random mode feature enables random 
// selection of points. The filter can also generate vertices (topological
// primitives) as well as points. This is useful because vertices are
// rendered while points are not.

#ifndef __vtkMaskPoints_h
#define __vtkMaskPoints_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkMaskPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkMaskPoints *New();
  vtkTypeMacro(vtkMaskPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on every nth point.
  vtkSetClampMacro(OnRatio,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Limit the number of points that can be passed through.
  vtkSetClampMacro(MaximumNumberOfPoints,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(MaximumNumberOfPoints,vtkIdType);

  // Description:
  // Start with this point.
  vtkSetClampMacro(Offset,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(Offset,vtkIdType);

  // Description:
  // Special flag causes randomization of point selection. If this mode is on,
  // statistically every nth point (i.e., OnRatio) will be displayed.
  vtkSetMacro(RandomMode,int);
  vtkGetMacro(RandomMode,int);
  vtkBooleanMacro(RandomMode,int);

  // Description:
  // Generate output polydata vertices as well as points. A useful
  // convenience method because vertices are drawn (they are topology) while
  // points are not (they are geometry). By default this method is off.
  vtkSetMacro(GenerateVertices,int);
  vtkGetMacro(GenerateVertices,int);
  vtkBooleanMacro(GenerateVertices,int);

  // Description:
  // When vertex generation is enabled, by default vertices are produced
  // as multi-vertex cells (more than one per cell), if you wish to have a single
  // vertex per cell, enable this flag.
  vtkSetMacro(SingleVertexPerCell,int);
  vtkGetMacro(SingleVertexPerCell,int);
  vtkBooleanMacro(SingleVertexPerCell,int);

protected:
  vtkMaskPoints();
  ~vtkMaskPoints() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int OnRatio;     // every OnRatio point is on; all others are off.
  vtkIdType Offset;      // offset (or starting point id)
  int RandomMode;  // turn on/off randomization
  vtkIdType MaximumNumberOfPoints;
  int GenerateVertices; //generate polydata verts
  int SingleVertexPerCell;
private:
  vtkMaskPoints(const vtkMaskPoints&);  // Not implemented.
  void operator=(const vtkMaskPoints&);  // Not implemented.
};

#endif
