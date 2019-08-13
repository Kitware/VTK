/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenerateGlobalIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkGenerateGlobalIds
 * @brief generates global point and cell ids.
 *
 * vtkGenerateGlobalIds generates global point and cell ids. This filter also
 * generated ghost-point information, flagging duplicate points appropriately.
 * vtkGenerateGlobalIds works across all blocks in the input datasets and across
 * all ranks.
 */

#ifndef vtkGenerateGlobalIds_h
#define vtkGenerateGlobalIds_h

#include "vtkFiltersParallelDIY2Module.h" // for export macros
#include "vtkPassInputTypeAlgorithm.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkGenerateGlobalIds : public vtkPassInputTypeAlgorithm
{
public:
  static vtkGenerateGlobalIds* New();
  vtkTypeMacro(vtkGenerateGlobalIds, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

protected:
  vtkGenerateGlobalIds();
  ~vtkGenerateGlobalIds() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGenerateGlobalIds(const vtkGenerateGlobalIds&) = delete;
  void operator=(const vtkGenerateGlobalIds&) = delete;

  vtkMultiProcessController* Controller;
};

#endif
