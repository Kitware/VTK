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

#include <sstream>
#include <cassert>
#include <locale>
#include <vtksys/SystemTools.hxx>
#include <string>

vtkStandardNewMacro(vtkInteractorEventRecorder);

float vtkInteractorEventRecorder::StreamVersion = 1.0f;

//----------------------------------------------------------------------------
vtkInteractorEventRecorder::vtkInteractorEventRecorder()
{
  //take over the processing of keypress events from the superclass
  this->KeyPressCallbackCommand->SetCallback(
    vtkInteractorEventRecorder::ProcessCharEvent);
  this->KeyPressCallbackCommand->SetPassiveObserver(1); // get events first
  //processes delete events
  this->DeleteEventCallbackCommand = vtkCallbackCommand::New();
  this->DeleteEventCallbackCommand->SetClientData(this);
  this->DeleteEventCallbackCommand->SetCallback(
    vtkInteractorEventRecorder::ProcessDeleteEvent);

  this->EventCallbackCommand->SetCallback(
    vtkInteractorEventRecorder::ProcessEvents);
  this->EventCallbackCommand->SetPassiveObserver(1); // get events first

  this->FileName = NULL;

  this->State = vtkInteractorEventRecorder::Start;
  this->InputStream = NULL;
  this->OutputStream = NULL;

  this->ReadFromInputString = 0;
  this->InputString = NULL;
}

//----------------------------------------------------------------------------
vtkInteractorEventRecorder::~vtkInteractorEventRecorder()
{
  this->SetInteractor(0);

  delete [] this->FileName;

  if ( this->InputStream )
  {
    this->InputStream->clear();
    delete this->InputStream;
    this->InputStream = NULL;
  }

  delete this->OutputStream;
  this->OutputStream = NULL;

  delete [] this->InputString;
  this->InputString = NULL;
  this->DeleteEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
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

    // Make sure that the interactor does not exit in response
    // to a StartEvent. The Interactor has code to allow others to handle
    // the event look of they want to
    i->HandleEventLoop = 1;

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
    this->Interactor->HandleEventLoop = 0;

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
  }
}

//----------------------------------------------------------------------------
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

      // Use C locale. We don't want the user-defined locale when we write
      // float values.
      (*this->OutputStream).imbue(std::locale::classic());

      *this->OutputStream << "# StreamVersion "
                          << vtkInteractorEventRecorder::StreamVersion << "\n";
    }

    vtkDebugMacro(<<"Recording");
    this->State = vtkInteractorEventRecorder::Recording;
  }
}

//----------------------------------------------------------------------------
void vtkInteractorEventRecorder::Play()
{
  if ( this->State == vtkInteractorEventRecorder::Start )
  {
    if ( this->ReadFromInputString )
    {
      vtkDebugMacro(<< "Reading from InputString");
      size_t len = 0;
      if ( this->InputString != NULL )
      {
        len = strlen(this->InputString);
      }
      if ( len == 0 )
      {
        vtkErrorMacro(<< "No input string specified");
        return;
      }
      std::string inputStr(this->InputString, len);
      delete this->InputStream;
      this->InputStream = new std::istringstream(inputStr);
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
    char event[128], keySym[64];
    int pos[2], ctrlKey, shiftKey, keyCode, repeatCount;
    float stream_version = 0.0f, tempf;
    std::string line;

    while ( vtksys::SystemTools::GetLineFromStream(*this->InputStream, line) )
    {
      std::istringstream iss(line);

      // Use classic locale, we don't want to parse float values with
      // user-defined locale.
      iss.imbue(std::locale::classic());

      iss.width(256);
      iss >> event;

      // Quick skip comment
      if (*event == '#')
      {
        // Parse the StreamVersion (not using >> since comment could be empty)
        // Expecting: # StreamVersion x.y

        if (strlen(line.c_str()) > 16 &&
          !strncmp(line.c_str(), "# StreamVersion ", 16))
        {
          int res = sscanf(line.c_str() + 16, "%f", &tempf);
          if (res && res != EOF)
          {
            stream_version = tempf;
          }
        }
      }
      else
      {
        unsigned long ievent = vtkCommand::GetEventIdFromString(event);
        if (ievent != vtkCommand::NoEvent)
        {
          if (stream_version >= 1.1)
          {
            // We could grab the time info here
          }
          iss >> pos[0];
          iss >> pos[1];
          iss >> ctrlKey;
          iss >> shiftKey;
          iss >> keyCode;
          iss >> repeatCount;
          iss >> keySym;

          this->Interactor->SetEventPosition(pos);
          this->Interactor->SetControlKey(ctrlKey);
          this->Interactor->SetShiftKey(shiftKey);
          this->Interactor->SetKeyCode(static_cast<char>(keyCode));
          this->Interactor->SetRepeatCount(repeatCount);
          this->Interactor->SetKeySym(keySym);

          this->Interactor->InvokeEvent(ievent, NULL);
        }
      }
    }
  }

  this->State = vtkInteractorEventRecorder::Start;
}

//----------------------------------------------------------------------------
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
   return;
 }
 this->InputStream->clear();
 this->InputStream->seekg(0);
}

//----------------------------------------------------------------------------
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
    this->Interactor->RemoveObserver(this->DeleteEventCallbackCommand);
  }

  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
  {
    i->AddObserver(vtkCommand::CharEvent,
                   this->KeyPressCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::DeleteEvent,
                   this->DeleteEventCallbackCommand, this->Priority);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkInteractorEventRecorder::ProcessDeleteEvent(vtkObject* vtkNotUsed(object),
                                                    unsigned long event,
                                                    void* clientData,
                                                    void* vtkNotUsed(callData))
{
  assert (event == vtkCommand::DeleteEvent);
  (void)event;
  vtkInteractorEventRecorder* self =
    reinterpret_cast<vtkInteractorEventRecorder *>( clientData );
  // if the interactor is being deleted then remove the event handlers
  self->SetInteractor(0);
}

//----------------------------------------------------------------------------
void vtkInteractorEventRecorder::ProcessCharEvent(vtkObject* object,
                                                  unsigned long event,
                                                  void* clientData,
                                                  void* vtkNotUsed(callData))
{
  assert (event == vtkCommand::CharEvent);
  (void)event;
  vtkInteractorEventRecorder* self =
    reinterpret_cast<vtkInteractorEventRecorder *>( clientData );
  vtkRenderWindowInteractor* rwi =
    static_cast<vtkRenderWindowInteractor *>( object );
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

//----------------------------------------------------------------------------
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
      case vtkCommand::ModifiedEvent: //don't want these
        break;

      default:
        // A 'e' or a 'q' will stop the recording
        if (rwi->GetKeySym() &&
            (rwi->GetKeySym() == std::string("e") ||
             rwi->GetKeySym() == std::string("q")))
        {
          self->Off();
        }
        else
        {
          self->WriteEvent(vtkCommand::GetStringFromEventId(event),
                           rwi->GetEventPosition(), rwi->GetControlKey(),
                           rwi->GetShiftKey(), rwi->GetKeyCode(),
                           rwi->GetRepeatCount(), rwi->GetKeySym());
        }
    }
    self->OutputStream->flush();
  }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkInteractorEventRecorder::ReadEvent()
{
}

//----------------------------------------------------------------------------
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






