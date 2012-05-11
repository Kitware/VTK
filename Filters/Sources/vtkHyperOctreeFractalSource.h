/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeFractalSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeFractalSource - Create an octree from a fractal.
// hyperoctree
// .SECTION Description
//
// .SECTION See Also
// vtkHyperOctreeSampleFunction

#ifndef __vtkHyperOctreeFractalSource_h
#define __vtkHyperOctreeFractalSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperOctreeAlgorithm.h"

class vtkImplicitFunction;

class VTKFILTERSSOURCES_EXPORT vtkHyperOctreeFractalSource : public vtkHyperOctreeAlgorithm
{
public:
  vtkTypeMacro(vtkHyperOctreeFractalSource,vtkHyperOctreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHyperOctreeFractalSource *New();

  // Description:
  // Return the maximum number of levels of the hyperoctree.
  // \post positive_result: result>=1
  int GetMaximumLevel();

  // Description:
  // Set the maximum number of levels of the hyperoctree. If
  // GetMinLevels()>=levels, GetMinLevels() is changed to levels-1.
  // \pre positive_levels: levels>=1
  // \post is_set: this->GetLevels()==levels
  // \post min_is_valid: this->GetMinLevels()<this->GetLevels()
  void SetMaximumLevel(int levels);

  // Description:
  // Return the minimal number of levels of systematic subdivision.
  // \post positive_result: result>=0
  void SetMinimumLevel(int level);
  int GetMinimumLevel();


  //========== Mandelbrot parameters ==========

  // Description:
  // Set the projection from  the 4D space (4 parameters / 2 imaginary numbers)
  // to the axes of the 3D Volume.
  // 0=C_Real, 1=C_Imaginary, 2=X_Real, 4=X_Imaginary
  void SetProjectionAxes(int x, int y, int z);
  void SetProjectionAxes(int a[3]) {this->SetProjectionAxes(a[0],a[1],a[2]);}
  vtkGetVector3Macro(ProjectionAxes, int);

  // Description:
  // Imaginary and real value for C (constant in equation)
  // and X (initial value).
  vtkSetVector4Macro(OriginCX, double);
  vtkGetVector4Macro(OriginCX, double);

  // Description:
  // Just a different way of setting the sample.
  // This sets the size of the 4D volume.
  // SampleCX is computed from size and extent.
  // Size is ignored when a dimension i 0 (collapsed).
  vtkSetVector4Macro(SizeCX, double);
  vtkGetVector4Macro(SizeCX, double);

  // Description:
  // The maximum number of cycles run to see if the value goes over 2
  vtkSetClampMacro(MaximumNumberOfIterations, unsigned short, 1, 255);
  vtkGetMacro(MaximumNumberOfIterations, unsigned char);

  // Description:
  // Create a 2D or 3D fractal.
  vtkSetClampMacro(Dimension, int, 2, 3);
  vtkGetMacro(Dimension, int);

  // Description:
  // Controls when a leaf gets subdivided.  If the corner values span
  // a larger range than this value, the leaf is subdivided.  This
  // defaults to 2.
  vtkSetMacro(SpanThreshold, double);
  vtkGetMacro(SpanThreshold, double);

protected:
  vtkHyperOctreeFractalSource();
  ~vtkHyperOctreeFractalSource();

  int RequestInformation (vtkInformation * vtkNotUsed(request),
                          vtkInformationVector ** vtkNotUsed( inputVector ),
                          vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  void Subdivide(vtkHyperOctreeCursor *cursor,
                 int level, vtkHyperOctree *output,
                 double* origin, double* size,
                 float* cornerVals);

  int MaximumLevel;
  int MinimumLevel;
  int Dimension;

  int ProjectionAxes[3];

  unsigned char MaximumNumberOfIterations;

  // Complex constant/initial-value at origin.
  double OriginCX[4];

  // A temporary vector that is computed as needed.
  // It is used to return a vector.
  double SizeCX[4];

  float EvaluateWorldPoint(double p[3]);
  float EvaluateSet(double p[4]);

  double Origin[3];
  double Size[3];

  double SpanThreshold;

private:
  vtkHyperOctreeFractalSource(const vtkHyperOctreeFractalSource&);  // Not implemented.
  void operator=(const vtkHyperOctreeFractalSource&);  // Not implemented.
};

#endif
