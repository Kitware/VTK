/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorEventRecorder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorEventRecorder.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

vtkCxxRevisionMacro(vtkInteractorEventRecorder, "1.9");
vtkStandardNewMacro(vtkInteractorEventRecorder);

float vtkInteractorEventRecorder::StreamVersion = 1.0;

vtkInteractorEventRecorder::vtkInteractorEventRecorder()
{
  //take over the processing of delete and keypress events from the superclass
  this->KeyPressCallbackCommand->SetCallback(
    vtkInteractorEventRecorder::ProcessCharEvent);

  this->Priority = VTK_LARGE_FLOAT; //get events first
  this->EventCallbackCommand->SetCallback(
    vtkInteractorEventRecorder::ProcessEvents);

  this->FileName = NULL;

  this->State = vtkInteractorEventRecorder::Start;
  this->InputStream = NULL;
  this->OutputStream = NULL;

  this->ReadFromInputString = 0;
  this->InputString = NULL;
}

vtkInteractorEventRecorder::~vtkInteractorEventRecorder()
{
  this->SetInteractor(0);
  
  if ( this->FileName )
    {
    delete [] this->FileName;
    }

  if ( this->InputStream )
    {
    delete this->InputStream;
    this->InputStream = NULL;
    }

  if ( this->OutputStream )
    {
    delete this->OutputStream;
    this->OutputStream = NULL;
    }
  
  if ( this->InputString )
    {
    delete [] this->InputString;
    this->InputString = NULL;
    }
}

void vtkInteractorEventRecorder::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //----------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    this->Enabled = 1;

    // listen to any event
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::AnyEvent, this->EventCallbackCommand, 
                   this->Priority);

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }

  else //disabling-----------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }

    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }
}

void vtkInteractorEventRecorder::Record()
{
  if ( this->State == vtkInteractorEventRecorder::Start )
    {
    if ( ! this->OutputStream ) //need to open file
      {
      this->OutputStream = new ofstream(this->FileName, ios::out);
      if (this->OutputStream->fail())
        {
        vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
        delete this->OutputStream;
        return;
        }
      *this->OutputStream << "# StreamVersion " 
                          << vtkInteractorEventRecorder::StreamVersion << "\n";
      }
    
    vtkDebugMacro(<<"Recording");
    this->State = vtkInteractorEventRecorder::Recording;
    }
}

void vtkInteractorEventRecorder::Play()
{
  if ( this->State == vtkInteractorEventRecorder::Start )
    {
    if ( this->ReadFromInputString )
      {
      vtkDebugMacro(<< "Reading from InputString");
      int len;
      if ( this->InputString == NULL || (len = strlen(this->InputString) <= 0) )
        {
        vtkErrorMacro(<< "No input string specified");
        return;
        }
      this->InputStream = new istrstream(this->InputString, len);
      if (this->InputStream->fail())
        {
        vtkErrorMacro(<< "Unable to read from string");
        delete this->InputStream;
        return;
        }
      }
    else
      {
      if ( ! this->InputStream ) //need to open file
        {
        this->InputStream = new ifstream(this->FileName, ios::in);
        if (this->InputStream->fail())
          {
          vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
          delete this->InputStream;
          return;
          }
        }
      }

    vtkDebugMacro(<<"Playing");
    this->State = vtkInteractorEventRecorder::Playing;

    // Read events and invoke them on the object in question
    char event[128], keySym[64], buffer[512];
    int pos[2], ctrlKey, shiftKey, keyCode, repeatCount;
    float stream_version = 0.0, tempf;

    while ( ! this->InputStream->eof() )
      {
      this->InputStream->width(256);
      *this->InputStream >> event;

      // Quick skip comment
      if (*event == '#')
        {
        this->InputStream->getline(buffer, 512);

        // Parse the StreamVersion (not using >> since comment could be empty)
        // Expecting: # StreamVersion x.y

        if (strlen(buffer) > 15 && !strncmp(buffer, " StreamVersion ", 15))
          {
          int res = sscanf(buffer + 15, "%f", &tempf);
          if (res && res != EOF)
            {
            stream_version = tempf;
            }
          }
        }
      else
        {
        unsigned long ievent = vtkCommand::GetEventIdFromString(event);
        if (ievent == vtkCommand::NoEvent)
          {
          this->InputStream->ignore(512, '\n');
          }
        else
          {
          if (stream_version >= 1.1)
            {
            // We could grab the time info here
            }
          *this->InputStream >> pos[0];
          *this->InputStream >> pos[1];
          *this->InputStream >> ctrlKey;
          *this->InputStream >> shiftKey;
          *this->InputStream >> keyCode;
          *this->InputStream >> repeatCount;
          *this->InputStream >> keySym;
          
          this->Interactor->SetEventPosition(pos);
          this->Interactor->SetControlKey(ctrlKey);
          this->Interactor->SetShiftKey(shiftKey);
          this->Interactor->SetKeyCode(keyCode);
          this->Interactor->SetRepeatCount(repeatCount);
          this->Interactor->SetKeySym(keySym);
          
          this->Interactor->InvokeEvent(ievent, NULL);
          }
        }
      }
    }

  this->State = vtkInteractorEventRecorder::Start;
}

void vtkInteractorEventRecorder::Stop()
{
  this->State = vtkInteractorEventRecorder::Start;
  this->Modified();
}

void vtkInteractorEventRecorder::Rewind()
{
 if ( ! this->InputStream ) //need to already have an open file
   {
   vtkGenericWarningMacro(<<"No input file opened to rewind...");
   }
 this->InputStream->clear();
 this->InputStream->seekg(0);
}

// This adds the keypress event observer and the delete event observer
void vtkInteractorEventRecorder::SetInteractor(vtkRenderWindowInteractor* i)
{
  if (i == this->Interactor)
    {
    return;
    }

  // if we already have an Interactor then stop observing it
  if (this->Interactor)
    {
    this->SetEnabled(0); //disable the old interactor
    this->Interactor->RemoveObserver(this->KeyPressCallbackCommand);
    }

  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
    {
    i->AddObserver(vtkCommand::CharEvent, 
                   this->KeyPressCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::DeleteEvent, 
                   this->KeyPressCallbackCommand, this->Priority);
    }
  
  this->Modified();
}

void vtkInteractorEventRecorder::ProcessCharEvent(vtkObject* object, 
                                                  unsigned long event,
                                                  void* clientData, 
                                                  void* vtkNotUsed(callData))
{
  vtkInteractorEventRecorder* self = 
    reinterpret_cast<vtkInteractorEventRecorder *>( clientData );
  vtkRenderWindowInteractor* rwi = 
    static_cast<vtkRenderWindowInteractor *>( object );

  switch(event)
    {
    case vtkCommand::DeleteEvent:
      // if the interactor is being deleted then remove the event handlers
      self->SetInteractor(0);
      break;

    case vtkCommand::CharEvent:
      if ( self->KeyPressActivation )
        {
        if (rwi->GetKeyCode() == self->KeyPressActivationValue )
          {
          if ( !self->Enabled )
            {
            self->On();
            }
          else
            {
            self->Off();
            }
          }//event not aborted
        }//if activation enabled
    }
}

void vtkInteractorEventRecorder::ProcessEvents(vtkObject* object, 
                                               unsigned long event,
                                               void* clientData, 
                                               void* vtkNotUsed(callData))
{
  vtkInteractorEventRecorder* self = 
    reinterpret_cast<vtkInteractorEventRecorder *>( clientData );
  vtkRenderWindowInteractor* rwi = 
    static_cast<vtkRenderWindowInteractor *>( object );

  // all events are processed
  if ( self->State == vtkInteractorEventRecorder::Recording )
    {
    switch(event)
      {
      case vtkCommand::ModifiedEvent: //dont want these
        break;

      default:
        self->WriteEvent(vtkCommand::GetStringFromEventId(event),
                         rwi->GetEventPosition(), rwi->GetControlKey(), 
                         rwi->GetShiftKey(), rwi->GetKeyCode(),
                         rwi->GetRepeatCount(), rwi->GetKeySym());
      }
    self->OutputStream->flush();
    }
}

void vtkInteractorEventRecorder::WriteEvent(const char* event, int pos[2], 
                                            int ctrlKey, int shiftKey, 
                                            int keyCode, int repeatCount,
                                            char* keySym)
{
  *this->OutputStream << event << " " << pos[0] << " " << pos[1] << " "
                      << ctrlKey << " " << shiftKey << " "
                      << keyCode << " " << repeatCount << " ";
  if ( keySym )
    {
    *this->OutputStream << keySym << "\n";
    }
  else
    {
    *this->OutputStream << "0\n";
    }
}
  
void vtkInteractorEventRecorder::ReadEvent()
{
}

void vtkInteractorEventRecorder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->FileName)
    {
    os << indent << "File Name: " << this->FileName << "\n";
    }

  os << indent << "ReadFromInputString: " 
     << (this->ReadFromInputString ? "On\n" : "Off\n");

  if ( this->InputString )
    {
    os << indent << "Input String: " << this->InputString << "\n";
    }
  else
    {
    os << indent << "Input String: (None)\n";
    }
}


  



