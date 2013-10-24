/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPExodusIIElementBlockCellIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPExodusIIElementBlockCellIterator - vtkCellIterator subclass
// specialized for vtkCPExodusIIElementBlock.

#ifndef __vtkCPExodusIIElementBlockCellIterator_h
#define __vtkCPExodusIIElementBlockCellIterator_h

#include "vtkCellIterator.h"
#include "vtkIOExodusModule.h" // For export macro

#include "vtkSmartPointer.h" // For smart pointer

class vtkCPExodusIIElementBlock;
class vtkCPExodusIIElementBlockPrivate;

class VTKIOEXODUS_EXPORT vtkCPExodusIIElementBlockCellIterator
    : public vtkCellIterator
{
public:
  typedef vtkCPExodusIIElementBlockPrivate StorageType;

  static vtkCPExodusIIElementBlockCellIterator *New();
  vtkTypeMacro(vtkCPExodusIIElementBlockCellIterator, vtkCellIterator)
  void PrintSelf(ostream &os, vtkIndent indent);

  bool IsValid();
  vtkIdType GetCellId();

protected:
  vtkCPExodusIIElementBlockCellIterator();
  ~vtkCPExodusIIElementBlockCellIterator();

  void ResetToFirstCell();
  void IncrementToNextCell();
  void FetchCellType();
  void FetchPointIds();
  void FetchPoints();

  friend class ::vtkCPExodusIIElementBlock;
  void SetStorage(vtkCPExodusIIElementBlock *eb);

private:
  vtkCPExodusIIElementBlockCellIterator(const vtkCPExodusIIElementBlockCellIterator &); // Not implemented.
  void operator=(const vtkCPExodusIIElementBlockCellIterator &);   // Not implemented.

  vtkSmartPointer<StorageType> Storage;
  vtkSmartPointer<vtkPoints> DataSetPoints;
  vtkIdType CellId;
};

#endif //__vtkCPExodusIIElementBlockCellIterator_h
