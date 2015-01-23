/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeClustering2DLayoutStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkAttributeClustering2DLayoutStrategy - a simple fast 2D graph layout
//
// .SECTION Description
// This class is a density grid based force directed layout strategy.
// Also please note that 'fast' is relative to quite slow. :)
// The layout running time is O(V+E) with an extremely high constant.
// .SECTION Thanks
// Thanks to Godzilla for not eating my computer so that this class
// could be written.

#ifndef vtkAttributeClustering2DLayoutStrategy_h
#define vtkAttributeClustering2DLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

class vtkFastSplatter;
class vtkImageData;
class vtkIntArray;
class vtkFloatArray;

class VTKINFOVISLAYOUT_EXPORT vtkAttributeClustering2DLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkAttributeClustering2DLayoutStrategy *New();

  vtkTypeMacro(vtkAttributeClustering2DLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The name of the array on the vertices, whose values will be used for
  // determining clusters.
  vtkGetStringMacro(VertexAttribute);
  void SetVertexAttribute(const char*);

  // Description:
  // Seed the random number generator used to jitter point positions.
  // This has a significant effect on their final positions when
  // the layout is complete.
  vtkSetClampMacro(RandomSeed, int, 0, VTK_INT_MAX);
  vtkGetMacro(RandomSeed, int);

  // Description:
  // Set/Get the maximum number of iterations to be used.
  // The higher this number, the more iterations through the algorithm
  // is possible, and thus, the more the graph gets modified.
  // The default is '100' for no particular reason
  // Note: The strong recommendation is that you do not change
  // this parameter. :)
  vtkSetClampMacro(MaxNumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(MaxNumberOfIterations, int);

  // Description:
  // Set/Get the number of iterations per layout.
  // The only use for this ivar is for the application
  // to do visualizations of the layout before it's complete.
  // The default is '100' to match the default 'MaxNumberOfIterations'
  // Note: Changing this parameter is just fine :)
  vtkSetClampMacro(IterationsPerLayout, int, 0, VTK_INT_MAX);
  vtkGetMacro(IterationsPerLayout, int);

  // Description:
  // Set the initial temperature.  The temperature default is '5'
  // for no particular reason
  // Note: The strong recommendation is that you do not change
  // this parameter. :)
  vtkSetClampMacro(InitialTemperature, float, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(InitialTemperature, float);

  // Description:
  // Set/Get the Cool-down rate.
  // The higher this number is, the longer it will take to "cool-down",
  // and thus, the more the graph will be modified. The default is '10'
  // for no particular reason.
  // Note: The strong recommendation is that you do not change
  // this parameter. :)
  vtkSetClampMacro(CoolDownRate, double, 0.01, VTK_DOUBLE_MAX);
  vtkGetMacro(CoolDownRate, double);

  // Description:
  // Manually set the resting distance. Otherwise the
  // distance is computed automatically.
  vtkSetMacro(RestDistance, float);
  vtkGetMacro(RestDistance, float);

  // Description:
  // This strategy sets up some data structures
  // for faster processing of each Layout() call
  virtual void Initialize();

  // Description:
  // This is the layout method where the graph that was
  // set in SetGraph() is laid out. The method can either
  // entirely layout the graph or iteratively lay out the
  // graph. If you have an iterative layout please implement
  // the IsLayoutComplete() method.
  virtual void Layout();

  // Description:
  // I'm an iterative layout so this method lets the caller
  // know if I'm done laying out the graph
  virtual int IsLayoutComplete() {return this->LayoutComplete;}

protected:
  vtkAttributeClustering2DLayoutStrategy();
  ~vtkAttributeClustering2DLayoutStrategy();

  int    MaxNumberOfIterations;  //Maximum number of iterations.
  float  InitialTemperature;
  float  CoolDownRate;  //Cool-down rate.  Note:  Higher # = Slower rate.

private:

  //BTX
  // This class 'has a' vtkFastSplatter for the density grid
  vtkSmartPointer<vtkFastSplatter>        DensityGrid;
  vtkSmartPointer<vtkImageData>           SplatImage;
  vtkSmartPointer<vtkFloatArray>          RepulsionArray;
  vtkSmartPointer<vtkFloatArray>          AttractionArray;
  vtkSmartPointer<vtkIntArray>            EdgeCountArray;
  //ETX

  int RandomSeed;
  int IterationsPerLayout;
  int TotalIterations;
  int LayoutComplete;
  float Temp;
  float RestDistance;
  float CuttingThreshold;
  char* VertexAttribute;

  // Private helper methods
  void GenerateCircularSplat(vtkImageData *splat, int x, int y);
  void GenerateGaussianSplat(vtkImageData *splat, int x, int y);
  void ResolveCoincidentVertices();

//BTX
  class Internals;
  Internals* Implementation;
//ETX

  vtkAttributeClustering2DLayoutStrategy(const vtkAttributeClustering2DLayoutStrategy&);  // Not implemented.
  void operator=(const vtkAttributeClustering2DLayoutStrategy&);  // Not implemented.
};

#endif

