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
/**
 * @class   vtkTemporalFractal
 * @brief   A source to test AMR data object.
 *
 * vtkTemporalFractal is a collection of uniform grids.  All have the same
 * dimensions. Each block has a different origin and spacing.  It uses
 * mandelbrot to create cell data. The fractal array is scaled to look like a
 * volume fraction.
 *
 * I may also add block id and level as extra cell arrays.
 * This source produces a vtkHierarchicalBoxDataSet when
 * GenerateRectilinearGrids is off, otherwise produces a vtkMultiBlockDataSet.
*/

#ifndef vtkTemporalFractal_h
#define vtkTemporalFractal_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkSmartPointer.h" //for ivars

class vtkCompositeDataSet;
class vtkDataSet;
class vtkHierarchicalBoxDataSet;
class vtkIntArray;
class vtkRectilinearGrid;
class vtkUniformGrid;
class TemporalFractalOutputUtil;

class VTKFILTERSHYBRID_EXPORT vtkTemporalFractal: public vtkAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkTemporalFractal *New();
  vtkTypeMacro(vtkTemporalFractal,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Essentially the iso surface value.  The fractal array is scaled to map
   * this value to 0.5 for use as a volume fraction.
   */
  vtkSetMacro(FractalValue, float);
  vtkGetMacro(FractalValue, float);
  //@}

  //@{
  /**
   * Any blocks touching a predefined line will be subdivided to this level.
   * Other blocks are subdivided so that neighboring blocks only differ
   * by one level.
   */
  vtkSetMacro(MaximumLevel, int);
  vtkGetMacro(MaximumLevel, int);
  //@}

  //@{
  /**
   * XYZ dimensions of cells.
   */
  vtkSetMacro(Dimensions, int);
  vtkGetMacro(Dimensions, int);
  //@}

  //@{
  /**
   * For testing ghost levels.
   */
  vtkSetMacro(GhostLevels, vtkTypeBool);
  vtkGetMacro(GhostLevels, vtkTypeBool);
  vtkBooleanMacro(GhostLevels, vtkTypeBool);
  //@}

  //@{
  /**
   * Generate either rectilinear grids either uniform grids.
   * Default is false.
   */
  vtkSetMacro(GenerateRectilinearGrids, vtkTypeBool);
  vtkGetMacro(GenerateRectilinearGrids, vtkTypeBool);
  vtkBooleanMacro(GenerateRectilinearGrids, vtkTypeBool);
  //@}

  //@{
  /**
   * Limit this source to discrete integer time steps
   * Default is off (continuous)
   */
  vtkSetMacro(DiscreteTimeSteps, vtkTypeBool);
  vtkGetMacro(DiscreteTimeSteps, vtkTypeBool);
  vtkBooleanMacro(DiscreteTimeSteps, vtkTypeBool);
  //@}

  //@{
  /**
   * Make a 2D data set to test.
   */
  vtkSetMacro(TwoDimensional, vtkTypeBool);
  vtkGetMacro(TwoDimensional, vtkTypeBool);
  vtkBooleanMacro(TwoDimensional, vtkTypeBool);
  //@}

  //@{
  /**
   * Test the case when the blocks do not have the same sizes.
   * Adds 2 to the x extent of the far x blocks (level 1).
   */
  vtkSetMacro(Asymmetric,int);
  vtkGetMacro(Asymmetric,int);
  //@}

  //@{
  /**
   * Make the division adaptive or not, defaults to Adaptive
   */
  vtkSetMacro(AdaptiveSubdivision, vtkTypeBool);
  vtkGetMacro(AdaptiveSubdivision, vtkTypeBool);
  vtkBooleanMacro(AdaptiveSubdivision, vtkTypeBool);
  //@}


protected:
  vtkTemporalFractal();
  ~vtkTemporalFractal() override;

  int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;

  int StartBlock;
  int EndBlock;
  int BlockCount;

  /**
   * see vtkAlgorithm for details
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector) override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);
  virtual int RequestOneTimeStep(vtkCompositeDataSet *output,
                                 vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);
  //@}

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

  int Asymmetric;
  int MaximumLevel;
  int Dimensions;
  float FractalValue;
  vtkTypeBool GhostLevels;
  vtkIntArray *Levels;
  vtkTypeBool TwoDimensional;
  vtkTypeBool DiscreteTimeSteps;

  // New method of specifying blocks.
  double TopLevelSpacing[3];
  double TopLevelOrigin[3];

  vtkTypeBool GenerateRectilinearGrids;

  double CurrentTime;

  vtkTypeBool AdaptiveSubdivision;
  vtkSmartPointer<TemporalFractalOutputUtil> OutputUtil;

private:
  vtkTemporalFractal(const vtkTemporalFractal&) = delete;
  void operator=(const vtkTemporalFractal&) = delete;
};


#endif
