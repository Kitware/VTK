/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiPieceDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * datasets eg. vtkMultiBlockDataSet, vtkHierarchicalBoxDataSet. Hence the lack
 * of algorithms producting vtkMultiPieceDataSet.
*/

#ifndef vtkMultiPieceDataSet_h
#define vtkMultiPieceDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObjectTree.h"

class vtkDataSet;
class VTKCOMMONDATAMODEL_EXPORT vtkMultiPieceDataSet : public vtkDataObjectTree
{
public:
  static vtkMultiPieceDataSet* New();
  vtkTypeMacro(vtkMultiPieceDataSet, vtkDataObjectTree);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_MULTIPIECE_DATA_SET;}

  /**
   * Set the number of pieces. This will cause allocation if the new number of
   * pieces is greater than the current size. All new pieces are initialized to
   * null.
   */
  void SetNumberOfPieces(unsigned int numpieces);

  /**
   * Returns the number of pieces.
   */
  unsigned int GetNumberOfPieces();

  //@{
  /**
   * Returns the piece at the given index.
   */
  vtkDataSet* GetPiece(unsigned int pieceno);
  vtkDataObject* GetPieceAsDataObject(unsigned int pieceno);
  //@}

  /**
   * Sets the data object as the given piece. The total number of pieces will
   * be resized to fit the requested piece no.
   */
  void SetPiece(unsigned int pieceno, vtkDataObject* piece);


/**
 * Returns true if meta-data is available for a given piece.
 */
  int HasMetaData(unsigned int piece)
    { return this->Superclass::HasChildMetaData(piece); }

  /**
   * Returns the meta-data for the piece. If none is already present, a new
   * vtkInformation object will be allocated. Use HasMetaData to avoid
   * allocating vtkInformation objects.
   */
  vtkInformation* GetMetaData(unsigned int pieceno)
    { return this->Superclass::GetChildMetaData(pieceno); }

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkMultiPieceDataSet* GetData(vtkInformation* info);
  static vtkMultiPieceDataSet* GetData(vtkInformationVector* v, int i=0);
  //@}

  /**
   * Unhiding superclass method.
   */
  vtkInformation* GetMetaData(vtkCompositeDataIterator* iter) VTK_OVERRIDE
    { return this->Superclass::GetMetaData(iter); }

  /**
   * Unhiding superclass method.
   */
  int HasMetaData(vtkCompositeDataIterator* iter) VTK_OVERRIDE
    { return this->Superclass::HasMetaData(iter); }

protected:
  vtkMultiPieceDataSet();
  ~vtkMultiPieceDataSet() VTK_OVERRIDE;

private:
  vtkMultiPieceDataSet(const vtkMultiPieceDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMultiPieceDataSet&) VTK_DELETE_FUNCTION;

};

#endif


