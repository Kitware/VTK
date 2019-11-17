/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToHyperTreeGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  void PrintSelf(ostream&, vtkIndent) override;

  vtkSetMacro(DepthMax, int);
  vtkGetMacro(DepthMax, int);

  vtkSetMacro(NbColors, int);
  vtkGetMacro(NbColors, int);

protected:
  vtkImageDataToHyperTreeGrid();
  ~vtkImageDataToHyperTreeGrid() override;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

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

#endif // vtkImageDataToHyperTreeGrid_h
