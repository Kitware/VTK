/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3Writer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPXdmf3Writer
 * @brief   mpi parallel writer for XDMF/HDF5 files
 *
 * vtkPXdmf3Writer converts vtkDataObjects to XDMF format and and when
 * run in parallel under MPI each rank writes only the data it is
 * responsible for.
 *
 * In the absence of the information provided by vtkModelMetadata,
 * if this writer is not part of a parallel application, we will use
 * reasonable defaults for all the values in the output XDMF file.
 * If you don't provide a block ID element array, we'll create a
 * block for each cell type that appears in the unstructured grid.
 *
 * However if this writer is part of a parallel application (hence
 * writing out a distributed XDMF file), then we need at the very
 * least a list of all the block IDs that appear in the file.  And
 * we need the element array of block IDs for the input unstructured grid.
 *
 * In the absence of a vtkModelMetadata object, you can also provide
 * time step information which we will include in the output XDMF
 * file.
 */

#ifndef vtkPXdmf3Writer_h
#define vtkPXdmf3Writer_h

#include "vtkIOParallelXdmf3Module.h" // For export macro
#include "vtkXdmf3Writer.h"

class VTKIOPARALLELXDMF3_EXPORT vtkPXdmf3Writer : public vtkXdmf3Writer
{
public:
  static vtkPXdmf3Writer* New();
  vtkTypeMacro(vtkPXdmf3Writer, vtkXdmf3Writer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPXdmf3Writer();
  ~vtkPXdmf3Writer() override;
  int CheckParameters() override;

  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int GlobalContinueExecuting(int localContinue) override;

private:
  vtkPXdmf3Writer(const vtkPXdmf3Writer&) = delete;
  void operator=(const vtkPXdmf3Writer&) = delete;
};

#endif /* vtkPXdmf3Writer_h */
