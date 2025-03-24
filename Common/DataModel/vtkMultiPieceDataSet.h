// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiPieceDataSet
 * @brief   composite dataset to encapsulates pieces of
 * dataset.
 *
 * A vtkMultiPieceDataSet dataset groups multiple data pieces together.
 * For example, say that a simulation broke a volume into 16 piece so that
 * each piece can be processed with 1 process in parallel. We want to load
 * this volume in a visualization cluster of 4 nodes. Each node will get 4
 * pieces, not necessarily forming a whole rectangular piece. In this case,
 * it is not possible to append the 4 pieces together into a vtkImageData.
 * In this case, these 4 pieces can be collected together using a
 * vtkMultiPieceDataSet.
 * Note that vtkMultiPieceDataSet is intended to be included in other composite
 * datasets eg. vtkMultiBlockDataSet. Hence the lack
 * of algorithms producing vtkMultiPieceDataSet.
 */

#ifndef vtkMultiPieceDataSet_h
#define vtkMultiPieceDataSet_h

#include "vtkPartitionedDataSet.h"

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkMultiPieceDataSet : public vtkPartitionedDataSet
{
public:
  static vtkMultiPieceDataSet* New();
  vtkTypeMacro(vtkMultiPieceDataSet, vtkPartitionedDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_MULTIPIECE_DATA_SET; }

  /**
   * Set the number of pieces. This will cause allocation if the new number of
   * pieces is greater than the current size. All new pieces are initialized to
   * null.
   */
  void SetNumberOfPieces(unsigned int numpieces) { this->SetNumberOfPartitions(numpieces); }

  /**
   * Returns the number of pieces.
   */
  unsigned int GetNumberOfPieces() { return this->GetNumberOfPartitions(); }

  ///@{
  /**
   * Returns the piece at the given index.
   */
  vtkDataSet* GetPiece(unsigned int pieceno) { return this->GetPartition(pieceno); }
  vtkDataObject* GetPieceAsDataObject(unsigned int pieceno)
  {
    return this->GetPartitionAsDataObject(pieceno);
  }
  ///@}

  /**
   * Sets the data object as the given piece. The total number of pieces will
   * be resized to fit the requested piece no.
   */
  void SetPiece(unsigned int pieceno, vtkDataObject* piece) { this->SetPartition(pieceno, piece); }

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkMultiPieceDataSet* GetData(vtkInformation* info);
  static vtkMultiPieceDataSet* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkMultiPieceDataSet();
  ~vtkMultiPieceDataSet() override;

private:
  vtkMultiPieceDataSet(const vtkMultiPieceDataSet&) = delete;
  void operator=(const vtkMultiPieceDataSet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
