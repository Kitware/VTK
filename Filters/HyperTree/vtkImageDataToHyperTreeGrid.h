// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageDataToHyperTreeGrid
 * @brief
 *
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkImageDataToHyperTreeGrid_h
#define vtkImageDataToHyperTreeGrid_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkIntArray;
class vtkUnsignedCharArray;
class vtkDoubleArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkImageDataToHyperTreeGrid : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkImageDataToHyperTreeGrid* New();
  vtkTypeMacro(vtkImageDataToHyperTreeGrid, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(DepthMax, int);
  vtkGetMacro(DepthMax, int);

  vtkSetMacro(NbColors, int);
  vtkGetMacro(NbColors, int);

protected:
  vtkImageDataToHyperTreeGrid();
  ~vtkImageDataToHyperTreeGrid() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  void ProcessPixels(vtkIntArray*, vtkHyperTreeGridNonOrientedCursor*);

  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkImageDataToHyperTreeGrid(const vtkImageDataToHyperTreeGrid&) = delete;
  void operator=(const vtkImageDataToHyperTreeGrid&) = delete;

  int DepthMax;
  int NbColors;

  vtkDataArray* InScalars;

  vtkUnsignedCharArray* Color;
  vtkDoubleArray* Depth;
  vtkBitArray* Mask;
  int GlobalId;
};

VTK_ABI_NAMESPACE_END
#endif // vtkImageDataToHyperTreeGrid_h
