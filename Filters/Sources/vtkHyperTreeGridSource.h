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
/**
 * @class   vtkHyperTreeGridSource
 * @brief   Create a synthetic grid of hypertrees.
 *
 *
 * This class uses input parameters, most notably a string descriptor,
 * to generate a vtkHyperTreeGrid instance representing the corresponding
 * tree-based AMR grid. This descriptor uses the following conventions,
 * e.g., to describe a 1-D ternary subdivision with 2 root cells
 * L0    L1        L2
 * RR  | .R. ... | ...
 * For this tree:
 *  HTG:       .
 *           /   \
 *  L0:     .     .
 *         /|\   /|\
 *  L1:   c . c c c c
 *         /|\
 *  L2:   c c c
 * The top level of the tree is not considered a grid level
 * NB: For ease of legibility, white spaces are allowed and ignored.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridSource_h
#define vtkHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <string> // STL Header
#include <map> // STL Header
#include <vector> // STL Header

class vtkBitArray;
class vtkDataArray;
class vtkHyperTreeCursor;
class vtkIdTypeArray;
class vtkImplicitFunction;
class vtkHyperTreeGrid;
class vtkQuadric;

class VTKFILTERSSOURCES_EXPORT vtkHyperTreeGridSource : public vtkHyperTreeGridAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridSource,vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkHyperTreeGridSource* New();

  /**
   * Return the maximum number of levels of the hypertree.
   * \post positive_result: result>=1
   */
  unsigned int GetMaximumLevel();

  /**
   * Set the maximum number of levels of the hypertrees.
   * \pre positive_levels: levels>=1
   * \post is_set: this->GetLevels()==levels
   * \post min_is_valid: this->GetMinLevels()<this->GetLevels()
   */
  void SetMaximumLevel( unsigned int levels );

  //@{
  /**
   * Set/Get the origin of the grid
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Set/Get the scale to be applied to root cells in each dimension of the grid
   */
  vtkSetVector3Macro(GridScale, double);
  vtkGetVector3Macro(GridScale, double);
  //@}

  //@{
  /**
   * Set/Get the number of root cells in each dimension of the grid
   */
  vtkSetVector3Macro(GridSize, unsigned int);
  vtkGetVector3Macro(GridSize, unsigned int);
  //@}

  //@{
  /**
   * Specify whether indexing mode of grid root cells must be transposed to
   * x-axis first, z-axis last, instead of the default z-axis first, k-axis last
   */
  vtkSetMacro(TransposedRootIndexing, bool);
  vtkGetMacro(TransposedRootIndexing, bool);
  void SetIndexingModeToKJI();
  void SetIndexingModeToIJK();
  //@}

  //@{
  /**
   * Set/Get the dimensionality of the grid
   */
  vtkSetClampMacro(Dimension, unsigned int, 1, 3);
  vtkGetMacro(Dimension, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the orientation of the grid (in 1D and 2D)
   */
  virtual void SetOrientation(unsigned int);
  vtkGetMacro(Orientation, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme
   */
  vtkSetClampMacro(BranchFactor, unsigned int, 2, 3);
  vtkGetMacro(BranchFactor, unsigned int);
  //@}

  //@{
  /**
   * Set/get whether the descriptor string should be used.
   * NB: Otherwise a quadric definition is expected.
   * Default: true
   */
  vtkSetMacro(UseDescriptor, bool);
  vtkGetMacro(UseDescriptor, bool);
  vtkBooleanMacro(UseDescriptor, bool);
  //@}

  //@{
  /**
   * Set/get whether the material mask should be used.
   * NB: This is only used when UseDescriptor is ON
   * Default: false
   */
  vtkSetMacro(UseMaterialMask, bool);
  vtkGetMacro(UseMaterialMask, bool);
  vtkBooleanMacro(UseMaterialMask, bool);
  //@}

  //@{
  /**
   * Set/get whether cell-centered interface fields
   * should be generated.
   * Default: false
   */
  vtkSetMacro(GenerateInterfaceFields, bool);
  vtkGetMacro(GenerateInterfaceFields, bool);
  vtkBooleanMacro(GenerateInterfaceFields, bool);
  //@}

  //@{
  /**
   * Set/Get the string used to describe the grid.
   */
  vtkSetStringMacro(Descriptor);
  vtkGetStringMacro(Descriptor);
  //@}

  //@{
  /**
   * Set/Get the string used to as a material mask.
   */
  vtkSetStringMacro(MaterialMask);
  vtkGetStringMacro(MaterialMask);
  //@}

  //@{
  /**
   * Set/Get the bitarray used to describe the grid.
   */
  virtual void SetDescriptorBits( vtkBitArray* );
  vtkGetObjectMacro( DescriptorBits, vtkBitArray );
  //@}

  /**
   * Set the index array used to as a material mask.
   */
  virtual void SetLevelZeroMaterialIndex( vtkIdTypeArray* );

  //@{
  /**
   * Set/Get the bitarray used as a material mask.
   */
  virtual void SetMaterialMaskBits( vtkBitArray* );
  vtkGetObjectMacro( MaterialMaskBits, vtkBitArray );
  //@}

  //@{
  /**
   * Set/Get the quadric function.
   */
  virtual void SetQuadric( vtkQuadric* );
  vtkGetObjectMacro(Quadric, vtkQuadric);
  //@}

  //@{
  /**
   * Helpers to set/get the 10 coefficients of the quadric function
   */
  void SetQuadricCoefficients( double[10] );
  void GetQuadricCoefficients( double[10] );
  double* GetQuadricCoefficients();
  //@}

  /**
   * Override GetMTime because we delegate to a vtkQuadric.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Helpers to convert string descriptors & mask to bit arrays
   */
  vtkBitArray* ConvertDescriptorStringToBitArray( const std::string& );
  vtkBitArray* ConvertMaterialMaskStringToBitArray( const std::string& );
  //@}

protected:
  vtkHyperTreeGridSource();
  ~vtkHyperTreeGridSource() override;

  int RequestInformation( vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector* ) override;

  int RequestData( vtkInformation*,
                   vtkInformationVector**,
                   vtkInformationVector* ) override;

  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Main routine to process individual trees in the grid
   */
  int ProcessTrees( vtkHyperTreeGrid*,
                    vtkDataObject* ) override;

  /**
   * Initialize grid from descriptor string when it is to be used. The extent
   * is used to determine what parts of *Descriptor* to use if the grid is
   * partitioned.
   */
  int InitializeFromStringDescriptor(int* extent);

  /**
   * Initialize grid from bit array descriptors when it is to be used
   */
  int InitializeFromBitsDescriptor();

  /**
   * Initialize tree grid from descriptor and call subdivide if needed
   */
  void InitTreeFromDescriptor( vtkHyperTreeGrid* output,
                               vtkHyperTreeCursor* cursor,
                               int treeIdx,
                               int idx[3] );

  /**
   * Subdivide grid from descriptor string when it is to be used
   */
  void SubdivideFromStringDescriptor( vtkHyperTreeGrid* output,
                                      vtkHyperTreeCursor* cursor,
                                      unsigned int level,
                                      int treeIdx,
                                      int childIdx,
                                      int idx[3],
                                      int parentPos );

  /**
   * Subdivide grid from descriptor string when it is to be used
   */
  void SubdivideFromBitsDescriptor( vtkHyperTreeGrid* output,
                                    vtkHyperTreeCursor* cursor,
                                    unsigned int level,
                                    int treeIdx,
                                    int childIdx,
                                    int idx[3],
                                    int parentPos );

  /**
   * Subdivide grid from quadric when descriptor is not used
   */
  void SubdivideFromQuadric( vtkHyperTreeGrid* output,
                             vtkHyperTreeCursor* cursor,
                             unsigned int level,
                             int treeIdx,
                             const int idx[3],
                             double origin[3],
                             double size[3] );

  /**
   * Evaluate quadric at given point coordinates
   */
  double EvaluateQuadric( double[3] );

  double Origin[3];
  double GridScale[3];
  unsigned int GridSize[3];
  bool TransposedRootIndexing;
  unsigned int MaximumLevel;
  unsigned int Dimension;
  unsigned int Orientation;
  unsigned int BranchFactor;
  unsigned int BlockSize;
  bool UseDescriptor;
  bool UseMaterialMask;
  bool GenerateInterfaceFields;

  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;

  char* Descriptor;
  char* MaterialMask;
  std::vector<std::string> LevelDescriptors;
  std::vector<std::string> LevelMaterialMasks;

  vtkBitArray* DescriptorBits;
  vtkBitArray* MaterialMaskBits;
  std::vector<vtkIdType> LevelBitsIndex;
  std::vector<vtkIdType> LevelBitsIndexCnt;

  vtkIdTypeArray* LevelZeroMaterialIndex;
  std::map<vtkIdType, vtkIdType> LevelZeroMaterialMap;

  std::vector<int> LevelCounters;

  vtkQuadric* Quadric;

  vtkHyperTreeGrid* OutputHTG;

private:
  vtkHyperTreeGridSource(const vtkHyperTreeGridSource&) = delete;
  void operator=(const vtkHyperTreeGridSource&) = delete;
};

#endif
