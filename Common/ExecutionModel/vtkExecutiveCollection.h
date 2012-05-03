/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutiveCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2008, 2009 by SCI Institute, University of Utah.

  This is part of the Parallel Dataflow System originally developed by
  Huy T. Vo and Claudio T. Silva. For more information, see:

  "Parallel Dataflow Scheme for Streaming (Un)Structured Data" by Huy
  T. Vo, Daniel K. Osmari, Brian Summa, Joao L.D. Comba, Valerio
  Pascucci and Claudio T. Silva, SCI Institute, University of Utah,
  Technical Report #UUSCI-2009-004, 2009.

  "Multi-Threaded Streaming Pipeline For VTK" by Huy T. Vo and Claudio
  T. Silva, SCI Institute, University of Utah, Technical Report
  #UUSCI-2009-005, 2009.
-------------------------------------------------------------------------*/
// .NAME vtkExecutiveCollection - maintain a list of executive objects
// .SECTION Description
// vtkExecutiveCollection is an object that creates and manipulates lists of
// objects that are (inherited from) vtkExecutives.

// .SECTION See Also
// vtkExecutive vtkCollection

#ifndef __vtkExecutiveCollection_h
#define __vtkExecutiveCollection_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkExecutive.h" // Needed for static cast

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExecutiveCollection : public vtkCollection
{
public:
  static vtkExecutiveCollection *New();
  vtkTypeMacro(vtkExecutiveCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an executive to the list.
  void AddItem(vtkExecutive *exec)
  {
    this->vtkCollection::AddItem(exec);
  }

  // Description:
  // Get the next executive in the list.
  vtkExecutive *GetNextItem()
  {
    return static_cast<vtkExecutive *>(this->GetNextItemAsObject());
  }

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkExecutive *GetNextExecutive(vtkCollectionSimpleIterator &cookie)
  {
    return static_cast<vtkExecutive *>(this->GetNextItemAsObject(cookie));
  }
  //ETX

protected:
  vtkExecutiveCollection() {}
  ~vtkExecutiveCollection() {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
  {
    this->vtkCollection::AddItem(o);
  }

private:
  vtkExecutiveCollection(const vtkExecutiveCollection&);  // Not implemented.
  void operator=(const vtkExecutiveCollection&);  // Not implemented.
};


#endif
