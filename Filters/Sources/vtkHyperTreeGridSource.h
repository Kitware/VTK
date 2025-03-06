// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 * In a parallel context, root level trees can be assigned piece numbers in the string descriptor
 * Prefix trees with a digit from 0 to 9 to assign it to a distributed piece. The digit prefix acts
 * as a switch, staying active until another digit is specified. For example 0R.R 1R 0RR 2..R |
 * [...] descriptor will assign the first 3 trees to piece 0, the next one to piece 1, the 2 next to
 * piece 0 and the last 3 to piece 2.
 *
 * When no prefix is specified, all trees belong to piece 0 by default.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridSource_h
#define vtkHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <map>    // STL Header
#include <string> // STL Header
#include <vector> // STL Header

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkDataArray;
class vtkHyperTreeGridNonOrientedCursor;
class vtkIdTypeArray;
class vtkImplicitFunction;
class vtkHyperTreeGrid;
class vtkQuadric;

class VTKFILTERSSOURCES_EXPORT vtkHyperTreeGridSource : public vtkHyperTreeGridAlgorithm
{
public:
  vtkTypeMacro(vtkHyperTreeGridSource, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkHyperTreeGridSource* New();

  /**
   * Return the maximum number of levels of the hypertree.
   * \post positive_result: result>=1
   */
  unsigned int GetMaxDepth();

  /**
   * Set the maximum number of levels of the hypertrees.
   * \pre positive_levels: levels>=1
   * \post is_set: this->GetLevels()==levels
   * \post min_is_valid: this->GetMinLevels()<this->GetLevels()
   */
  void SetMaxDepth(unsigned int levels);

  ///@{
  /**
   * Set/Get the origin of the grid
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Set/Get the scale to be applied to root cells in each dimension of the grid
   */
  vtkSetVector3Macro(GridScale, double);
  vtkGetVector3Macro(GridScale, double);
  void SetGridScale(double scale) { this->SetGridScale(scale, scale, scale); }
  ///@}

  ///@{
  /**
   * Set/Get the number of root cells + 1 in each dimension of the grid
   */
  void SetDimensions(const unsigned int* dims);
  void SetDimensions(unsigned int, unsigned int, unsigned int);
  vtkGetVector3Macro(Dimensions, unsigned int);
  ///@}

  ///@{
  /**
   * Specify whether indexing mode of grid root cells must be transposed to
   * x-axis first, z-axis last, instead of the default z-axis first, k-axis last
   */
  vtkSetMacro(TransposedRootIndexing, bool);
  vtkGetMacro(TransposedRootIndexing, bool);
  void SetIndexingModeToKJI();
  void SetIndexingModeToIJK();
  ///@}

  ///@{
  /**
   * Set/Get the orientation of the grid (in 1D and 2D)
   */
  vtkGetMacro(Orientation, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme
   */
  vtkSetClampMacro(BranchFactor, unsigned int, 2, 3);
  vtkGetMacro(BranchFactor, unsigned int);
  ///@}

  ///@{
  /**
   * Set/get whether the descriptor string should be used.
   * NB: Otherwise a quadric definition is expected.
   * Default: true
   */
  vtkSetMacro(UseDescriptor, bool);
  vtkGetMacro(UseDescriptor, bool);
  vtkBooleanMacro(UseDescriptor, bool);
  ///@}

  ///@{
  /**
   * Set/get whether the material mask should be used.
   * NB: This is only used when UseDescriptor is ON
   * Default: false
   */
  vtkSetMacro(UseMask, bool);
  vtkGetMacro(UseMask, bool);
  vtkBooleanMacro(UseMask, bool);
  ///@}

  ///@{
  /**
   * Set/get whether cell-centered interface fields
   * should be generated.
   * Default: false
   */
  vtkSetMacro(GenerateInterfaceFields, bool);
  vtkGetMacro(GenerateInterfaceFields, bool);
  vtkBooleanMacro(GenerateInterfaceFields, bool);
  ///@}

  ///@{
  /**
   * Set/Get the string used to describe the grid.
   */
  vtkSetStringMacro(Descriptor);
  vtkGetStringMacro(Descriptor);
  ///@}

  ///@{
  /**
   * Set/Get the string used to as a material mask.
   */
  vtkSetStringMacro(Mask);
  vtkGetStringMacro(Mask);
  ///@}

  ///@{
  /**
   * Set/Get the bitarray used to describe the grid.
   */
  virtual void SetDescriptorBits(vtkBitArray*);
  vtkGetObjectMacro(DescriptorBits, vtkBitArray);
  ///@}

  /**
   * Set the index array used to as a material mask.
   */
  virtual void SetLevelZeroMaterialIndex(vtkIdTypeArray*);

  ///@{
  /**
   * Set/Get the bitarray used as a material mask.
   */
  virtual void SetMaskBits(vtkBitArray*);
  vtkGetObjectMacro(MaskBits, vtkBitArray);
  ///@}

  ///@{
  /**
   * Set/Get the quadric function.
   */
  virtual void SetQuadric(vtkQuadric*);
  vtkGetObjectMacro(Quadric, vtkQuadric);
  ///@}

  ///@{
  /**
   * Helpers to set/get the 10 coefficients of the quadric function
   */
  void SetQuadricCoefficients(double[10]);
  void GetQuadricCoefficients(double[10]);
  double* GetQuadricCoefficients();
  ///@}

  /**
   * Override GetMTime because we delegate to a vtkQuadric.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Helpers to convert string descriptors & mask to bit arrays
   */
  vtkBitArray* ConvertDescriptorStringToBitArray(const std::string&);
  vtkBitArray* ConvertMaskStringToBitArray(const std::string&);
  ///@}

protected:
  vtkHyperTreeGridSource();
  ~vtkHyperTreeGridSource() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to process individual trees in the grid
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Initialize grid from descriptor string when it is to be used
   */
  int InitializeFromStringDescriptor();

  /**
   * Initialize grid from bit array descriptors when it is to be used
   */
  int InitializeFromBitsDescriptor();

  /**
   * Initialize tree grid from descriptor and call subdivide if needed
   */
  void InitTreeFromDescriptor(vtkHyperTreeGrid* output, vtkHyperTreeGridNonOrientedCursor* cursor,
    int treeIdx, int idx[3], int offset = 0);

  /**
   * Subdivide grid from descriptor string when it is to be used
   * `offset` represents the offset reading in the root level descriptor, caused by process number
   * specifiers.
   */
  void SubdivideFromStringDescriptor(vtkHyperTreeGrid* output,
    vtkHyperTreeGridNonOrientedCursor* cursor, unsigned int level, int treeIdx, int childIdx,
    int idx[3], int parentPos, int offset = 0);

  /**
   * Subdivide grid from descriptor string when it is to be used
   */
  void SubdivideFromBitsDescriptor(vtkHyperTreeGrid* output,
    vtkHyperTreeGridNonOrientedCursor* cursor, unsigned int level, int treeIdx, int childIdx,
    int idx[3], int parentPos);

  /**
   * Subdivide grid from quadric when descriptor is not used
   */
  void SubdivideFromQuadric(vtkHyperTreeGrid* output, vtkHyperTreeGridNonOrientedCursor* cursor,
    unsigned int level, int treeIdx, const int idx[3], double origin[3], double size[3]);

  /**
   * Evaluate quadric at given point coordinates
   */
  double EvaluateQuadric(double[3]);

  double Origin[3];
  double GridScale[3];
  unsigned int Dimension;

  unsigned int Dimensions[3];
  bool TransposedRootIndexing;
  unsigned int MaxDepth;

  unsigned int Orientation;
  unsigned int BranchFactor;
  unsigned int BlockSize;
  bool UseDescriptor;
  bool UseMask;
  bool GenerateInterfaceFields;

  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;

  char* Descriptor;
  char* Mask;
  std::vector<std::string> LevelDescriptors;
  std::vector<std::string> LevelMasks;

  vtkBitArray* DescriptorBits;
  vtkBitArray* MaskBits;
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

  // Multi-piece utilities
  int Piece = 0;
  int NumPieces = 1;
  int CurrentTreeProcess = 0; // Track the process where next root trees should go

  /**
   * Return true if current level size is consistent: at root level, nTotal == nLeaves+nRefined,
   * and on lower levels, descriptor size matching nNextLevel predicted number of cells.
   * Logs error messages on failure.
   */
  bool IsLevelDescriptorConsistent(bool isRootLevel, unsigned int nRefined, unsigned int nLeaves,
    unsigned int nTotal, unsigned int nNextLevel, const std::ostringstream& descriptor);
};

VTK_ABI_NAMESPACE_END
#endif
