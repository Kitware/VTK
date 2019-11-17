/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformHyperTreeGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUniformHyperTreeGridSource
 * @brief   Create a synthetic grid of uniform hypertrees.
 *
 * This class uses input parameters, most notably a string descriptor,
 * to generate a vtkHyperTreeGrid instance representing the corresponding
 * tree-based AMR grid with uniform root cell sizes along each axis.
 *
 * @sa
 * vtkHyperTreeGridSource vtkUniformHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, NexGen Analytics 2017
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkUniformHyperTreeGridSource_h
#define vtkUniformHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridSource.h"

class VTKFILTERSSOURCES_EXPORT vtkUniformHyperTreeGridSource : public vtkHyperTreeGridSource
{
public:
  vtkTypeMacro(vtkUniformHyperTreeGridSource, vtkHyperTreeGridSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkUniformHyperTreeGridSource* New();

protected:
  vtkUniformHyperTreeGridSource();
  ~vtkUniformHyperTreeGridSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkUniformHyperTreeGridSource(const vtkUniformHyperTreeGridSource&) = delete;
  void operator=(const vtkUniformHyperTreeGridSource&) = delete;
};

#endif
