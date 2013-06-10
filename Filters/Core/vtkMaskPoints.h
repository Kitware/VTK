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
// to begin masking from.
// It is possible to also generate different random selections
// (jittered strides, real random samples, and spatially stratified
// random samples) from the input data.
// The filter can also generate vertices (topological
// primitives) as well as points. This is useful because vertices are
// rendered while points are not.

#ifndef __vtkMaskPoints_h
#define __vtkMaskPoints_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkMaskPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkMaskPoints *New();
  vtkTypeMacro(vtkMaskPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on every nth point (strided sampling), ignored by random modes.
  vtkSetClampMacro(OnRatio,int,1,VTK_INT_MAX);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Limit the number of points that can be passed through (i.e.,
  // sets the output sample size).
  vtkSetClampMacro(MaximumNumberOfPoints,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(MaximumNumberOfPoints,vtkIdType);

  // Description:
  // Start sampling with this point. Ignored by certain random modes.
  vtkSetClampMacro(Offset,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(Offset,vtkIdType);

  // Description:
  // Special flag causes randomization of point selection.
  vtkSetMacro(RandomMode,int);
  vtkGetMacro(RandomMode,int);
  vtkBooleanMacro(RandomMode,int);

  // Description:
  // Special mode selector that switches between random mode types.
  // 0 - randomized strides: randomly strides through the data (default);
  //     fairly certain that this is not a statistically random sample
  //     because the output depends on the order of the input and
  //     the input points do not have an equal chance to appear in the output
  //     (plus Vitter's incremental random algorithms are more complex
  //      than this, while not a proof it is good indication this isn't
  //      a statistically random sample - the closest would be algorithm S)
  // 1 - random sample: create a statistically random sample using Vitter's
  //     incremental algorithm D without A described in Vitter
  //     "Faster Mthods for Random Sampling", Communications of the ACM
  //     Volume 27, Issue 7, 1984
  //     (OnRatio and Offset are ignored) O(sample size)
  // 2 - spatially stratified random sample: create a spatially
  //     stratified random sample using the first method described in
  //     Woodring et al. "In-situ Sampling of a Large-Scale Particle
  //     Simulation for Interactive Visualization and Analysis",
  //     Computer Graphics Forum, 2011 (EuroVis 2011).
  //     (OnRatio and Offset are ignored) O(N log N)
  vtkSetClampMacro(RandomModeType, int, 0, 2);
  vtkGetMacro(RandomModeType, int);

  // Description:
  // THIS ONLY WORKS WITH THE PARALLEL IMPLEMENTATION vtkPMaskPoints RUNNING
  // IN PARALLEL.
  // NOTHING WILL CHANGE IF THIS IS NOT THE PARALLEL vtkPMaskPoints.
  // Determines whether maximum number of points is taken per processor
  // (default) or if the maximum number of points is proportionally
  // taken across processors (i.e., number of points per
  // processor = points on a processor * maximum number of points /
  // total points across all processors).  In the first case,
  // the total number of points = maximum number of points *
  // number of processors.  In the second case, the total number of
  // points = maximum number of points.
  vtkSetMacro(ProportionalMaximumNumberOfPoints, int);
  vtkGetMacro(ProportionalMaximumNumberOfPoints, int);
  vtkBooleanMacro(ProportionalMaximumNumberOfPoints, int);

  // Description:
  // Generate output polydata vertices as well as points. A useful
  // convenience method because vertices are drawn (they are topology) while
  // points are not (they are geometry). By default this method is off.
  vtkSetMacro(GenerateVertices,int);
  vtkGetMacro(GenerateVertices,int);
  vtkBooleanMacro(GenerateVertices,int);

  // Description:
  // When vertex generation is enabled, by default vertices are produced
  // as multi-vertex cells (more than one per cell), if you wish to have
  // a single vertex per cell, enable this flag.
  vtkSetMacro(SingleVertexPerCell,int);
  vtkGetMacro(SingleVertexPerCell,int);
  vtkBooleanMacro(SingleVertexPerCell,int);

protected:
  vtkMaskPoints();
  ~vtkMaskPoints() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  int OnRatio;     // every OnRatio point is on; all others are off.
  vtkIdType Offset;      // offset (or starting point id)
  int RandomMode;  // turn on/off randomization
  vtkIdType MaximumNumberOfPoints;
  int GenerateVertices; //generate polydata verts
  int SingleVertexPerCell;
  int RandomModeType; // choose the random sampling mode
  int ProportionalMaximumNumberOfPoints;

  virtual void InternalScatter(unsigned long*, unsigned long *, int, int) {};
  virtual void InternalGather(unsigned long*, unsigned long*, int, int) {};
  virtual int InternalGetNumberOfProcesses() { return 1; };
  virtual int InternalGetLocalProcessId() { return 0; };
  virtual void InternalBarrier() {};
  unsigned long GetLocalSampleSize(vtkIdType, int);

private:
  vtkMaskPoints(const vtkMaskPoints&);  // Not implemented.
  void operator=(const vtkMaskPoints&);  // Not implemented.
};

#endif
