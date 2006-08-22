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

#ifndef __vtkTemporalFractal_h
#define __vtkTemporalFractal_h

#include "vtkTemporalDataSetAlgorithm.h"

class vtkIntArray;
class vtkUniformGrid;
class vtkRectilinearGrid;
class vtkDataSet;
class vtkHierarchicalDataSet;

class VTK_EXPORT vtkTemporalFractal : public vtkTemporalDataSetAlgorithm
{
public:
  static vtkTemporalFractal *New();
  
  vtkTypeRevisionMacro(vtkTemporalFractal,vtkTemporalDataSetAlgorithm);
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
  // Make a 2D data set to test.
  vtkSetMacro(TwoDimensional, int);
  vtkGetMacro(TwoDimensional, int);
  vtkBooleanMacro(TwoDimensional, int);

  // Description:
  // Test the case when the blocks do not have the same sizes.
  // Adds 2 to the x extent of the far x blocks (level 1).
  vtkSetMacro(Asymetric,int);
  vtkGetMacro(Asymetric,int);

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
  virtual int RequestOneTimeStep(vtkHierarchicalDataSet *output,
                                 vtkInformation *request, 
                                 vtkInformationVector **inputVector, 
                                 vtkInformationVector *outputVector);
  
  void Traverse(int &blockId, int level, vtkHierarchicalDataSet* output, 
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
  
  void AddVectorArray(vtkHierarchicalDataSet *output);
  void AddTestArray(vtkHierarchicalDataSet *output);
  void AddFractalArray(vtkHierarchicalDataSet *output);
  void AddBlockIdArray(vtkHierarchicalDataSet *output);
  void AddDepthArray(vtkHierarchicalDataSet *output);
  
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

  // New method of specifing blocks.
  double TopLevelSpacing[3];
  double TopLevelOrigin[3];
  
  int GenerateRectilinearGrids;

  double CurrentTime;

private:
  vtkTemporalFractal(const vtkTemporalFractal&);  // Not implemented.
  void operator=(const vtkTemporalFractal&);  // Not implemented.
};


#endif
