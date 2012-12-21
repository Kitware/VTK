/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperTreeGridSource - Create a synthetic grid of hypertrees.
//
// .SECTION Description
// This class uses input parameters, most notably a string descriptor,
// to generate a vtkHyperTreeGrid instance representing the corresponding
// tree-based AMR grid. This descriptor uses the following conventions,
// e.g., to describe a 1-D ternary subdivision with 2 root cells
// L0    L1        L2
// RR  | .R. ... | ...
// For this tree:
/*  HTG:       .         */
/*           /   \       */
/*  L0:     .     .      */
/*         /|\   /|\     */
/*  L1:   c . c c c c    */
/*         /|\           */
/*  L2:   c c c          */
// The top level of the tree is not considered a grid level
// NB: For ease of legibility, white spaces are allowed and ignored.
//
// .SECTION Thanks
// This class was written by Philippe Pebay, Kitware SAS 2012

#ifndef __vtkHyperTreeGridSource_h
#define __vtkHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkStdString.h" // For vtkStdString ivars

#include <vector> // STL Header

class vtkDataArray;
class vtkImplicitFunction;
class vtkHyperTreeGrid;
class vtkQuadric;

class VTKFILTERSSOURCES_EXPORT vtkHyperTreeGridSource : public vtkHyperTreeGridAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridSource,vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHyperTreeGridSource* New();

  // Description:
  // Return the maximum number of levels of the hypertree.
  // \post positive_result: result>=1
  unsigned int GetMaximumLevel();

  // Description:
  // Set the maximum number of levels of the hypertree.
  // \pre positive_levels: levels>=1
  // \post is_set: this->GetLevels()==levels
  void SetMaximumLevel( unsigned int levels );

  // Description:
  // Set/Get the scale to be applied to root cells in each dimension of the grid
  vtkSetVector3Macro(GridScale, double);
  vtkGetVector3Macro(GridScale, double);

  // Description:
  // Set/Get the number of root cells in each dimension of the grid
  vtkSetVector3Macro(GridSize, unsigned int);
  vtkGetVector3Macro(GridSize, unsigned int);

  // Description:
  // Set/Get the subdivision factor in the grid refinement scheme
  vtkSetClampMacro(BranchFactor, unsigned int, 2, 3);
  vtkGetMacro(BranchFactor, unsigned int);

  // Description:
  // Set/Get the dimensionality of the grid
  vtkSetClampMacro(Dimension, unsigned int, 2, 3);
  vtkGetMacro(Dimension, unsigned int);

  // Description:
  // Set/Get whether the dual grid interface is the default one
  // Default: false
  vtkSetMacro(Dual, bool);
  vtkGetMacro(Dual, bool);
  vtkBooleanMacro(Dual, bool);

  // Description:
  // Set/get whether the descriptor string should be used.
  // NB: Otherwise a quadric definition is expected.
  // Default: true
  vtkSetMacro(UseDescriptor, bool);
  vtkGetMacro(UseDescriptor, bool);
  vtkBooleanMacro(UseDescriptor, bool);

  // Description:
  // Set/get whether the material mask should be used.
  // NB: This is only used when UseDescriptor is ON
  // Default: false
  vtkSetMacro(UseMaterialMask, bool);
  vtkGetMacro(UseMaterialMask, bool);
  vtkBooleanMacro(UseMaterialMask, bool);

  // Description:
  // Set/Get the string used to describe the grid
  virtual void SetDescriptor( const vtkStdString& );
  virtual vtkStdString GetDescriptor();

  // Description:
  // Set/Get the string used to as a material mask
  virtual void SetMaterialMask( const vtkStdString& );
  virtual vtkStdString GetMaterialMask();

  // Description
  // Set/Get the 10 coefficients of the quadric function
  void SetQuadricCoefficients( double[10] );
  void SetQuadricCoefficients( double, double, double, double, double,
                               double, double, double, double, double );
  void GetQuadricCoefficients( double a[10] );
  double* GetQuadricCoefficients();

  // Description:
  // Override GetMTime because we delegate to a vtkQuadric
  unsigned long GetMTime();

protected:
  vtkHyperTreeGridSource();
  ~vtkHyperTreeGridSource();

  int RequestInformation ( vtkInformation*,
                           vtkInformationVector**,
                           vtkInformationVector* );

  virtual int RequestData( vtkInformation*,
                           vtkInformationVector**,
                           vtkInformationVector* );

  // Description:
  // Initialize grid from descriptor string when it is to be used
  int InitializeFromDescriptor();

  // Description:
  // Subdivide grid from descriptor string when it is to be used
  void SubdivideFromDescriptor( vtkHyperTreeCursor* cursor,
                                unsigned int level,
                                int treeIdx,
                                int childIdx,
                                int idx[3],
                                int cellIdOffset,
                                int parentPos );

  // Description:
  // Subdivide grid from quadric when descriptor is not used
  void SubdivideFromQuadric( vtkHyperTreeCursor* cursor,
                             unsigned int level,
                             int treeIdx,
                             int idx[3],
                             int cellIdOffset,
                             double origin[3],
                             double size[3] );

  // Description:
  // Evaluate quadric at given point coordinates
  double EvaluateQuadric( double[3] );

  double GridScale[3];
  unsigned int GridSize[3];
  unsigned int MaximumLevel;
  unsigned int Dimension;
  unsigned int BranchFactor;
  unsigned int BlockSize;
  bool Dual;
  bool UseDescriptor;
  bool UseMaterialMask;

  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;

  vtkStdString Descriptor;
  vtkStdString MaterialMask;
  std::vector<vtkStdString> LevelDescriptors;
  std::vector<vtkStdString> LevelMaterialMasks;
  std::vector<int> LevelCounters;

  vtkQuadric* Quadric;

  vtkHyperTreeGrid* Output;

private:
  vtkHyperTreeGridSource(const vtkHyperTreeGridSource&);  // Not implemented.
  void operator=(const vtkHyperTreeGridSource&);  // Not implemented.
};

#endif
