/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartInteractor.cxx

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

/// \file vtkQtChartInteractor.cxx
/// \date 5/2/2007

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartInteractor.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartKeyboardFunction.h"
#include "vtkQtChartMouseFunction.h"

#include <QCursor>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QMouseEvent>
#include <QRect>
#include <QShortcut>
#include <QVector>
#include <QWheelEvent>


class vtkQtChartInteractorModeItem
{
public:
  vtkQtChartInteractorModeItem(vtkQtChartMouseFunction *function,
      Qt::KeyboardModifiers modifiers);
  vtkQtChartInteractorModeItem(const vtkQtChartInteractorModeItem &other);
  ~vtkQtChartInteractorModeItem() {}

  vtkQtChartMouseFunction *Function;
  Qt::KeyboardModifiers Modifiers;
};


class vtkQtChartInteractorMode
{
public:
  vtkQtChartInteractorMode();
  vtkQtChartInteractorMode(const vtkQtChartInteractorMode &other);
  ~vtkQtChartInteractorMode() {}

  vtkQtChartMouseFunction *getFunction(Qt::KeyboardModifiers modifiers);

  QList<vtkQtChartInteractorModeItem> Functions;
};


class vtkQtChartInteractorModeList
{
public:
  vtkQtChartInteractorModeList();
  vtkQtChartInteractorModeList(const vtkQtChartInteractorModeList &other);
  ~vtkQtChartInteractorModeList() {}

  vtkQtChartInteractorMode *getCurrentMode();

  QList<vtkQtChartInteractorMode> Modes;
  int CurrentMode;
};


class vtkQtChartInteractorInternal
{
public:
  vtkQtChartInteractorInternal();
  ~vtkQtChartInteractorInternal() {}

  vtkQtChartInteractorModeList *getModeList(Qt::MouseButton button);
  vtkQtChartInteractorModeList *getWheelModeList();

  vtkQtChartMouseFunction *Owner;
  vtkQtChartInteractorModeList *OwnerList;
  QVector<vtkQtChartInteractorModeList> Buttons;
  QMap<QKeySequence, vtkQtChartKeyboardFunction *> Keys;
};


//----------------------------------------------------------------------------
vtkQtChartInteractorModeItem::vtkQtChartInteractorModeItem(
    vtkQtChartMouseFunction *function, Qt::KeyboardModifiers modifiers)
{
  this->Function = function;
  this->Modifiers = modifiers;
}

vtkQtChartInteractorModeItem::vtkQtChartInteractorModeItem(
    const vtkQtChartInteractorModeItem &other)
{
  this->Function = other.Function;
  this->Modifiers = other.Modifiers;
}


//----------------------------------------------------------------------------
vtkQtChartInteractorMode::vtkQtChartInteractorMode()
  : Functions()
{
}

vtkQtChartInteractorMode::vtkQtChartInteractorMode(
    const vtkQtChartInteractorMode &other)
  : Functions()
{
  // Copy the list of functions.
  QList<vtkQtChartInteractorModeItem>::ConstIterator iter;
  for(iter = other.Functions.begin(); iter != other.Functions.end(); ++iter)
    {
    this->Functions.append(*iter);
    }
}

vtkQtChartMouseFunction *vtkQtChartInteractorMode::getFunction(
    Qt::KeyboardModifiers modifiers)
{
  // If there is only one function, ignore the event modifiers.
  if(this->Functions.size() == 1)
    {
    return this->Functions[0].Function;
    }

  QList<vtkQtChartInteractorModeItem>::Iterator iter = this->Functions.begin();
  for( ; iter != this->Functions.end(); ++iter)
    {
    if(modifiers == iter->Modifiers)
      {
      return iter->Function;
      }
    }

  return 0;
}


//----------------------------------------------------------------------------
vtkQtChartInteractorModeList::vtkQtChartInteractorModeList()
  : Modes()
{
  this->CurrentMode = 0;
}

vtkQtChartInteractorModeList::vtkQtChartInteractorModeList(
    const vtkQtChartInteractorModeList &other)
  : Modes()
{
  this->CurrentMode = other.CurrentMode;

  // Copy the mode list.
  QList<vtkQtChartInteractorMode>::ConstIterator iter = other.Modes.begin();
  for( ; iter != other.Modes.end(); ++iter)
    {
    this->Modes.append(*iter);
    }
}

vtkQtChartInteractorMode *vtkQtChartInteractorModeList::getCurrentMode()
{
  if(this->CurrentMode < this->Modes.size())
    {
    return &this->Modes[this->CurrentMode];
    }

  return 0;
}


//----------------------------------------------------------------------------
vtkQtChartInteractorInternal::vtkQtChartInteractorInternal()
  : Buttons(4), Keys()
{
  this->Owner = 0;
  this->OwnerList = 0;
}

vtkQtChartInteractorModeList *vtkQtChartInteractorInternal::getModeList(
    Qt::MouseButton button)
{
  if(button == Qt::LeftButton)
    {
    return &this->Buttons[0];
    }
  else if(button == Qt::MidButton)
    {
    return &this->Buttons[1];
    }
  else if(button == Qt::RightButton)
    {
    return &this->Buttons[2];
    }

  return 0;
}

vtkQtChartInteractorModeList *vtkQtChartInteractorInternal::getWheelModeList()
{
  return &this->Buttons[3];
}


//----------------------------------------------------------------------------
vtkQtChartInteractor::vtkQtChartInteractor(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new vtkQtChartInteractorInternal();
  this->ChartArea = 0;
  this->XModifier = Qt::ControlModifier;
  this->YModifier = Qt::AltModifier;
}

vtkQtChartInteractor::~vtkQtChartInteractor()
{
  delete this->Internal;
}

void vtkQtChartInteractor::setChartArea(vtkQtChartArea *area)
{
  QMap<QKeySequence, vtkQtChartKeyboardFunction *>::Iterator jter;
  if(this->ChartArea)
    {
    // Clear the chart area pointer in the keyboard functions.
    jter =this->Internal->Keys.begin();
    for( ; jter != this->Internal->Keys.end(); ++jter)
      {
      (*jter)->setChartArea(0);
      }
    }

  this->ChartArea = area;
  if(this->ChartArea)
    {
    // Assign the new chart area to the keyboard functions.
    jter =this->Internal->Keys.begin();
    for( ; jter != this->Internal->Keys.end(); ++jter)
      {
      (*jter)->setChartArea(this->ChartArea);
      }
    }
}

void vtkQtChartInteractor::setFunction(Qt::MouseButton button,
    vtkQtChartMouseFunction *function, Qt::KeyboardModifiers modifiers)
{
  this->removeFunctions(button);
  this->addFunction(button, function, modifiers);
}

void vtkQtChartInteractor::setWheelFunction(vtkQtChartMouseFunction *function,
    Qt::KeyboardModifiers modifiers)
{
  this->removeWheelFunctions();
  this->addWheelFunction(function, modifiers);
}

void vtkQtChartInteractor::addFunction(Qt::MouseButton button,
    vtkQtChartMouseFunction *function, Qt::KeyboardModifiers modifiers)
{
  if(!function)
    {
    return;
    }

  this->addFunction(this->Internal->getModeList(button), function, modifiers);
}

void vtkQtChartInteractor::addWheelFunction(vtkQtChartMouseFunction *function,
    Qt::KeyboardModifiers modifiers)
{
  if(!function)
    {
    return;
    }

  this->addFunction(this->Internal->getWheelModeList(), function, modifiers);
}

void vtkQtChartInteractor::removeFunction(vtkQtChartMouseFunction *function)
{
  if(!function)
    {
    return;
    }

  // If the function being removed is currently active, cancel the
  // mouse state.
  if(function == this->Internal->Owner)
    {
    this->Internal->Owner->setMouseOwner(false);
    this->Internal->Owner = 0;
    this->Internal->OwnerList = 0;
    }

  // Find the function and remove it from the list.
  QVector<vtkQtChartInteractorModeList>::Iterator iter =
      this->Internal->Buttons.begin();
  for( ; iter != this->Internal->Buttons.end(); ++iter)
    {
    QList<vtkQtChartInteractorMode>::Iterator jter = iter->Modes.begin();
    for(int index = 0; jter != iter->Modes.end(); ++jter, ++index)
      {
      QList<vtkQtChartInteractorModeItem>::Iterator kter =
          jter->Functions.begin();
      for( ; kter != jter->Functions.end(); ++kter)
        {
        if(function == kter->Function)
          {
          jter->Functions.erase(kter);
          if(jter->Functions.size() == 0)
            {
            // Remove the mode if it is empty.
            iter->Modes.erase(jter);
            if(index == iter->CurrentMode)
              {
              iter->CurrentMode = 0;
              }
            }

          break;
          }
        }
      }
    }

  // Disconnect from the function signals.
  this->disconnect(function, 0, this, 0);
}

void vtkQtChartInteractor::removeFunctions(Qt::MouseButton button)
{
  this->removeFunctions(this->Internal->getModeList(button));
}

void vtkQtChartInteractor::removeWheelFunctions()
{
  this->removeFunctions(this->Internal->getWheelModeList());
}

void vtkQtChartInteractor::removeAllFunctions()
{
  this->removeFunctions(Qt::LeftButton);
  this->removeFunctions(Qt::MidButton);
  this->removeFunctions(Qt::RightButton);
  this->removeWheelFunctions();
}

int vtkQtChartInteractor::getNumberOfModes(Qt::MouseButton button) const
{
  vtkQtChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list)
    {
    return list->Modes.size();
    }

  return 0;
}

int vtkQtChartInteractor::getMode(Qt::MouseButton button) const
{
  vtkQtChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list)
    {
    return list->CurrentMode;
    }

  return 0;
}

void vtkQtChartInteractor::setMode(Qt::MouseButton button, int index)
{
  vtkQtChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list && index >= 0 && index < list->Modes.size())
    {
    list->CurrentMode = index;
    }
}

int vtkQtChartInteractor::getNumberOfWheelModes() const
{
  vtkQtChartInteractorModeList *list = this->Internal->getWheelModeList();
  if(list)
    {
    return list->Modes.size();
    }

  return 0;
}

int vtkQtChartInteractor::getWheelMode() const
{
  vtkQtChartInteractorModeList *list = this->Internal->getWheelModeList();
  if(list)
    {
    return list->CurrentMode;
    }

  return 0;
}

void vtkQtChartInteractor::setWheelMode(int index)
{
  vtkQtChartInteractorModeList *list = this->Internal->getWheelModeList();
  if(list && index >= 0 && index < list->Modes.size())
    {
    list->CurrentMode = index;
    }
}

void vtkQtChartInteractor::addKeyboardFunction(const QKeySequence &sequence,
    vtkQtChartKeyboardFunction *function)
{
  if(!function)
    {
    return;
    }

  // Make sure the sequence doesn't exist.
  QMap<QKeySequence, vtkQtChartKeyboardFunction *>::Iterator iter =
      this->Internal->Keys.find(sequence);
  if(iter == this->Internal->Keys.end())
    {
    // Add the function to the list.
    this->Internal->Keys.insert(sequence, function);
    function->setChartArea(this->ChartArea);
    }
}

void vtkQtChartInteractor::removeKeyboardFunction(
    vtkQtChartKeyboardFunction *function)
{
  if(!function)
    {
    return;
    }

  // Search the list for the function. The function can be added to
  // more than one key sequence.
  function->setChartArea(0);
  QMap<QKeySequence, vtkQtChartKeyboardFunction *>::Iterator iter =
      this->Internal->Keys.begin();
  while(iter != this->Internal->Keys.end())
    {
    if(*iter == function)
      {
      // Remove the function from the list.
      iter = this->Internal->Keys.erase(iter);
      }
    else
      {
      ++iter;
      }
    }
}

void vtkQtChartInteractor::removeKeyboardFunctions()
{
  // Remove all the keyboard functions.
  this->Internal->Keys.clear();
}

bool vtkQtChartInteractor::keyPressEvent(QKeyEvent *e)
{
  if(!this->ChartArea)
    {
    return false;
    }

  // Create a key sequence object from the key event.
  QKeySequence sequence(e->key() | (e->modifiers() & (Qt::ShiftModifier |
      Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)));

  // Search the list of functions for the sequence.
  QMap<QKeySequence, vtkQtChartKeyboardFunction *>::Iterator iter =
      this->Internal->Keys.find(sequence);
  if(iter == this->Internal->Keys.end())
    {
    return false;
    }

  // Call the keyboard function.
  (*iter)->activate();
  return true;
}

void vtkQtChartInteractor::mousePressEvent(QMouseEvent *e)
{
  // Find the mode list associated with a button. If a function on
  // another button owns the mouse state, don't pass the event to the
  // button's functions.
  bool handled = false;
  vtkQtChartInteractorModeList *list =
      this->Internal->getModeList(e->button());
  if(list && (!this->Internal->OwnerList || list == this->Internal->OwnerList))
    {
    // If there is an active function, send it the event. If not,
    // find the function for the current mode and modifiers.
    vtkQtChartMouseFunction *function = this->Internal->Owner;
    if(!function)
      {
      vtkQtChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mousePressEvent(e, this->ChartArea);
      }
    }

  if(handled || this->Internal->Owner)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartInteractor::mouseMoveEvent(QMouseEvent *e)
{
  // See which buttons are pressed.
  vtkQtChartInteractorModeList *left = 0;
  vtkQtChartInteractorModeList *middle = 0;
  vtkQtChartInteractorModeList *right = 0;
  Qt::MouseButtons buttons = e->buttons();
  if(buttons & Qt::LeftButton)
    {
    left = this->Internal->getModeList(Qt::LeftButton);
    }

  if(buttons & Qt::MidButton)
    {
    middle = this->Internal->getModeList(Qt::MidButton);
    }

  if(buttons & Qt::RightButton)
    {
    right = this->Internal->getModeList(Qt::RightButton);
    }

  bool handled = false;
  if(left || middle || right)
    {
    // If more than one button is pressed and no function is active,
    // it is unclear which function to call. An active function can
    // be called even if multiple buttons are pressed.
    vtkQtChartMouseFunction *function = 0;
    bool multiple = (left && middle) || (left && right) || (middle && right);
    if(this->Internal->Owner)
      {
      // Make sure the owner's button is pressed.
      if(this->Internal->OwnerList == left ||
          this->Internal->OwnerList == middle ||
          this->Internal->OwnerList == right)
        {
        function = this->Internal->Owner;
        }
      }
    else if(!multiple)
      {
      vtkQtChartInteractorModeList *list = left;
      if(!list)
        {
        list = middle;
        }

      if(!list)
        {
        list = right;
        }

      vtkQtChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mouseMoveEvent(e, this->ChartArea);
      }
    }

  if(handled)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartInteractor::mouseReleaseEvent(QMouseEvent *e)
{
  // Find the mode list associated with a button. Always send the
  // mouse release event even if a function on another button owns
  // the state. This ensures that mouse press events sent to mouse
  // functions receive mouse release events.
  bool handled = false;
  vtkQtChartInteractorModeList *list =
      this->Internal->getModeList(e->button());
  if(list)
    {
    // If there is an active function, send it the event. If not,
    // find the function for the current mode and modifiers.
    vtkQtChartMouseFunction *function = 0;
    if(this->Internal->OwnerList == list)
      {
      function = this->Internal->Owner;
      }

    if(!function)
      {
      vtkQtChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mouseReleaseEvent(e, this->ChartArea);
      }
    }

  if(handled || this->Internal->Owner)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartInteractor::mouseDoubleClickEvent(QMouseEvent *e)
{
  // Find the mode list associated with a button. If a function on
  // another button owns the mouse state, don't pass the event to the
  // button's functions.
  bool handled = false;
  vtkQtChartInteractorModeList *list =
      this->Internal->getModeList(e->button());
  if(list && (!this->Internal->OwnerList || list == this->Internal->OwnerList))
    {
    // If there is an active function, send it the event. If not,
    // find the function for the current mode and modifiers.
    vtkQtChartMouseFunction *function = this->Internal->Owner;
    if(!function)
      {
      vtkQtChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mouseDoubleClickEvent(e, this->ChartArea);
      }
    }

  if(handled || this->Internal->Owner)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartInteractor::wheelEvent(QWheelEvent *e)
{
  // Find the mode list associated with a button. If a function on
  // another button owns the mouse state, don't pass the event to the
  // button's functions.
  bool handled = false;
  vtkQtChartInteractorModeList *list = this->Internal->getWheelModeList();
  if(list && (!this->Internal->OwnerList || list == this->Internal->OwnerList))
    {
    // If there is an active function, send it the event. If not,
    // find the function for the current mode and modifiers.
    vtkQtChartMouseFunction *function = this->Internal->Owner;
    if(!function)
      {
      vtkQtChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->wheelEvent(e, this->ChartArea);
      }
    }

  if(handled || this->Internal->Owner)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartInteractor::beginState(vtkQtChartMouseFunction *owner)
{
  if(this->Internal->Owner == 0)
    {
    // Find the mouse button this function is attached to.
    QVector<vtkQtChartInteractorModeList>::Iterator iter =
        this->Internal->Buttons.begin();
    for( ; iter != this->Internal->Buttons.end(); ++iter)
      {
      QList<vtkQtChartInteractorMode>::Iterator jter = iter->Modes.begin();
      for( ; jter != iter->Modes.end(); ++jter)
        {
        QList<vtkQtChartInteractorModeItem>::Iterator kter =
            jter->Functions.begin();
        for( ; kter != jter->Functions.end(); ++kter)
          {
          if(owner == kter->Function)
            {
            owner->setMouseOwner(true);
            this->Internal->Owner = owner;
            this->Internal->OwnerList = &(*iter);
            break;
            }
          }
        }
      }
    }
}

void vtkQtChartInteractor::endState(vtkQtChartMouseFunction *owner)
{
  if(owner && this->Internal->Owner == owner)
    {
    owner->setMouseOwner(false);
    this->Internal->Owner = 0;
    this->Internal->OwnerList = 0;
    }
}

void vtkQtChartInteractor::addFunction(vtkQtChartInteractorModeList *list,
    vtkQtChartMouseFunction *function, Qt::KeyboardModifiers modifiers)
{
  if(list)
    {
    vtkQtChartInteractorMode *mode = 0;
    if(function->isCombinable())
      {
      // If the function is combinable, search for a compatible mode.
      QList<vtkQtChartInteractorMode>::Iterator iter = list->Modes.begin();
      for( ; iter != list->Modes.end(); ++iter)
        {
        bool canCombine = true;
        QList<vtkQtChartInteractorModeItem>::Iterator jter =
            iter->Functions.begin();
        for( ; jter != iter->Functions.end(); ++jter)
          {
          if(!jter->Function->isCombinable())
            {
            canCombine = false;
            break;
            }

          if(modifiers == jter->Modifiers)
            {
            canCombine = false;
            break;
            }
          }

        if(canCombine)
          {
          mode = &(*iter);
          break;
          }
        }
      }

    if(!mode)
      {
      // Add a new mode if the function can't be added to any of the
      // current modes.
      list->Modes.append(vtkQtChartInteractorMode());
      mode = &list->Modes.last();
      }

    // Finally, add the function to the mode.
    mode->Functions.append(vtkQtChartInteractorModeItem(function, modifiers));
    this->connect(function, SIGNAL(cursorChangeRequested(const QCursor &)),
        this, SIGNAL(cursorChangeRequested(const QCursor &)));
    this->connect(
        function, SIGNAL(interactionStarted(vtkQtChartMouseFunction *)),
        this, SLOT(beginState(vtkQtChartMouseFunction *)));
    this->connect(
        function, SIGNAL(interactionFinished(vtkQtChartMouseFunction *)),
        this, SLOT(endState(vtkQtChartMouseFunction *)));
    }
}

void vtkQtChartInteractor::removeFunctions(vtkQtChartInteractorModeList *list)
{
  if(list)
    {
    // If the list contains an active function, cancel the mouse
    // state before removing the button's functions.
    if(this->Internal->Owner && list == this->Internal->OwnerList)
      {
      this->Internal->Owner->setMouseOwner(false);
      this->Internal->Owner = 0;
      this->Internal->OwnerList = 0;
      }

    // Disconnect from the function signals.
    QList<vtkQtChartInteractorMode>::Iterator iter = list->Modes.begin();
    for( ; iter != list->Modes.end(); ++iter)
      {
      QList<vtkQtChartInteractorModeItem>::Iterator jter =
          iter->Functions.begin();
      for( ; jter != iter->Functions.end(); ++jter)
        {
        this->disconnect(jter->Function, 0, this, 0);
        }
      }

    // Clear all the button functions.
    list->CurrentMode = 0;
    list->Modes.clear();
    }
}


