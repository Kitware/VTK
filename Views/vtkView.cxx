/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"
#include "vtkViewTheme.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
class vtkView::Command : public vtkCommand
{
public:
  static Command* New() {  return new Command(); }
  virtual void Execute(vtkObject *caller, unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->ProcessEvents(caller, eventId, callData);
      }
    }
  void SetTarget(vtkView* t)
    {
    this->Target = t;
    }
private:
  Command() { this->Target = 0; }
  vtkView* Target;
};

//----------------------------------------------------------------------------
class vtkView::vtkInternal
{
public:
  vtkstd::map<vtkObject*, vtkstd::string> RegisteredProgress;
};

//----------------------------------------------------------------------------
class vtkView::vtkImplementation
{
public:
  vtkstd::vector<vtkstd::vector<vtkSmartPointer<vtkDataRepresentation> > > Ports;
};
  

vtkCxxRevisionMacro(vtkView, "1.17");
vtkStandardNewMacro(vtkView);
vtkCxxSetObjectMacro(vtkView, SelectionArrayNames, vtkStringArray);
//----------------------------------------------------------------------------
vtkView::vtkView()
{
  this->Internal = new vtkView::vtkInternal();
  this->Implementation = new vtkView::vtkImplementation();
  this->Observer = vtkView::Command::New();
  this->Observer->SetTarget(this);
  this->SelectionArrayNames = vtkStringArray::New();
  this->SelectionType = vtkSelectionNode::INDICES;
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();  
}

//----------------------------------------------------------------------------
vtkView::~vtkView()
{
//  this->Representations->Delete();
  this->RemoveAllRepresentations();

  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->SetSelectionArrayNames(0);
  delete this->Internal;
  delete this->Implementation;
}

//----------------------------------------------------------------------------
bool vtkView::IsItemPresent(vtkDataRepresentation* rep)
{
  unsigned int i, j;
  for( i = 0; i < this->Implementation->Ports.size(); i++ )
    {
    for( j = 0; j < this->Implementation->Ports[i].size(); j++ )
      {
      if( this->Implementation->Ports[i][j] == rep )
        {
        return true;
        }
      }
    }
      
  return false;
}

//----------------------------------------------------------------------------
bool vtkView::IsItemPresent(int i, vtkDataRepresentation* rep)
{
  unsigned int j;
  if( !this->CheckPort(i, 0) )
    {
    return false;
    }
  else
    {
    for( j = 0; j < this->Implementation->Ports[i].size(); j++ )
      {
      if( this->Implementation->Ports[i][j] == rep )
        {
        return true;
        }
      }
    }
      
  return false;
}

//----------------------------------------------------------------------------
void vtkView::SizePort(int port, int index)
{
  if( this->Implementation->Ports.size() < static_cast<size_t>(port+1) )
    {
    this->Implementation->Ports.resize(port+1);
    }
  
  if( this->Implementation->Ports[port].size() < static_cast<size_t>(index+1) )
    {
    int old_size = static_cast<int>(this->Implementation->Ports[port].size());
    this->Implementation->Ports[port].resize(index+1);
    for( int k = old_size; k < index+1; k++ )
      {
      this->Implementation->Ports[port][k] = NULL;
      }
    }
}

//----------------------------------------------------------------------------
bool vtkView::CheckPort(int port, int index )
{
  if( this->Implementation->Ports.size() < static_cast<size_t>(port+1) )
    {
    return false;
    }
  
  if( this->Implementation->Ports[port].size() < static_cast<size_t>(index+1) )
    {
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInput(vtkDataObject* input)
{
  return this->AddRepresentationFromInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInput(int port, vtkDataObject* input)
{
  return this->AddRepresentationFromInputConnection(port, input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInput(vtkDataObject* input)
{
  return this->SetRepresentationFromInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInput(int port, vtkDataObject* input)
{
  return this->SetRepresentationFromInputConnection(port, input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInput(int port, int index, vtkDataObject* input)
{
  return this->SetRepresentationFromInputConnection(port, index, input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::CreateDefaultRepresentation(vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = vtkDataRepresentation::New();
  rep->SetInputConnection(conn);
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInputConnection(vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
    {
    vtkErrorMacro("Could not add representation from input connection because "
      "no default representation was created for the given input connection.");
    return 0;
    }

  this->AddRepresentation(rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInputConnection(int port, vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
    {
    vtkErrorMacro("Could not add representation from input connection because "
      "no default representation was created for the given input connection.");
    return 0;
    }

  this->AddRepresentation(port, rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInputConnection(vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
    {
    vtkErrorMacro("Could not add representation from input connection because "
      "no default representation was created for the given input connection.");
    return 0;
    }

  this->SetRepresentation(rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInputConnection(int port, vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
    {
    vtkErrorMacro("Could not set representation from input connection because "
      "no default representation was created for the given input connection.");
    return 0;
    }

  this->SetRepresentation(port, rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInputConnection(int port, int index, vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
    {
    vtkErrorMacro("Could not set representation from input connection because "
      "no default representation was created for the given input connection.");
    return 0;
    }

  this->SetRepresentation(port, index, rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
void vtkView::AddRepresentation(vtkDataRepresentation* rep)
{
  this->AddRepresentation( 0, rep );
}

//----------------------------------------------------------------------------
void vtkView::AddRepresentation(int port, vtkDataRepresentation* rep)
{
  if( !this->CheckPort( port, 0 ) )
    {
    rep->Update();
    this->SetRepresentation(port, 0, rep);
    }
  else
    {
    if( !this->IsItemPresent(port, rep) )
      {
      if( rep->AddToView( this ) )
        {
        rep->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
        rep->Update();
        if (rep->GetNumberOfInputPorts() > 0)
          {
          this->AddInputConnection(port, 0, rep->GetInputConnection(),
                                            rep->GetSelectionConnection());
          }
        this->AddRepresentationInternal(rep);

        int port_length=
          static_cast<int>(this->Implementation->Ports[port].size());
        this->SizePort( port, port_length );
        this->Implementation->Ports[port][port_length] = rep;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::SetRepresentation(vtkDataRepresentation* rep)
{
  this->RemoveAllRepresentations();
  this->AddRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkView::SetRepresentation(int port, vtkDataRepresentation* rep)
{ 
  this->RemoveAllRepresentations(port);
  this->AddRepresentation(port, rep);
}

//----------------------------------------------------------------------------
void vtkView::SetRepresentation(int port, int index, vtkDataRepresentation* rep)
{ 
  vtkDataRepresentation* old_rep = NULL;
  if( this->CheckPort( port, index ) )
    {
    old_rep = this->Implementation->Ports[port][index];
    }

  if( old_rep != rep )
    {
    if( rep->AddToView( this ) )
      {
      if( old_rep != NULL )
        {
        old_rep->RemoveFromView( this );
        old_rep->RemoveObserver(this->GetObserver());
        if (old_rep->GetNumberOfInputPorts() > 0)
          {
          this->RemoveInputConnection(port, index,
                                      old_rep->GetInputConnection(),
                                      old_rep->GetSelectionConnection());
          }
        this->RemoveRepresentationInternal(old_rep);
        }
      
      rep->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
      rep->Update();
      if (rep->GetNumberOfInputPorts() > 0)
        {
        this->AddInputConnection(port, index, rep->GetInputConnection(),
                                              rep->GetSelectionConnection());
        }
      this->AddRepresentationInternal(rep);
      this->SizePort(port, index);
      this->Implementation->Ports[port][index] = rep;
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveRepresentation(vtkDataRepresentation* rep)
{
  if (this->IsItemPresent(rep))
    {
    rep->RemoveFromView(this);
    rep->RemoveObserver(this->GetObserver());
    if (rep->GetNumberOfInputPorts() > 0)
      {
      this->RemoveInputConnection(0, 0, rep->GetInputConnection(),
                                        rep->GetSelectionConnection());
      }
    this->RemoveRepresentationInternal(rep);
    this->RemoveItem(rep);
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveItem(vtkDataRepresentation* rep)
{
  unsigned int i;
  for( i = 0; i < this->Implementation->Ports.size(); i++ )
    {
    vtkstd::vector<vtkSmartPointer<vtkDataRepresentation> >::iterator port_iter =
      this->Implementation->Ports[i].begin();
    while( port_iter != this->Implementation->Ports[i].end() )
      {
      if( *port_iter == rep )
        {
        this->Implementation->Ports[i].erase( port_iter );
        break;
        }
      ++port_iter;
      }
    }
}      
  
//----------------------------------------------------------------------------
void vtkView::RemoveRepresentation(vtkAlgorithmOutput* conn)
{
  unsigned int i, j;
  for( i = 0; i < this->Implementation->Ports.size(); i++ )
    {
    for( j = 0; j < this->Implementation->Ports[i].size(); j++ )
      {
      vtkDataRepresentation* rep = this->Implementation->Ports[i][j];
      
      if (rep->GetNumberOfInputPorts() > 0 &&
          rep->GetInputConnection() == conn)
        {
        this->RemoveRepresentation(rep);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveAllRepresentations()
{
  size_t nPorts = this->Implementation->Ports.size();
  for (unsigned int port = 0; port < nPorts; ++port)
    {
    this->RemoveAllRepresentations(port);
    }
    
  this->Implementation->Ports.clear();
}

//----------------------------------------------------------------------------
void vtkView::RemoveAllRepresentations(int port)
{
  if (!this->CheckPort(port, 0))
    {
    return;
    }
  
  while (this->Implementation->Ports[port].size())
    {
    vtkDataRepresentation* rep = this->Implementation->Ports[port].back();
    this->RemoveRepresentation(rep);
    }
}

//----------------------------------------------------------------------------
int vtkView::GetNumberOfRepresentations()
{
  int counter = 0;
  if( this->CheckPort(0,0) )
    {
    counter = static_cast<int>(this->Implementation->Ports[0].size());
    }
  return counter;
}

//----------------------------------------------------------------------------
int vtkView::GetNumberOfRepresentations(int port)
{
  if( this->Implementation->Ports.size() > static_cast<size_t>(port) )
    {
    return static_cast<int>(this->Implementation->Ports[port].size());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::GetRepresentation(int index)
{
  if( this->CheckPort( 0, index ) )
    {
    return this->Implementation->Ports[0][index];
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::GetRepresentation(int port, int index)
{
  if( this->CheckPort( port, index ) )
    {
    return this->Implementation->Ports[port][index];
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkView::SetSelectionArrayName(const char* name)
{
  if (!this->SelectionArrayNames)
    {
    this->SelectionArrayNames = vtkStringArray::New();
    }
  this->SelectionArrayNames->Initialize();
  this->SelectionArrayNames->InsertNextValue(name);
}

//----------------------------------------------------------------------------
const char* vtkView::GetSelectionArrayName()
{
  if (this->SelectionArrayNames &&
      this->SelectionArrayNames->GetNumberOfTuples() > 0)
    {
    return this->SelectionArrayNames->GetValue(0);
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkCommand* vtkView::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
void vtkView::ProcessEvents(vtkObject* caller, unsigned long eventId, 
  void* callData)
{
  vtkDataRepresentation* caller_rep = vtkDataRepresentation::SafeDownCast( caller );
  if (this->IsItemPresent(caller_rep) && eventId == vtkCommand::SelectionChangedEvent)
    {
    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    }

  if (eventId == vtkCommand::ProgressEvent)
    {
    vtkstd::map<vtkObject*, vtkstd::string>::iterator iter = 
      this->Internal->RegisteredProgress.find(caller);
    if (iter != this->Internal->RegisteredProgress.end())
      {
      ViewProgressEventCallData eventdata(iter->second.c_str(),
        *(reinterpret_cast<const double*>(callData)));
      this->InvokeEvent(vtkCommand::ViewProgressEvent, &eventdata);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RegisterProgress(vtkObject* algorithm, const char* message/*=NULL*/)
{
  if (algorithm)
    {
    const char* used_message = message? message : algorithm->GetClassName();
    this->Internal->RegisteredProgress[algorithm] = used_message;
    algorithm->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkView::UnRegisterProgress(vtkObject* algorithm)
{
  if (algorithm)
    {
    vtkstd::map<vtkObject*, vtkstd::string>::iterator iter = 
      this->Internal->RegisteredProgress.find(algorithm);
    if (iter != this->Internal->RegisteredProgress.end())
      {
      this->Internal->RegisteredProgress.erase(iter);
      algorithm->RemoveObservers(vtkCommand::ProgressEvent, this->Observer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::Update()
{
  unsigned int i, j;
  for( i = 0; i < this->Implementation->Ports.size(); i++ )
    {
    for( j = 0; j < this->Implementation->Ports[i].size(); j++ )
      {
      if( this->Implementation->Ports[i][j] )
        {
        this->Implementation->Ports[i][j]->Update();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SelectionType: " << this->SelectionType << endl;
  os << indent << "SelectionArrayNames: " << (this->SelectionArrayNames ? "" : "(null)") << endl;
  if (this->SelectionArrayNames)
    {
    this->SelectionArrayNames->PrintSelf(os, indent.GetNextIndent());
    }
}
