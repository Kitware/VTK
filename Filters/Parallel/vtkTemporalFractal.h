/*=========================================================================

  Program:   ParaView
  Module:    vtkTemporalFractal.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalFractal - A source to test AMR data object.
// .SECTION Description
// vtkTemporalFractal is a collection of uniform grids.  All have the same
// dimensions. Each block has a different origin and spacing.  It uses
// mandelbrot to create cell data. I scale the fractal array to look like a
// volme fraction.
// I may also add block id and level as extra cell arrays.
// This source produces a vtkHierarchicalBoxDataSet when 
// GenerateRectilinearGrids is off, otherwise produces a vtkMultiBlockDataSet.

#ifndef __vtkTemporalFractal_h
#define __vtkTemporalFractal_h

#include "vtkTemporalDataSetAlgorithm.h"

class vtkCompositeDataSet;
class vtkDataSet;
class vtkHierarchicalBoxDataSet;
class vtkIntArray;
class vtkRectilinearGrid;
class vtkUniformGrid;

class VTK_PARALLEL_EXPORT vtkTemporalFractal : public vtkTemporalDataSetAlgorithm
{
public:
  static vtkTemporalFractal *New();
  vtkTypeMacro(vtkTemporalFractal,vtkTemporalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Essentially the iso surface value.
  // The fractal array is scaled to map this value to 0.5 for use as a volume
  // fraction.
  vtkSetMacro(FractalValue, float);
  vtkGetMacro(FractalValue, float);  

  // Description:
  // Any blocks touching a predefined line will be subdivided to this level.
  // Other blocks are subdivided so that neighboring blocks only differ
  // by one level.
  vtkSetMacro(MaximumLevel, int);
  vtkGetMacro(MaximumLevel, int);

  // Description:
  // XYZ dimensions of cells.
  vtkSetMacro(Dimensions, int);
  vtkGetMacro(Dimensions, int);

  // Description:
  // For testing ghost levels.
  vtkSetMacro(GhostLevels, int);
  vtkGetMacro(GhostLevels, int);
  vtkBooleanMacro(GhostLevels, int);
  
  // Description:
  // Generate either rectilinear grids either uniform grids.
  // Default is false.
  vtkSetMacro(GenerateRectilinearGrids, int);
  vtkGetMacro(GenerateRectilinearGrids, int);
  vtkBooleanMacro(GenerateRectilinearGrids, int);

  // Description:
  // Limit this source to discrete integer time steps
  // Default is off (continuous)
  vtkSetMacro(DiscreteTimeSteps, int);
  vtkGetMacro(DiscreteTimeSteps, int);
  vtkBooleanMacro(DiscreteTimeSteps, int);

  // Description:
  // Make a 2D data set to test.
  vtkSetMacro(TwoDimensional, int);
  vtkGetMacro(TwoDimensional, int);
  vtkBooleanMacro(TwoDimensional, int);

  // Description:
  // Test the case when the blocks do not have the same sizes.
  // Adds 2 to the x extent of the far x blocks (level 1).
  vtkSetMacro(Asymetric,int);
  vtkGetMacro(Asymetric,int);

  // Description:
  // Make the division adaptive or not, defaults to Adaptive
  vtkSetMacro(AdaptiveSubdivision, int);
  vtkGetMacro(AdaptiveSubdivision, int);
  vtkBooleanMacro(AdaptiveSubdivision, int);


protected:
  vtkTemporalFractal();
  ~vtkTemporalFractal();

  int StartBlock;
  int EndBlock;
  int BlockCount;
  
  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestInformation(vtkInformation *request, 
                                 vtkInformationVector **inputVector, 
                                 vtkInformationVector *outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation *request, 
                          vtkInformationVector **inputVector, 
                          vtkInformationVector *outputVector);
  virtual int RequestOneTimeStep(vtkCompositeDataSet *output,
                                 vtkInformation *request, 
                                 vtkInformationVector **inputVector, 
                                 vtkInformationVector *outputVector);
  
  void Traverse(int &blockId, int level, vtkDataObject* output, 
                int x0,int x1, int y0,int y1, int z0,int z1,
                int onFace[6]);

  int LineTest2(float x0, float y0, float z0, 
                float x1, float y1, float z1,
                double bds[6]); 
  int LineTest(float x0, float y0, float z0, 
               float x1, float y1, float z1,
               double bds[6], int level, int target); 

  void SetBlockInfo(vtkUniformGrid *grid, int level, int* ext,int onFace[6]);
  void SetRBlockInfo(vtkRectilinearGrid *grid, int level, int* ext,
                     int onFace[6]);


  void AddDataSet(vtkDataObject* output, unsigned int level, int extents[6],
    vtkDataSet* dataSet);
  
  void AddVectorArray(vtkHierarchicalBoxDataSet *output);
  void AddTestArray(vtkHierarchicalBoxDataSet *output);
  void AddFractalArray(vtkCompositeDataSet* output);
  void AddBlockIdArray(vtkHierarchicalBoxDataSet *output);
  void AddDepthArray(vtkHierarchicalBoxDataSet *output);
  
  void AddGhostLevelArray(vtkDataSet *grid,
                          int dim[3],
                          int onFace[6]);

  int MandelbrotTest(double x, double y);
  int TwoDTest(double bds[6], int level, int target);

  void CellExtentToBounds(int level,
                          int ext[6],
                          double bds[6]);
  
  void ExecuteRectilinearMandelbrot(vtkRectilinearGrid *grid,
                                    double *ptr);
  double EvaluateSet(double p[4]);
  void GetContinuousIncrements(int extent[6],
                               vtkIdType &incX,
                               vtkIdType &incY,
                               vtkIdType &incZ);
  
  // Dimensions:
  // Specify blocks relative to this top level block.
  // For now this has to be set before the blocks are defined.
  vtkSetVector3Macro(TopLevelSpacing, double);
  vtkGetVector3Macro(TopLevelSpacing, double);
  vtkSetVector3Macro(TopLevelOrigin, double);
  vtkGetVector3Macro(TopLevelOrigin, double);
  
  void InternalImageDataCopy(vtkTemporalFractal *src);

  int Asymetric;
  int MaximumLevel;
  int Dimensions;
  float FractalValue;
  int GhostLevels;
  vtkIntArray *Levels;
  int TwoDimensional;
  int DiscreteTimeSteps;

  // New method of specifing blocks.
  double TopLevelSpacing[3];
  double TopLevelOrigin[3];
  
  int GenerateRectilinearGrids;

  double CurrentTime;
  
  int AdaptiveSubdivision;

private:
  vtkTemporalFractal(const vtkTemporalFractal&);  // Not implemented.
  void operator=(const vtkTemporalFractal&);  // Not implemented.
};


#endif
