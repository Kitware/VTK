// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtentTranslator
 * @brief   Generates a structured extent from unstructured.
 *
 *
 * vtkExtentTranslator generates a structured extent from an unstructured
 * extent.  It uses a recursive scheme that splits the largest axis.  A hard
 * coded extent can be used for a starting point.
 */

#ifndef vtkExtentTranslator_h
#define vtkExtentTranslator_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationIntegerRequestKey;
class vtkInformationIntegerKey;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExtentTranslator : public vtkObject
{
public:
  static vtkExtentTranslator* New();

  vtkTypeMacro(vtkExtentTranslator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the Piece/NumPieces. Set the WholeExtent and then call PieceToExtent.
   * The result can be obtained from the Extent ivar.
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  vtkSetVector6Macro(Extent, int);
  vtkGetVector6Macro(Extent, int);
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  ///@{
  /**
   * These are the main methods that should be called. These methods
   * are responsible for converting a piece to an extent. The signatures
   * without arguments are only thread safe when each thread accesses a
   * different instance. The signatures with arguments are fully thread
   * safe.
   */
  virtual int PieceToExtent();
  virtual int PieceToExtentByPoints();
  virtual int PieceToExtentThreadSafe(int piece, int numPieces, int ghostLevel, int* wholeExtent,
    int* resultExtent, int splitMode, int byPoints);
  ///@}

  /**
   * How should the streamer break up extents. Block mode
   * tries to break an extent up into cube blocks.  It always chooses
   * the largest axis to split.
   * Slab mode first breaks up the Z axis.  If it gets to one slice,
   * then it starts breaking up other axes.
   */
  void SetSplitModeToBlock() { this->SplitMode = vtkExtentTranslator::BLOCK_MODE; }
  void SetSplitModeToXSlab() { this->SplitMode = vtkExtentTranslator::X_SLAB_MODE; }
  void SetSplitModeToYSlab() { this->SplitMode = vtkExtentTranslator::Y_SLAB_MODE; }
  void SetSplitModeToZSlab() { this->SplitMode = vtkExtentTranslator::Z_SLAB_MODE; }
  vtkGetMacro(SplitMode, int);

  /**
   * By default the translator creates N structured subextents by repeatedly
   * splitting the largest current dimension until there are N pieces.
   * If you do not want it always split the largest dimension, for instance when the
   * shortest dimension is the slowest changing and thus least coherent in memory,
   * use this to tell the translator which dimensions to split.
   */
  void SetSplitPath(int len, int* splitpath);

  // Don't change the numbers here - they are used in the code
  // to indicate array indices.
  enum Modes
  {
    X_SLAB_MODE = 0,
    Y_SLAB_MODE = 1,
    Z_SLAB_MODE = 2,
    BLOCK_MODE = 3
  };

  /**
   * Key used to request a particular split mode.
   * This is used by vtkStreamingDemandDrivenPipeline.
   */
  static vtkInformationIntegerRequestKey* UPDATE_SPLIT_MODE();

protected:
  vtkExtentTranslator();
  ~vtkExtentTranslator() override;

  static vtkInformationIntegerKey* DATA_SPLIT_MODE();

  friend class vtkInformationSplitModeRequestKey;

  ///@{
  /**
   * Returns 0 if no data exist for a piece.
   * The whole extent Should be passed in as the extent.
   * It is modified to return the result.
   */
  int SplitExtent(int piece, int numPieces, int* extent, int splitMode);
  int SplitExtentByPoints(int piece, int numPieces, int* extent, int splitMode);
  ///@}

  int Piece;
  int NumberOfPieces;
  int GhostLevel;
  int Extent[6];
  int WholeExtent[6];
  int SplitMode;

  int* SplitPath;
  int SplitLen;

private:
  vtkExtentTranslator(const vtkExtentTranslator&) = delete;
  void operator=(const vtkExtentTranslator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
