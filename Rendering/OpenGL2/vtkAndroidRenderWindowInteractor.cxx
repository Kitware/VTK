/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAndroidRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vtkAndroidRenderWindowInteractor.h"
#include "vtkEGLRenderWindow.h"

#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "VTK", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "VTK", __VA_ARGS__))

vtkStandardNewMacro(vtkAndroidRenderWindowInteractor);

void (*vtkAndroidRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkAndroidRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkAndroidRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkAndroidRenderWindowInteractor::vtkAndroidRenderWindowInteractor()
{
  this->MouseInWindow = 0;
  this->StartedMessageLoop = 0;

  this->KeyCodeToKeySymTable = (const char **)malloc(sizeof(const char *)*AKEYCODE_3D_MODE);
  this->KeyCodeToKeySymTable[AKEYCODE_UNKNOWN] = "None";
  this->KeyCodeToKeySymTable[AKEYCODE_SOFT_LEFT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_SOFT_RIGHT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_HOME] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BACK] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_CALL] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_ENDCALL] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_0] = "0";
  this->KeyCodeToKeySymTable[AKEYCODE_1] = "1";
  this->KeyCodeToKeySymTable[AKEYCODE_2] = "2";
  this->KeyCodeToKeySymTable[AKEYCODE_3] = "3";
  this->KeyCodeToKeySymTable[AKEYCODE_4] = "4";
  this->KeyCodeToKeySymTable[AKEYCODE_5] = "5";
  this->KeyCodeToKeySymTable[AKEYCODE_6] = "6";
  this->KeyCodeToKeySymTable[AKEYCODE_7] = "7";
  this->KeyCodeToKeySymTable[AKEYCODE_8] = "8";
  this->KeyCodeToKeySymTable[AKEYCODE_9] = "9";
  this->KeyCodeToKeySymTable[AKEYCODE_STAR] = "asterisk";
  this->KeyCodeToKeySymTable[AKEYCODE_POUND] = "numbersign";
  this->KeyCodeToKeySymTable[AKEYCODE_DPAD_UP] = "Up";
  this->KeyCodeToKeySymTable[AKEYCODE_DPAD_DOWN] = "Down";
  this->KeyCodeToKeySymTable[AKEYCODE_DPAD_LEFT] = "Left";
  this->KeyCodeToKeySymTable[AKEYCODE_DPAD_RIGHT] = "Right";
  this->KeyCodeToKeySymTable[AKEYCODE_DPAD_CENTER] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_VOLUME_UP] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_VOLUME_DOWN] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_POWER] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_CAMERA] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_CLEAR] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_A] = "a";
  this->KeyCodeToKeySymTable[AKEYCODE_B] = "b";
  this->KeyCodeToKeySymTable[AKEYCODE_C] = "c";
  this->KeyCodeToKeySymTable[AKEYCODE_D] = "d";
  this->KeyCodeToKeySymTable[AKEYCODE_E] = "e";
  this->KeyCodeToKeySymTable[AKEYCODE_F] = "f";
  this->KeyCodeToKeySymTable[AKEYCODE_G] = "g";
  this->KeyCodeToKeySymTable[AKEYCODE_H] = "h";
  this->KeyCodeToKeySymTable[AKEYCODE_I] = "i";
  this->KeyCodeToKeySymTable[AKEYCODE_J] = "j";
  this->KeyCodeToKeySymTable[AKEYCODE_K] = "k";
  this->KeyCodeToKeySymTable[AKEYCODE_L] = "l";
  this->KeyCodeToKeySymTable[AKEYCODE_M] = "m";
  this->KeyCodeToKeySymTable[AKEYCODE_N] = "n";
  this->KeyCodeToKeySymTable[AKEYCODE_O] = "o";
  this->KeyCodeToKeySymTable[AKEYCODE_P] = "p";
  this->KeyCodeToKeySymTable[AKEYCODE_Q] = "q";
  this->KeyCodeToKeySymTable[AKEYCODE_R] = "r";
  this->KeyCodeToKeySymTable[AKEYCODE_S] = "s";
  this->KeyCodeToKeySymTable[AKEYCODE_T] = "t";
  this->KeyCodeToKeySymTable[AKEYCODE_U] = "u";
  this->KeyCodeToKeySymTable[AKEYCODE_V] = "v";
  this->KeyCodeToKeySymTable[AKEYCODE_W] = "w";
  this->KeyCodeToKeySymTable[AKEYCODE_X] = "x";
  this->KeyCodeToKeySymTable[AKEYCODE_Y] = "y";
  this->KeyCodeToKeySymTable[AKEYCODE_Z] = "z";
  this->KeyCodeToKeySymTable[AKEYCODE_COMMA] = "comma";
  this->KeyCodeToKeySymTable[AKEYCODE_PERIOD] = "period";
  this->KeyCodeToKeySymTable[AKEYCODE_ALT_LEFT] = "Alt_L";
  this->KeyCodeToKeySymTable[AKEYCODE_ALT_RIGHT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_SHIFT_LEFT] = "Shift_L";
  this->KeyCodeToKeySymTable[AKEYCODE_SHIFT_RIGHT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_TAB] = "Tab";
  this->KeyCodeToKeySymTable[AKEYCODE_SPACE] = "space";
  this->KeyCodeToKeySymTable[AKEYCODE_SYM] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_EXPLORER] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_ENVELOPE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_ENTER] = "Return";
  this->KeyCodeToKeySymTable[AKEYCODE_DEL] = "Delete";
  this->KeyCodeToKeySymTable[AKEYCODE_GRAVE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MINUS] = "minus";
  this->KeyCodeToKeySymTable[AKEYCODE_EQUALS] = "equal";
  this->KeyCodeToKeySymTable[AKEYCODE_LEFT_BRACKET] = "bracketleft";
  this->KeyCodeToKeySymTable[AKEYCODE_RIGHT_BRACKET] = "bracketright";
  this->KeyCodeToKeySymTable[AKEYCODE_BACKSLASH] = "backslash";
  this->KeyCodeToKeySymTable[AKEYCODE_SEMICOLON] = "semicolon";
  this->KeyCodeToKeySymTable[AKEYCODE_APOSTROPHE] = "exclam";
  this->KeyCodeToKeySymTable[AKEYCODE_SLASH] = "slash";
  this->KeyCodeToKeySymTable[AKEYCODE_AT] = "quotedbl";
  this->KeyCodeToKeySymTable[AKEYCODE_NUM] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_HEADSETHOOK] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_FOCUS] = 0;   // *Camera* focus
  this->KeyCodeToKeySymTable[AKEYCODE_PLUS] = "plus";
  this->KeyCodeToKeySymTable[AKEYCODE_MENU] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_NOTIFICATION] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_SEARCH] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_PLAY_PAUSE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_STOP] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_NEXT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_PREVIOUS] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_REWIND] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_FAST_FORWARD] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MUTE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_PAGE_UP] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_PAGE_DOWN] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_PICTSYMBOLS] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_SWITCH_CHARSET] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_A] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_B] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_C] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_X] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_Y] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_Z] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_L1] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_R1] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_L2] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_R2] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_THUMBL] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_THUMBR] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_START] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_SELECT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_MODE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_ESCAPE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_FORWARD_DEL] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_CTRL_LEFT] = "Control_L";
  this->KeyCodeToKeySymTable[AKEYCODE_CTRL_RIGHT] = "Control_R";
  this->KeyCodeToKeySymTable[AKEYCODE_CAPS_LOCK] = "Caps_Lock";
  this->KeyCodeToKeySymTable[AKEYCODE_SCROLL_LOCK] = "Scroll_Lock";
  this->KeyCodeToKeySymTable[AKEYCODE_META_LEFT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_META_RIGHT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_FUNCTION] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_SYSRQ] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BREAK] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MOVE_HOME] = "Home";
  this->KeyCodeToKeySymTable[AKEYCODE_MOVE_END] = "End";
  this->KeyCodeToKeySymTable[AKEYCODE_INSERT] = "Insert";
  this->KeyCodeToKeySymTable[AKEYCODE_FORWARD] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_PLAY] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_PAUSE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_CLOSE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_EJECT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MEDIA_RECORD] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_F1] = "F1";
  this->KeyCodeToKeySymTable[AKEYCODE_F2] = "F2";
  this->KeyCodeToKeySymTable[AKEYCODE_F3] = "F3";
  this->KeyCodeToKeySymTable[AKEYCODE_F4] = "F4";
  this->KeyCodeToKeySymTable[AKEYCODE_F5] = "F5";
  this->KeyCodeToKeySymTable[AKEYCODE_F6] = "F6";
  this->KeyCodeToKeySymTable[AKEYCODE_F7] = "F7";
  this->KeyCodeToKeySymTable[AKEYCODE_F8] = "F8";
  this->KeyCodeToKeySymTable[AKEYCODE_F9] = "F9";
  this->KeyCodeToKeySymTable[AKEYCODE_F10] = "F10";
  this->KeyCodeToKeySymTable[AKEYCODE_F11] = "F11";
  this->KeyCodeToKeySymTable[AKEYCODE_F12] = "F12";
  this->KeyCodeToKeySymTable[AKEYCODE_NUM_LOCK] = "Num_Lock";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_0] = "KP_0";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_1] = "KP_1";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_2] = "KP_2";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_3] = "KP_3";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_4] = "KP_4";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_5] = "KP_5";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_6] = "KP_6";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_7] = "KP_7";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_8] = "KP_8";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_9] = "KP_9";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_DIVIDE] = "slash";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_MULTIPLY] = "asterisk";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_SUBTRACT] = "minus";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_ADD] = "plus";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_DOT] = "period";
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_COMMA] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_ENTER] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_EQUALS] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_LEFT_PAREN] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_NUMPAD_RIGHT_PAREN] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_VOLUME_MUTE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_INFO] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_CHANNEL_UP] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_CHANNEL_DOWN] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_ZOOM_IN] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_ZOOM_OUT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_TV] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_WINDOW] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_GUIDE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_DVR] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BOOKMARK] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_CAPTIONS] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_SETTINGS] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_TV_POWER] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_TV_INPUT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_STB_POWER] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_STB_INPUT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_AVR_POWER] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_AVR_INPUT] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_PROG_RED] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_PROG_GREEN] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_PROG_YELLOW] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_PROG_BLUE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_APP_SWITCH] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_1] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_2] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_3] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_4] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_5] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_6] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_7] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_8] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_9] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_10] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_11] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_12] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_13] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_14] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_15] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_BUTTON_16] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_LANGUAGE_SWITCH] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_MANNER_MODE] = 0;
  this->KeyCodeToKeySymTable[AKEYCODE_3D_MODE] = 0;
}

//----------------------------------------------------------------------------
vtkAndroidRenderWindowInteractor::~vtkAndroidRenderWindowInteractor()
{
}

//----------------------------------------------------------------------------
void  vtkAndroidRenderWindowInteractor::StartEventLoop()
{
  this->StartedMessageLoop = 1;
  this->Done = false;

  LOGW("Starting event loop");
  while (!this->Done)
    {
    // Read all pending events.
    int ident;
    int events;
    struct android_poll_source* source;

    ident = ALooper_pollAll(500, NULL, &events, (void**)&source);
    if (ident == ALOOPER_POLL_TIMEOUT)
      {
      // just watch for resize events
      if (this->AndroidApplication->window && this->Enabled)
        {
        // so it seems that andoid's configuration changes first
        // then the size of the native window changes a bit later
        // then after some rendering is done the egl surface gets resized
        // we try to handle that mess by watching for when the native window
        // changes size, then we update our size, render to push the change
        // down to egl, then render again.  I suspect there may be a
        // better way to get the change down to egl than doing a full render
        int width = ANativeWindow_getWidth(this->AndroidApplication->window);
        int height = ANativeWindow_getHeight(this->AndroidApplication->window);
        if (width != this->Size[0] || height != this->Size[1])
          {
          this->UpdateSize(width,height);
          this->RenderWindow->Render();
          this->RenderWindow->Render();
          vtkErrorMacro("Config Resized to " << width << " by " << height);
          }
        }
      }

    if (ident >= 0)
      {
      LOGW("Processing Event");
      // Process this event.
      if (source != NULL)
        {
        source->process(this->AndroidApplication, source);
        }

      // Check if we are exiting.
      if (this->AndroidApplication->destroyRequested != 0)
        {
        LOGW("Destroying Window");
        this->RenderWindow->Finalize();
        LOGW("Destroyed");
        return;
        }
      }
    }
}

static void android_handle_cmd(struct android_app* app, int32_t cmd)
{
  vtkAndroidRenderWindowInteractor *self = (vtkAndroidRenderWindowInteractor *)app->userData;
  self->HandleCommand(cmd);
}

void vtkAndroidRenderWindowInteractor::HandleCommand(int32_t cmd)
{
  LOGW("Handling Command");
  switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
      // The window is being shown, get it ready.
      if (this->RenderWindow != NULL)
        {
        LOGW("Creating Window");
        this->RenderWindow->SetWindowId(this->AndroidApplication->window);
        this->RenderWindow->Start();
        LOGW("Done Creating Window start");
        this->RenderWindow->Render();
        LOGW("Done first render");
        }
      break;
//    case APP_CMD_CONFIG_CHANGED:
//      {
      // android seems to change window sizes at some random amount of time after this
      // event so not much to do here as we do not really know the new size yet
//      }
//      break;
    case APP_CMD_WINDOW_REDRAW_NEEDED:
      {
      this->RenderWindow->Render();
      }
      break;
    case APP_CMD_TERM_WINDOW:
      {
      LOGW("Terminating Window");
      this->RenderWindow->Finalize();
      LOGW("Terminated");
  //    ANativeActivity_finish(this->AndroidApplication->activity);
      }
      break;
    case APP_CMD_DESTROY:
      LOGW("Destroying Application");
      this->Done = true;
      break;
    }
}

static int32_t android_handle_input(struct android_app* app, AInputEvent* event)
{
  vtkAndroidRenderWindowInteractor *self = (vtkAndroidRenderWindowInteractor *)app->userData;
  return self->HandleInput(event);
}

const char *vtkAndroidRenderWindowInteractor::GetKeySym(int keyCode)
{
  const char *keysym = "None";
  if (keyCode <= AKEYCODE_3D_MODE)
    {
    keysym = this->KeyCodeToKeySymTable[keyCode];
    }
  return keysym;
}

void vtkAndroidRenderWindowInteractor::HandleKeyEvent(bool down, int nChar, int metaState, int nRepCnt)
{
  const char *keysym = this->GetKeySym(nChar);
  if (down)
    {
    this->SetKeyEventInformation(metaState & AMETA_CTRL_ON,
                                 metaState & AMETA_SHIFT_ON,
                                 nChar, nRepCnt,
                                 keysym);
    this->SetAltKey(metaState & AMETA_ALT_ON);
    this->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
    return;
    }

  this->SetKeyEventInformation(metaState & AMETA_CTRL_ON,
                               metaState & AMETA_SHIFT_ON,
                               nChar, nRepCnt,
                               keysym);
  this->SetAltKey(metaState & AMETA_ALT_ON);
  this->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
  if (keysym && strlen(keysym) == 1)
    {
    this->SetKeyEventInformation(metaState & AMETA_CTRL_ON,
                                 metaState & AMETA_SHIFT_ON,
                                 keysym[0],
                                 nRepCnt);
    this->InvokeEvent(vtkCommand::CharEvent, NULL);
    }
}

void vtkAndroidRenderWindowInteractor::HandleMotionEvent(
  int action, int eventPointer, int numPtrs,
  int *xPtr, int *yPtr, int *idPtr, int metaState)
{
  // update positions
  //vtkWarningMacro(<< "handling motion event for action: " << action << " with num pointers: " << numPtrs);
  for (int i = 0; i < numPtrs; ++i)
    {
    this->SetEventInformationFlipY(xPtr[i],
                                   yPtr[i],
                                   metaState & AMETA_CTRL_ON,
                                   metaState & AMETA_SHIFT_ON,
                                   0,0,0,
                                   idPtr[i]);
    }
  this->SetPointerIndex(idPtr[eventPointer]);
  switch(action)
    {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
      this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      return;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
      this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
      return;
    case AMOTION_EVENT_ACTION_MOVE:
      this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
      return;
    } // end switch action
}

int32_t vtkAndroidRenderWindowInteractor::HandleInput(AInputEvent* event)
{
  if (!this->Enabled)
    {
    return 0;
    }

  switch (AInputEvent_getType(event))
    {
    case AINPUT_EVENT_TYPE_MOTION:
      {
      int action = AMotionEvent_getAction(event);
      int metaState = AMotionEvent_getMetaState(event);
      int numPtrs = AMotionEvent_getPointerCount(event);
      int eventPointer = action >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
      action = action & AMOTION_EVENT_ACTION_MASK;

      int *xPtr = (int *)malloc(numPtrs*sizeof(int));
      int *yPtr = (int *)malloc(numPtrs*sizeof(int));
      int *idPtr = (int *)malloc(numPtrs*sizeof(int));
      for (int i = 0; i < numPtrs; ++i)
        {
        idPtr[i] = AMotionEvent_getPointerId(event, i);
        xPtr[i] = AMotionEvent_getX(event, i);
        yPtr[i] = AMotionEvent_getY(event, i);
        }
      this->HandleMotionEvent(action, eventPointer, numPtrs,
        xPtr, yPtr, idPtr, metaState);
      free(xPtr);
      free(yPtr);
      free(idPtr);
      return 1;
      }
      break;
    case AINPUT_EVENT_TYPE_KEY:
      {
      int action = AKeyEvent_getAction(event);
      int nChar = AKeyEvent_getKeyCode(event);
      int metaState = AKeyEvent_getMetaState(event);
      int nRepCnt = AKeyEvent_getRepeatCount(event);
      this->HandleKeyEvent(action == AKEY_EVENT_ACTION_DOWN,
        nChar,
        metaState,
        nRepCnt);
      return 1;
      }
      break;
    } // end switch event type motion versus key

  return 0;
}


//----------------------------------------------------------------------------
void vtkAndroidRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vtkErrorMacro(<<"No renderer defined!");
    return;
    }
  if (this->Initialized)
    {
    return;
    }

  vtkEGLRenderWindow *ren;
  int *size;

  this->Initialized = 1;
  // get the info we need from the RenderingWindow
  ren = (vtkEGLRenderWindow *)(this->RenderWindow);

  if (ren->GetOwnWindow())
    {
    this->AndroidApplication->userData = this;
    this->AndroidApplication->onAppCmd = android_handle_cmd;
    this->AndroidApplication->onInputEvent = android_handle_input;

    // run event loop until window mapped
    bool done = false;
    while (!done && this->RenderWindow->GetMapped() == 0)
      {
      // Read all pending events.
      int ident;
      int events;
      struct android_poll_source* source;

      if ((ident = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0)
        {
        // Process this event.
        if (source != NULL)
          {
          source->process(this->AndroidApplication, source);
          }

        // Check if we are exiting.
        if (this->AndroidApplication->destroyRequested != 0)
          {
          LOGW("Destroying Window in init");
          this->RenderWindow->Finalize();
          done = true;
          LOGW("Destroyed window in init");
          return;
          }
        }
      }
    }

  size = ren->GetSize();
  ren->GetPosition();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkAndroidRenderWindowInteractor::Enable()
{
  if (this->Enabled)
    {
    return;
    }
  this->Enabled = 1;
  this->Modified();
}


//----------------------------------------------------------------------------
void vtkAndroidRenderWindowInteractor::Disable()
{
  if (!this->Enabled)
    {
    return;
    }

  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAndroidRenderWindowInteractor::TerminateApp(void)
{
  if (this->AndroidApplication)
    {
    ANativeActivity_finish(this->AndroidApplication->activity);
    }

//  this->AndroidApplication->destroyRequested = 1;
}

//----------------------------------------------------------------------------
int vtkAndroidRenderWindowInteractor::InternalCreateTimer(int timerId, int vtkNotUsed(timerType),
                                                          unsigned long duration)
{
  // todo
  return 0;
}

//----------------------------------------------------------------------------
int vtkAndroidRenderWindowInteractor::InternalDestroyTimer(int platformTimerId)
{
  // todo
  return 0;
}


//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void
vtkAndroidRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkAndroidRenderWindowInteractor::ClassExitMethod
       || arg != vtkAndroidRenderWindowInteractor::ClassExitMethodArg)
    {
    // delete the current arg if there is a delete method
    if ((vtkAndroidRenderWindowInteractor::ClassExitMethodArg)
        && (vtkAndroidRenderWindowInteractor::ClassExitMethodArgDelete))
      {
      (*vtkAndroidRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkAndroidRenderWindowInteractor::ClassExitMethodArg);
      }
    vtkAndroidRenderWindowInteractor::ClassExitMethod = f;
    vtkAndroidRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
    }
}

//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void
vtkAndroidRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkAndroidRenderWindowInteractor::ClassExitMethodArgDelete)
    {
    vtkAndroidRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
    }
}

//----------------------------------------------------------------------------
void vtkAndroidRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//----------------------------------------------------------------------------
void vtkAndroidRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
    {
    this->InvokeEvent(vtkCommand::ExitEvent,NULL);
    }
  else if (this->ClassExitMethod)
    {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
    }

  this->TerminateApp();
}
