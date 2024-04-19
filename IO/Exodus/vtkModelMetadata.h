// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkModelMetadata
 * @brief   This class encapsulates the metadata
 *   that appear in mesh-based file formats but do not appear in
 *   vtkUnstructuredGrid.
 *
 *
 *   This class is inspired by the Exodus II file format, but
 *   because this class does not depend on the Exodus library, it
 *   should be possible to use it to represent metadata for other
 *   dataset file formats.  Sandia Labs uses it in their Exodus II
 *   reader, their Exodus II writer and their EnSight writer.
 *   vtkDistributedDataFilter looks for metadata attached to
 *   it's input and redistributes the metadata with the grid.
 *
 *   The fields in this class are those described in the document
 *   "EXODUS II: A Finite Element Data Model", SAND92-2137, November 1995.
 *
 *   Element and node IDs stored in this object must be global IDs,
 *   in the event that the original dataset was partitioned across
 *   many files.
 *
 *   One way to initialize this object is by using vtkExodusModel
 *   (a Sandia class used by the Sandia Exodus reader).
 *   That class will take an open Exodus II file and a
 *   vtkUnstructuredGrid drawn from it and will set the required fields.
 *
 *   Alternatively, you can use all the Set*
 *   methods to set the individual fields. This class does not
 *   copy the data, it simply uses your pointer. This
 *   class will free the storage associated with your pointer
 *   when the class is deleted.  Most fields have sensible defaults.
 *   The only requirement is that if you are using this ModelMetadata
 *   to write out an Exodus or EnSight file in parallel, you must
 *   SetBlockIds and SetBlockIdArrayName.  Your vtkUnstructuredGrid must
 *   have a cell array giving the block ID for each cell.
 *
 * @warning
 *   The Exodus II library supports an optimized element order map
 *   (section 3.7 in the SAND document).  It contains all the element
 *   IDs, listed in the order in which a solver should process them.
 *   We don't include this, and won't unless there is a request.
 *
 * @warning
 *   There is an assumption in some classes that the name of the cell
 *   array containing global element ids is "GlobalElementId" and the
 *   name of the point array containing global node ids is "GlobalNodeId".
 *   (element == cell) and (node == point).
 *
 * @sa
 *   vtkDistributedDataFilter vtkExtractCells
 */

#ifndef vtkModelMetadata_h
#define vtkModelMetadata_h

#include "vtkIOExodusModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkStringArray.h"  // for vtkStringArray
VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkCharArray;
class vtkIdTypeArray;
class vtkIntArray;
class vtkFloatArray;
class vtkIntArray;
class vtkStringArray;
class vtkModelMetadataSTLCloak;

class VTKIOEXODUS_EXPORT vtkModelMetadata : public vtkObject
{
public:
  vtkTypeMacro(vtkModelMetadata, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkModelMetadata* New();

  /**
   * The global fields are those which pertain to the whole
   * file.  Examples are the title, information lines,
   * and list of block IDs.  This method prints out all the
   * global information.
   */

  virtual void PrintGlobalInformation();

  /**
   * The local fields are those which depend on exactly which
   * blocks, which time step, and which variables you read in
   * from the file.  Examples are the number of cells in
   * each block, and the list of nodes in a node set, or the
   * value of the global variables at a time step.  If
   * VERBOSE_TESTING is defined in your execution environment,
   * this method will print more than mere counts, and actually
   * print a few of the IDs, distribution factors and so on.  If
   * VERY_VERBOSE_TESTING is defined, it will print out
   * all ID lists, distribution factor lists, and so on.
   */

  virtual void PrintLocalInformation();

  ///@{
  /**
   * The title of the dataset.
   */
  vtkSetStringMacro(Title);
  const char* GetTitle() const { return this->Title; }
  ///@}

  /**
   * Set the information lines.
   */
  void SetInformationLines(int numLines, char** lines);

  /**
   * Get a pointer to all the information lines.  The number
   * of lines is returned;
   */
  int GetInformationLines(char*** lines) const;

  /**
   * Get the number of information lines.
   */
  int GetNumberOfInformationLines() const { return this->NumberOfInformationLines; }

  ///@{
  /**
   * Set the index of the time step represented by the results
   * data in the file attached to this ModelMetadata object.  Time
   * step indices start at 0 in this file, they start at 1 in
   * an Exodus file.
   */
  vtkSetMacro(TimeStepIndex, int);
  int GetTimeStepIndex() const { return this->TimeStepIndex; }
  ///@}

  /**
   * Set the total number of time steps in the file,
   * and the value at each time step.  We use your time
   * step value array and delete it when we're done.
   */
  void SetTimeSteps(int numberOfTimeSteps, float* timeStepValues);
  int GetNumberOfTimeSteps() const { return this->NumberOfTimeSteps; }

  /**
   * Get the time step values
   */
  float* GetTimeStepValues() const { return this->TimeStepValues; }

  /**
   * The name of the one, two or three coordinate dimensions.
   */
  void SetCoordinateNames(int dimension, char**);
  char** GetCoordinateNames() const { return this->CoordinateNames; }

  /**
   * Get the dimension of the model.  This is also the number
   * of coordinate names.
   */
  int GetDimension() const { return this->Dimension; }

  ///@{
  /**
   * The number of blocks in the file.  Set this before setting
   * any of the block arrays.
   */
  vtkSetMacro(NumberOfBlocks, int);
  int GetNumberOfBlocks() const { return this->NumberOfBlocks; }
  ///@}

  /**
   * An arbitrary integer ID for each block.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetBlockIds(int*);
  int* GetBlockIds() const { return this->BlockIds; }

  /**
   * Element type for each block - a name that means
   * something to person who created the file.
   * We use your pointers, and free the memory when the object is freed.
   */
  void SetBlockElementType(char**);
  char** GetBlockElementType() const { return this->BlockElementType; }

  /**
   * Set or get a pointer to a list of the number of elements in
   * each block.
   * We use your pointers, and free the memory when the object is freed.
   */
  int SetBlockNumberOfElements(int* nelts);
  int* GetBlockNumberOfElements() const { return this->BlockNumberOfElements; }

  /**
   * Set or get a pointer to a list of the number of nodes in the
   * elements of each block.
   * We use your pointers, and free the memory when the object is freed.
   */
  void SetBlockNodesPerElement(int*);
  int* GetBlockNodesPerElement() const { return this->BlockNodesPerElement; }

  /**
   * Set or get a pointer to a list global element IDs for the
   * elements in each block.
   * We use your pointers, and free the memory when the object is freed.
   */
  void SetBlockElementIdList(int*);
  int* GetBlockElementIdList() const { return this->BlockElementIdList; }

  /**
   * Get the length of the list of elements in every block.
   */
  int GetSumElementsPerBlock() const { return this->SumElementsPerBlock; }

  /**
   * Get a list of the index into the BlockElementIdList of the
   * start of each block's elements.
   */
  int* GetBlockElementIdListIndex() const { return this->BlockElementIdListIndex; }

  /**
   * Set or get a pointer to a list of the number of attributes
   * stored for the elements in each block.
   * We use your pointers, and free the memory when the object is freed.
   */
  int SetBlockNumberOfAttributesPerElement(int* natts);
  int* GetBlockNumberOfAttributesPerElement() const
  {
    return this->BlockNumberOfAttributesPerElement;
  }

  /**
   * Set or get a pointer to a list of the attributes for all
   * blocks.  The order of the list should be by block, by element
   * within the block, by attribute.  Omit blocks that don't
   * have element attributes.
   */
  void SetBlockAttributes(float*);
  float* GetBlockAttributes() const { return this->BlockAttributes; }

  /**
   * Get the length of the list of floating point block attributes.
   */
  int GetSizeBlockAttributeArray() const { return this->SizeBlockAttributeArray; }

  /**
   * Get a list of the index into the BlockAttributes of the
   * start of each block's element attribute list.
   */
  int* GetBlockAttributesIndex() const { return this->BlockAttributesIndex; }

  ///@{
  /**
   * The number of node sets in the file.  Set this value before
   * setting the various node set arrays.
   */
  vtkSetMacro(NumberOfNodeSets, int);
  int GetNumberOfNodeSets() const { return this->NumberOfNodeSets; }
  ///@}

  void SetNodeSetNames(vtkStringArray* names) { this->NodeSetNames = names; }
  vtkStringArray* GetNodeSetNames() const { return this->NodeSetNames; }

  /**
   * Set or get the list the IDs for each node set.
   * Length of list is the number of node sets.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetNodeSetIds(int*);
  int* GetNodeSetIds() const { return this->NodeSetIds; }

  /**
   * Set or get a pointer to a list of the number of nodes in each node set.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetNodeSetSize(int*);
  int* GetNodeSetSize() const { return this->NodeSetSize; }

  /**
   * Set or get a pointer to a concatenated list of the
   * IDs of all nodes in each node set.  First list all IDs in
   * node set 0, then all IDs in node set 1, and so on.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetNodeSetNodeIdList(int*);
  int* GetNodeSetNodeIdList() const { return this->NodeSetNodeIdList; }

  /**
   * Set or get a list of the number of distribution factors stored
   * by each node set.  This is either 0 or equal to the number of
   * nodes in the node set.
   * Length of list is number of node sets.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetNodeSetNumberOfDistributionFactors(int*);
  int* GetNodeSetNumberOfDistributionFactors() const
  {
    return this->NodeSetNumberOfDistributionFactors;
  }

  /**
   * Set or get a list of the distribution factors for the node sets.
   * The list is organized by node set, and within node set by node.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetNodeSetDistributionFactors(float*);
  float* GetNodeSetDistributionFactors() const { return this->NodeSetDistributionFactors; }

  ///@{
  /**
   * Get the total number of nodes in all node sets
   */
  vtkSetMacro(SumNodesPerNodeSet, int);
  int GetSumNodesPerNodeSet() const { return this->SumNodesPerNodeSet; }
  ///@}

  /**
   * Get the total number of distribution factors stored for all node sets
   */
  int GetSumDistFactPerNodeSet() const { return this->SumDistFactPerNodeSet; }

  /**
   * Get a list of the index of the starting entry for each node set
   * in the list of node set node IDs.
   */
  int* GetNodeSetNodeIdListIndex() const { return this->NodeSetNodeIdListIndex; }

  /**
   * Get a list of the index of the starting entry for each node set
   * in the list of node set distribution factors.
   */
  int* GetNodeSetDistributionFactorIndex() const { return this->NodeSetDistributionFactorIndex; }

  ///@{
  /**
   * Set or get the number of side sets.  Set this value before
   * setting any of the other side set arrays.
   */
  vtkSetMacro(NumberOfSideSets, int);
  int GetNumberOfSideSets() const { return this->NumberOfSideSets; }
  ///@}

  void SetSideSetNames(vtkStringArray* names) { this->SideSetNames = names; }
  vtkStringArray* GetSideSetNames() const { return this->SideSetNames; }

  /**
   * Set or get a pointer to a list giving the ID of each side set.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetSideSetIds(int*);
  int* GetSideSetIds() const { return this->SideSetIds; }

  /**
   * Set or get a pointer to a list of the number of sides in each side set.
   * We use your pointer, and free the memory when the object is freed.
   */
  int SetSideSetSize(int* sizes);
  int* GetSideSetSize() const { return this->SideSetSize; }

  /**
   * Set or get a pointer to a list of the number of distribution
   * factors stored by each side set.   Each side set has either
   * no distribution factors, or 1 per node in the side set.
   * We use your pointer, and free the memory when the object is freed.
   */
  int SetSideSetNumberOfDistributionFactors(int* df);
  int* GetSideSetNumberOfDistributionFactors() const
  {
    return this->SideSetNumberOfDistributionFactors;
  }

  /**
   * Set or get a pointer to a list of the elements containing each
   * side in each side set.  The list is organized by side set, and
   * within side set by element.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetSideSetElementList(int*);
  int* GetSideSetElementList() const { return this->SideSetElementList; }

  /**
   * Set or get a pointer to the element side for each side in the side set.
   * (See the manual for the convention for numbering sides in different
   * types of cells.)  Side Ids are arranged by side set and within
   * side set by side, and correspond to the SideSetElementList.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetSideSetSideList(int*);
  int* GetSideSetSideList() const { return this->SideSetSideList; }

  /**
   * Set or get a pointer to a list of the number of nodes in each
   * side of each side set.  This list is organized by side set, and
   * within side set by side.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetSideSetNumDFPerSide(int* numNodes);
  int* GetSideSetNumDFPerSide() const { return this->SideSetNumDFPerSide; }

  /**
   * Set or get a pointer to a list of all the distribution factors.
   * For every side set that has distribution factors, the number of
   * factors per node was given in the SideSetNumberOfDistributionFactors
   * array.  If this number for a given side set is N, then for that
   * side set we have N floating point values for each node for each
   * side in the side set.  If nodes are repeated in more than one
   * side, we repeat the distribution factors.  So this list is in order
   * by side set, by node.
   * We use your pointer, and free the memory when the object is freed.
   */
  void SetSideSetDistributionFactors(float*);
  float* GetSideSetDistributionFactors() const { return this->SideSetDistributionFactors; }

  ///@{
  /**
   * Get the total number of sides in all side sets
   */
  vtkSetMacro(SumSidesPerSideSet, int);
  int GetSumSidesPerSideSet() const { return this->SumSidesPerSideSet; }
  ///@}

  /**
   * Get the total number of distribution factors stored for all side sets
   */
  int GetSumDistFactPerSideSet() const { return this->SumDistFactPerSideSet; }

  /**
   * Get a list of the index of the starting entry for each side set
   * in the list of side set side IDs.
   */
  int* GetSideSetListIndex() const { return this->SideSetListIndex; }

  /**
   * Get a list of the index of the starting entry for each side set
   * in the list of side set distribution factors.
   */
  int* GetSideSetDistributionFactorIndex() const { return this->SideSetDistributionFactorIndex; }

  /**
   * The number of block properties (global variables)
   */
  int GetNumberOfBlockProperties() const { return this->NumberOfBlockProperties; }

  /**
   * Set or get the names of the block properties.
   */
  void SetBlockPropertyNames(int numProp, char** names);
  char** GetBlockPropertyNames() const { return this->BlockPropertyNames; }

  /**
   * Set or get value for each variable for each block.  List
   * the integer values in order by variable and within variable
   * by block.
   */
  void SetBlockPropertyValue(int*);
  int* GetBlockPropertyValue() const { return this->BlockPropertyValue; }

  /**
   * The number of node set properties (global variables)
   */
  int GetNumberOfNodeSetProperties() const { return this->NumberOfNodeSetProperties; }

  /**
   * Set or get the names of the node setproperties.
   */
  void SetNodeSetPropertyNames(int numProp, char** names);
  char** GetNodeSetPropertyNames() const { return this->NodeSetPropertyNames; }

  /**
   * Set or get value for each variable for each node set.  List
   * the integer values in order by variable and within variable
   * by node set.
   */
  void SetNodeSetPropertyValue(int*);
  int* GetNodeSetPropertyValue() const { return this->NodeSetPropertyValue; }

  /**
   * The number of side set properties (global variables)
   */
  int GetNumberOfSideSetProperties() const { return this->NumberOfSideSetProperties; }

  /**
   * Set or get the names of the side set properties.
   */
  void SetSideSetPropertyNames(int numProp, char** names);
  char** GetSideSetPropertyNames() const { return this->SideSetPropertyNames; }

  /**
   * Set or get value for each variable for each side set.  List
   * the integer values in order by variable and within variable
   * by side set.
   */
  void SetSideSetPropertyValue(int*);
  int* GetSideSetPropertyValue() const { return this->SideSetPropertyValue; }

  /**
   * Get the number of global variables per time step
   */
  int GetNumberOfGlobalVariables() const { return this->NumberOfGlobalVariables; }

  /**
   * Set or get the names of the global variables
   */
  void SetGlobalVariableNames(int numVarNames, char** n);
  char** GetGlobalVariableNames() const { return this->GlobalVariableNames; }

  /**
   * Set or get the values of the global variables at the current
   * time step.
   */
  void SetGlobalVariableValue(float* f);
  float* GetGlobalVariableValue() const { return this->GlobalVariableValue; }

  /**
   * The ModelMetadata maintains a list of the element variables that
   * were in the original file, and a list of the cell variables
   * in the UGrid derived from that file.  Some of the scalar variables
   * in the original file were combined into vectors in the UGrid.
   * In this method, provide the number of original element variables,
   * the names of the original element variables, the number of
   * element variables in the UGrid, the number of components for each
   * of those variables, and a map from each UGrid variable to the
   * the variable in the list of original names that represents it's
   * first component.
   */
  void SetElementVariableInfo(
    int numOrigNames, char** origNames, int numNames, char** names, int* numComp, int* map);

  /**
   * The ModelMetadata maintains a list of the node variables that
   * were in the original file, and a list of the node variables
   * in the UGrid derived from that file.  Some of the scalar variables
   * in the original file were combined into vectors in the UGrid.
   * In this method, provide the number of original node variables,
   * the names of the original node variables, the number of
   * node variables in the UGrid, the number of components for each
   * of those variables, and a map from each UGrid variable to the
   * the variable in the list of original names that represents it's
   * first component.
   */
  void SetNodeVariableInfo(
    int numOrigNames, char** origNames, int numNames, char** names, int* numComp, int* map);

  /**
   * A truth table indicating which element variables are
   * defined for which blocks. The variables are all the original
   * element variables that were in the file.
   * The table is by block ID and within block ID by variable.
   */
  void SetElementVariableTruthTable(int*);
  int* GetElementVariableTruthTable() const { return this->ElementVariableTruthTable; }

  ///@{
  /**
   * Instead of a truth table of all "1"s, you can set this
   * instance variable to indicate that all variables are
   * defined in all blocks.
   */
  vtkSetMacro(AllVariablesDefinedInAllBlocks, vtkTypeBool);
  vtkBooleanMacro(AllVariablesDefinedInAllBlocks, vtkTypeBool);
  vtkTypeBool GetAllVariablesDefinedInAllBlocks() const
  {
    return this->AllVariablesDefinedInAllBlocks;
  }
  ///@}

  /**
   * The ModelMetadata object may contain these lists:
   * o  the variables in the original data file
   * o  the variables created in the u grid from those original variables
   * o  a mapping from the grid variable names to the original names
   * o  a list of the number of components each grid variable has

   * (Example: Variables in Exodus II files are all scalars.  Some are
   * combined by the ExodusReader into vector variables in the grid.)

   * These methods return names of the original variables, the names
   * of the grid variables, a list of the number of components in
   * each grid variable, and a list of the index into the list of
   * original variable names where the original name of the first
   * component of a grid variable may be found.  The names of subsequent
   * components would immediately follow the name of the first
   * component.
   */
  int GetOriginalNumberOfElementVariables() const { return this->OriginalNumberOfElementVariables; }
  char** GetOriginalElementVariableNames() const { return this->OriginalElementVariableNames; }
  int GetNumberOfElementVariables() const { return this->NumberOfElementVariables; }
  char** GetElementVariableNames() const { return this->ElementVariableNames; }
  int* GetElementVariableNumberOfComponents() const
  {
    return this->ElementVariableNumberOfComponents;
  }
  int* GetMapToOriginalElementVariableNames() const
  {
    return this->MapToOriginalElementVariableNames;
  }

  int GetOriginalNumberOfNodeVariables() const { return this->OriginalNumberOfNodeVariables; }
  char** GetOriginalNodeVariableNames() const { return this->OriginalNodeVariableNames; }
  int GetNumberOfNodeVariables() const { return this->NumberOfNodeVariables; }
  char** GetNodeVariableNames() const { return this->NodeVariableNames; }
  int* GetNodeVariableNumberOfComponents() const { return this->NodeVariableNumberOfComponents; }
  int* GetMapToOriginalNodeVariableNames() const { return this->MapToOriginalNodeVariableNames; }

  ///@{
  /**
   * Free selected portions of the metadata when updating values
   * in the vtkModelMetadata object.  Resetting a particular field,
   * (i.e. SetNodeSetIds) frees the previous setting, but if you
   * are not setting every field, you may want to do a wholesale
   * "Free" first.

   * FreeAllGlobalData frees all the fields which don't depend on
   * which time step, which blocks, or which variables are in the input.
   * FreeAllLocalData frees all the fields which do depend on which
   * time step, blocks or variables are in the input.
   * FreeBlockDependentData frees all metadata fields which depend on
   * which blocks were read in.
   */
  void FreeAllGlobalData();
  void FreeAllLocalData();
  void FreeBlockDependentData();
  void FreeOriginalElementVariableNames();
  void FreeOriginalNodeVariableNames();
  void FreeUsedElementVariableNames();
  void FreeUsedNodeVariableNames();
  void FreeUsedElementVariables();
  void FreeUsedNodeVariables();
  ///@}

  /**
   * Set the object back to it's initial state
   */
  void Reset();

protected:
  vtkModelMetadata();
  ~vtkModelMetadata() override;

private:
  void InitializeAllMetadata();
  void InitializeAllIvars();

  void FreeAllMetadata();
  void FreeAllIvars();

  int BuildBlockElementIdListIndex();
  int BuildBlockAttributesIndex();
  int BuildSideSetDistributionFactorIndex();

  static char* StrDupWithNew(const char* s);

  static int FindNameOnList(char* name, char** list, int listLen);

  void ShowFloats(const char* what, int num, float* f);
  void ShowLines(const char* what, int num, char** l);
  void ShowIntArray(const char* what, int numx, int numy, int* id);
  void ShowInts(const char* what, int num, int* id);
  void ShowListsOfInts(const char* what, int* list, int nlists, int* idx, int len, int verbose);
  void ShowListsOfFloats(const char* what, float* list, int nlists, int* idx, int len, int verbose);

  void SetOriginalElementVariableNames(int nvars, char** names);
  void SetElementVariableNames(int nvars, char** names);
  void SetElementVariableNumberOfComponents(int* comp);
  void SetMapToOriginalElementVariableNames(int* map);

  void SetOriginalNodeVariableNames(int nvars, char** names);
  void SetNodeVariableNames(int nvars, char** names);
  void SetNodeVariableNumberOfComponents(int* comp);
  void SetMapToOriginalNodeVariableNames(int* map);

  int CalculateMaximumLengths(int& maxString, int& maxLine);

  // Fields in Exodus II file and their size (defined in exodusII.h)
  //   (G - global fields, relevant to entire file or file set)
  //   (L - local fields, they differ depending on which cells and nodes are
  //        in a file of a partitioned set, or are read in from file)

  char* Title; // (G)

  int NumberOfInformationLines; // (G)
  char** InformationLine;       // (G)

  int Dimension;          // (G)
  char** CoordinateNames; // (at most 3 of these) (G)

  // Time steps

  int TimeStepIndex;     // starting at 0 (Exodus file starts at 1)
  int NumberOfTimeSteps; // (G)
  float* TimeStepValues; // (G)

  // Block information - arrays that are input with Set*

  int NumberOfBlocks; // (G)

  int* BlockIds;                          // NumberOfBlocks (G) (start at 1)
  char** BlockElementType;                // NumberOfBlocks (G)
  int* BlockNumberOfElements;             // NumberOfBlocks (L)
  int* BlockNodesPerElement;              // NumberOfBlocks (G)
  int* BlockNumberOfAttributesPerElement; // NumberOfBlocks (G)
  int* BlockElementIdList;                // SumElementsPerBlock     (L)
  float* BlockAttributes;                 // SizeBlockAttributeArray (L)

  // Block information - values that we calculate

  int SumElementsPerBlock;
  int SizeBlockAttributeArray;

  int* BlockElementIdListIndex; // NumberOfBlocks
  int* BlockAttributesIndex;    // NumberOfBlocks

  vtkModelMetadataSTLCloak* BlockIdIndex; // computed map

  // Node Sets - arrays that are input to the class with Set*

  int NumberOfNodeSets; // (G)

  vtkSmartPointer<vtkStringArray> NodeSetNames;

  int* NodeSetIds;                         // NumberOfNodeSets (G)
  int* NodeSetSize;                        // NumberOfNodeSets (L)
  int* NodeSetNumberOfDistributionFactors; // NNS (L) (NSNDF[i] is 0 or NSS[i])
  int* NodeSetNodeIdList;                  // SumNodesPerNodeSet (L)
  float* NodeSetDistributionFactors;       // SumDistFactPerNodeSet (L)

  // Node Sets - values or arrays that the class computes

  int SumNodesPerNodeSet;
  int SumDistFactPerNodeSet;

  int* NodeSetNodeIdListIndex;         // NumberOfNodeSets
  int* NodeSetDistributionFactorIndex; // NumberOfNodeSets

  // Side Sets - input to class with Set*

  int NumberOfSideSets; // (G)

  vtkSmartPointer<vtkStringArray> SideSetNames;

  int* SideSetIds;                         // NumberOfSideSets (G)
  int* SideSetSize;                        // NumberOfSideSets (L)
  int* SideSetNumberOfDistributionFactors; // NSS (L) (SSNDF[i] = 0 or NumNodesInSide)
  int* SideSetElementList;                 // SumSidesPerSideSet (L)
  int* SideSetSideList;                    // SumSidesPerSideSet (L)
  int* SideSetNumDFPerSide;                // SumSidesPerSideSet (L)
  float* SideSetDistributionFactors;       // SumDistFactPerSideSet (L)

  // Side Sets - calculated by class

  int SumSidesPerSideSet;
  int SumDistFactPerSideSet;

  int* SideSetListIndex;               // NumberOfSideSets
  int* SideSetDistributionFactorIndex; // NumberOfSideSets

  // Other properties, provided as input with Set*

  int NumberOfBlockProperties; // (G)
  char** BlockPropertyNames;   // one per property (G)
  int* BlockPropertyValue;     // NumBlocks * NumBlockProperties (G)

  int NumberOfNodeSetProperties; // (G)
  char** NodeSetPropertyNames;   // one per property (G)
  int* NodeSetPropertyValue;     // NumNodeSets * NumNodeSetProperties (G)

  int NumberOfSideSetProperties; // (G)
  char** SideSetPropertyNames;   // one per property (G)
  int* SideSetPropertyValue;     // NumSideSets * NumSideSetProperties (G)

  // Global variables, 1 value per time step per variable.  We store
  // these as floats, even if they are doubles in the file.  The values
  // are global in the sense that they apply to the whole data set, but
  // the are local in the sense that they can change with each time step.
  // For the purpose of this object, which represents a particular
  // time step, they are therefore considered "local".  (Since they need
  // to be updated every time another read is done from the file.)

  int NumberOfGlobalVariables; // (G)
  char** GlobalVariableNames;  // (G) NumberOfGlobalVariables
  float* GlobalVariableValue;  // (G) NumberOfGlobalVariables

  // The element and node arrays in the file were all scalar arrays.
  // Those with similar names were combined into vectors in VTK.  Here
  // are all the original names from the Exodus file, the names given
  // the variables in the VTK ugrid, and a mapping from the VTK names
  // to the Exodus names.

  int OriginalNumberOfElementVariables;   // (G)
  char** OriginalElementVariableNames;    // (G) OriginalNumberOfElementVariables
  int NumberOfElementVariables;           // (G)
  int MaxNumberOfElementVariables;        // (G)
  char** ElementVariableNames;            // (G) MaxNumberOfElementVariables
  int* ElementVariableNumberOfComponents; // (G) MaxNumberOfElementVariables
  int* MapToOriginalElementVariableNames; // (G) MaxNumberOfElementVariables

  int OriginalNumberOfNodeVariables;   // (G)
  char** OriginalNodeVariableNames;    // (G) OriginalNumberOfNodeVariables
  int NumberOfNodeVariables;           // (G)
  int MaxNumberOfNodeVariables;        // (G)
  char** NodeVariableNames;            // (G) NumberOfNodeVariables
  int* NodeVariableNumberOfComponents; // (G) NumberOfNodeVariables
  int* MapToOriginalNodeVariableNames; // (G) NumberOfNodeVariables

  int* ElementVariableTruthTable; // (G) NumBlocks*OrigNumberOfElementVariables
  vtkTypeBool AllVariablesDefinedInAllBlocks;

  vtkModelMetadata(const vtkModelMetadata&) = delete;
  void operator=(const vtkModelMetadata&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
