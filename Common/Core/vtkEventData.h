/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEventData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief   platform-independent event data structures
 */

#ifndef vtkEventData_h
#define vtkEventData_h

#include "vtkCommand.h"

// enumeration of possible devices
enum class vtkEventDataDevice
{
  Unknown = -1,
  HeadMountedDisplay,
  RightController,
  LeftController,
  GenericTracker,
  NumberOfDevices
};

const int vtkEventDataNumberOfDevices = (static_cast<int>(vtkEventDataDevice::NumberOfDevices));

// enumeration of possible device inputs
enum class vtkEventDataDeviceInput
{
  Unknown = -1,
  Trigger,
  TrackPad,
  Joystick,
  Grip,
  ApplicationMenu,
  NumberOfInputs
};

const int vtkEventDataNumberOfInputs = (static_cast<int>(vtkEventDataDeviceInput::NumberOfInputs));

// enumeration of actions that can happen
enum class vtkEventDataAction
{
  Unknown = -1,
  Press,
  Release,
  Touch,
  Untouch,
  NumberOfActions
};

class vtkEventDataForDevice;
class vtkEventDataDevice3D;

class vtkEventData : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkEventData, vtkObjectBase);

  int GetType() const { return this->Type; }

  // are two events equivalent
  bool operator==(const vtkEventData& a) const
  {
    return this->Type == a.Type && this->Equivalent(&a);
  }

  // some convenience downcasts
  virtual vtkEventDataForDevice* GetAsEventDataForDevice() { return nullptr; }
  virtual vtkEventDataDevice3D* GetAsEventDataDevice3D() { return nullptr; }

protected:
  vtkEventData() {}
  ~vtkEventData() override {}

  // subclasses override this to define their
  // definition of equivalent
  virtual bool Equivalent(const vtkEventData* ed) const = 0;

  int Type;

private:
  vtkEventData(const vtkEventData& c) = delete;
};

// a subclass for events that may have one or more of
// device, input, and action
class vtkEventDataForDevice : public vtkEventData
{
public:
  vtkTypeMacro(vtkEventDataForDevice, vtkEventData);
  static vtkEventDataForDevice* New()
  {
    vtkEventDataForDevice* ret = new vtkEventDataForDevice;
    ret->InitializeObjectBase();
    return ret;
  }

  vtkEventDataDevice GetDevice() const { return this->Device; }
  vtkEventDataDeviceInput GetInput() const { return this->Input; }
  vtkEventDataAction GetAction() const { return this->Action; }

  void SetDevice(vtkEventDataDevice v) { this->Device = v; }
  void SetInput(vtkEventDataDeviceInput v) { this->Input = v; }
  void SetAction(vtkEventDataAction v) { this->Action = v; }

  vtkEventDataForDevice* GetAsEventDataForDevice() override { return this; }

protected:
  vtkEventDataDevice Device;
  vtkEventDataDeviceInput Input;
  vtkEventDataAction Action;

  bool Equivalent(const vtkEventData* e) const override
  {
    const vtkEventDataForDevice* edd = static_cast<const vtkEventDataForDevice*>(e);
    return this->Device == edd->Device && this->Input == edd->Input && this->Action == edd->Action;
  }

  vtkEventDataForDevice()
  {
    this->Device = vtkEventDataDevice::Unknown;
    this->Input = vtkEventDataDeviceInput::Unknown;
    this->Action = vtkEventDataAction::Unknown;
  }
  ~vtkEventDataForDevice() override {}

private:
  vtkEventDataForDevice(const vtkEventData& c) = delete;
  void operator=(const vtkEventDataForDevice&) = delete;
};

// a subclass for events that have a 3D world position
// direction and orientation.
class vtkEventDataDevice3D : public vtkEventDataForDevice
{
public:
  vtkTypeMacro(vtkEventDataDevice3D, vtkEventDataForDevice);

  vtkEventDataDevice3D* GetAsEventDataDevice3D() override { return this; }

  void GetWorldPosition(double v[3]) const
  {
    v[0] = this->WorldPosition[0];
    v[1] = this->WorldPosition[1];
    v[2] = this->WorldPosition[2];
  }
  const double* GetWorldPosition() const VTK_SIZEHINT(3) { return this->WorldPosition; }
  void SetWorldPosition(const double p[3])
  {
    this->WorldPosition[0] = p[0];
    this->WorldPosition[1] = p[1];
    this->WorldPosition[2] = p[2];
  }

  void GetWorldDirection(double v[3]) const
  {
    v[0] = this->WorldDirection[0];
    v[1] = this->WorldDirection[1];
    v[2] = this->WorldDirection[2];
  }
  const double* GetWorldDirection() const VTK_SIZEHINT(3) { return this->WorldDirection; }
  void SetWorldDirection(const double p[3])
  {
    this->WorldDirection[0] = p[0];
    this->WorldDirection[1] = p[1];
    this->WorldDirection[2] = p[2];
  }

  void GetWorldOrientation(double v[4]) const
  {
    v[0] = this->WorldOrientation[0];
    v[1] = this->WorldOrientation[1];
    v[2] = this->WorldOrientation[2];
    v[3] = this->WorldOrientation[3];
  }
  const double* GetWorldOrientation() const VTK_SIZEHINT(4) { return this->WorldOrientation; }
  void SetWorldOrientation(const double p[4])
  {
    this->WorldOrientation[0] = p[0];
    this->WorldOrientation[1] = p[1];
    this->WorldOrientation[2] = p[2];
    this->WorldOrientation[3] = p[3];
  }

  void GetTrackPadPosition(double v[2]) const
  {
    v[0] = this->TrackPadPosition[0];
    v[1] = this->TrackPadPosition[1];
  }
  const double* GetTrackPadPosition() const VTK_SIZEHINT(2) { return this->TrackPadPosition; }
  void SetTrackPadPosition(const double p[2])
  {
    this->TrackPadPosition[0] = p[0];
    this->TrackPadPosition[1] = p[1];
  }
  void SetTrackPadPosition(double x, double y)
  {
    this->TrackPadPosition[0] = x;
    this->TrackPadPosition[1] = y;
  }

protected:
  double WorldPosition[3];
  double WorldOrientation[4];
  double WorldDirection[3];
  double TrackPadPosition[2];

  vtkEventDataDevice3D() {}
  ~vtkEventDataDevice3D() override {}

private:
  vtkEventDataDevice3D(const vtkEventDataDevice3D& c) = delete;
  void operator=(const vtkEventDataDevice3D&) = delete;
};

// subclass for button event 3d
class vtkEventDataButton3D : public vtkEventDataDevice3D
{
public:
  vtkTypeMacro(vtkEventDataButton3D, vtkEventDataDevice3D);
  static vtkEventDataButton3D* New()
  {
    vtkEventDataButton3D* ret = new vtkEventDataButton3D;
    ret->InitializeObjectBase();
    return ret;
  }

protected:
  vtkEventDataButton3D() { this->Type = vtkCommand::Button3DEvent; }
  ~vtkEventDataButton3D() override {}

private:
  vtkEventDataButton3D(const vtkEventDataButton3D& c) = delete;
  void operator=(const vtkEventDataButton3D&) = delete;
};

// subclass for move event 3d
class vtkEventDataMove3D : public vtkEventDataDevice3D
{
public:
  vtkTypeMacro(vtkEventDataMove3D, vtkEventDataDevice3D);
  static vtkEventDataMove3D* New()
  {
    vtkEventDataMove3D* ret = new vtkEventDataMove3D;
    ret->InitializeObjectBase();
    return ret;
  }

protected:
  vtkEventDataMove3D() { this->Type = vtkCommand::Move3DEvent; }
  ~vtkEventDataMove3D() override {}

private:
  vtkEventDataMove3D(const vtkEventDataMove3D& c) = delete;
  void operator=(const vtkEventDataMove3D&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkEventData.h
