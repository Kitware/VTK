/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSocketCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSocketCollection -  a collection for sockets.
// .SECTION Description
// Apart from being vtkCollection subclass for sockets, this class
// provides means to wait for activity on all the sockets in the
// collection simultaneously.

#ifndef __vtkSocketCollection_h
#define __vtkSocketCollection_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkCollection.h"

class vtkSocket;
class VTKCOMMONSYSTEM_EXPORT vtkSocketCollection : public vtkCollection
{
public:
  static vtkSocketCollection* New();
  vtkTypeMacro(vtkSocketCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Add Socket to the collection.
  void AddItem(vtkSocket* soc);

  // Description:
  // Select all Connected sockets in the collection. If msec is specified,
  // it timesout after msec milliseconds on inactivity.
  // Returns 0 on timeout, -1 on error; 1 is a socket was selected.
  // The selected socket can be retrieved by GetLastSelectedSocket().
  int SelectSockets(unsigned long msec =0);

  // Description:
  // Returns the socket selected during the last SelectSockets(), if any.
  // NULL otherwise.
  vtkSocket* GetLastSelectedSocket()
    {return this->SelectedSocket; }

  // Description:
  // Overridden to unset SelectedSocket.
  void ReplaceItem(int i, vtkObject *);
  void RemoveItem(int i);
  void RemoveItem(vtkObject *);
  void RemoveAllItems();
protected:
  vtkSocketCollection();
  ~vtkSocketCollection();

  vtkSocket* SelectedSocket;
private:
  // Hide the standard AddItem.
  void AddItem(vtkObject* o) { this->Superclass::AddItem(o); }

private:
  vtkSocketCollection(const vtkSocketCollection&); // Not implemented.
  void operator=(const vtkSocketCollection&); // Not implemented.
};

#endif

