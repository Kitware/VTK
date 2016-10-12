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
/**
 * @class   vtkSocketCollection
 * @brief    a collection for sockets.
 *
 * Apart from being vtkCollection subclass for sockets, this class
 * provides means to wait for activity on all the sockets in the
 * collection simultaneously.
*/

#ifndef vtkSocketCollection_h
#define vtkSocketCollection_h

#include "vtkCommonSystemModule.h" // For export macro
#include "vtkCollection.h"

class vtkSocket;
class VTKCOMMONSYSTEM_EXPORT vtkSocketCollection : public vtkCollection
{
public:
  static vtkSocketCollection* New();
  vtkTypeMacro(vtkSocketCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Add Socket to the collection.
  void AddItem(vtkSocket* soc);

  /**
   * Select all Connected sockets in the collection. If msec is specified,
   * it timesout after msec milliseconds on inactivity.
   * Returns 0 on timeout, -1 on error; 1 is a socket was selected.
   * The selected socket can be retrieved by GetLastSelectedSocket().
   */
  int SelectSockets(unsigned long msec =0);

  /**
   * Returns the socket selected during the last SelectSockets(), if any.
   * NULL otherwise.
   */
  vtkSocket* GetLastSelectedSocket()
    {return this->SelectedSocket; }

  //@{
  /**
   * Overridden to unset SelectedSocket.
   */
  void ReplaceItem(int i, vtkObject *);
  void RemoveItem(int i);
  void RemoveItem(vtkObject *);
  void RemoveAllItems();
  //@}

protected:
  vtkSocketCollection();
  ~vtkSocketCollection() VTK_OVERRIDE;

  vtkSocket* SelectedSocket;
private:
  // Hide the standard AddItem.
  void AddItem(vtkObject* o) { this->Superclass::AddItem(o); }

private:
  vtkSocketCollection(const vtkSocketCollection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSocketCollection&) VTK_DELETE_FUNCTION;
};

#endif

