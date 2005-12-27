/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObserverMediator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkObserverMediator.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkPriorityQueue.h"
#include "vtkInteractorObserver.h"
#include <vtkstd/map>

vtkCxxRevisionMacro(vtkObserverMediator, "1.1");
vtkStandardNewMacro(vtkObserverMediator);

// PIMPL the map containing the observer priorities. Note that only observers who are
// requesting non-default cursors are placed into the map.
// Comparison functor based on observer priorities.
struct vtkObserverCompare
{
  bool operator()(vtkInteractorObserver* w1, vtkInteractorObserver* w2) const
  {
    return (w1->GetPriority() < w2->GetPriority());
  }
};

// The important feature of the map is that it sorts data (based on the functor above).
class vtkObserverMap : public vtkstd::map<vtkInteractorObserver*,int,vtkObserverCompare>
{
public:
  vtkObserverMap() : vtkstd::map<vtkInteractorObserver*,int,vtkObserverCompare>() {}
};
typedef vtkObserverMap::iterator ObserverMapIterator;


//----------------------------------------------------------------------------------
vtkObserverMediator::vtkObserverMediator()
{
  this->Interactor = NULL;
  this->ObserverMap = new vtkObserverMap;
  
  this->CurrentObserver = NULL;
  this->CurrentCursorShape = VTK_CURSOR_DEFAULT;
}


//----------------------------------------------------------------------------------
vtkObserverMediator::~vtkObserverMediator()
{
  delete this->ObserverMap;
}

//----------------------------------------------------------------------------------
void vtkObserverMediator::SetInteractor(vtkRenderWindowInteractor* i)
{
  this->Interactor = i;
}

//----------------------------------------------------------------------------------
// This  mediation process works by keeping track of non-default cursor requests.
// Ties are broken based on widget priority (hence the priority queue).
int vtkObserverMediator::RequestCursorShape(vtkInteractorObserver *w, int requestedShape)
{
  if ( !this->Interactor || !w )
    {
    return 0;
    }

  // Cull out the trivial cases when setting to default cursor
  if ( requestedShape == VTK_CURSOR_DEFAULT )
    {
    if ( !this->CurrentObserver || this->CurrentCursorShape == VTK_CURSOR_DEFAULT )
      {
      return 0; //nothing has changed
      }
    if ( w == this->CurrentObserver && requestedShape != this->CurrentCursorShape )
      {
      this->CurrentCursorShape = VTK_CURSOR_DEFAULT;
      this->ObserverMap->erase(w);
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
      return 1;
      }
    return 0;
    }

  // For some strange reason this code causes problems. The SetCurrentCursor() method
  // must be called for every move or the cursor reverts back to default.
//   if ( w == this->CurrentObserver && requestedShape == this->CurrentCursorShape )
//     {
//     return 0;
//     }

  // Place request in map if non-default
  ObserverMapIterator iter = this->ObserverMap->find(w);
  if ( iter != this->ObserverMap->end() )
    {//found something, see if request has changed. If so, erase it and reinsert into map.
    if ( (*iter).second != requestedShape )
      {
      this->ObserverMap->erase(iter);
      }
    }
  (*this->ObserverMap)[w] = requestedShape;

  // Get the item with the highest priority off of the queue
  if ( ! this->ObserverMap->empty() )
    {
    ObserverMapIterator iter = this->ObserverMap->end();
    --iter;
    this->Interactor->GetRenderWindow()->SetCurrentCursor((*iter).second);
    this->CurrentObserver = w;
    this->CurrentCursorShape = (*iter).second;
    return 1;
    }
  
  return 0;
}

  
//----------------------------------------------------------------------------------
void vtkObserverMediator::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Render Window Interactor: ";
  if ( this->Interactor )
    {
    os << this->Interactor << "\n";
    }
  else
    {
    os << "(None)\n";
    }
    
}
