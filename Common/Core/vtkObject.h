/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkObject - abstract base class for most VTK objects
// .SECTION Description
// vtkObject is the base class for most objects in the visualization
// toolkit. vtkObject provides methods for tracking modification time,
// debugging, printing, and event callbacks. Most objects created
// within the VTK framework should be a subclass of vtkObject or one
// of its children.  The few exceptions tend to be very small helper
// classes that usually never get instantiated or situations where
// multiple inheritance gets in the way.  vtkObject also performs
// reference counting: objects that are reference counted exist as
// long as another object uses them. Once the last reference to a
// reference counted object is removed, the object will spontaneously
// destruct.

// .SECTION Caveats
// Note: in VTK objects should always be created with the New() method
// and deleted with the Delete() method. VTK objects cannot be
// allocated off the stack (i.e., automatic objects) because the
// constructor is a protected method.

// .SECTION See also
// vtkCommand vtkTimeStamp

#ifndef vtkObject_h
#define vtkObject_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObjectBase.h"
#include "vtkSetGet.h"
#include "vtkTimeStamp.h"
#include "vtkWeakPointerBase.h" // needed for vtkWeakPointer

class vtkSubjectHelper;
class vtkCommand;

class VTKCOMMONCORE_EXPORT vtkObject : public vtkObjectBase
{
public:
  vtkTypeMacro(vtkObject,vtkObjectBase);

  // Description:
  // Create an object with Debug turned off, modified time initialized
  // to zero, and reference counting on.
  static vtkObject *New();

#ifdef _WIN32
  // avoid dll boundary problems
  void* operator new( size_t tSize );
  void operator delete( void* p );
#endif

  // Description:
  // Turn debugging output on.
  virtual void DebugOn();

  // Description:
  // Turn debugging output off.
  virtual void DebugOff();

  // Description:
  // Get the value of the debug flag.
  unsigned char GetDebug();

  // Description:
  // Set the value of the debug flag. A non-zero value turns debugging on.
  void SetDebug(unsigned char debugFlag);

  // Description:
  // This method is called when vtkErrorMacro executes. It allows
  // the debugger to break on error.
  static void BreakOnError();

  // Description:
  // Update the modification time for this object. Many filters rely on
  // the modification time to determine if they need to recompute their
  // data. The modification time is a unique monotonically increasing
  // unsigned long integer.
  virtual void Modified();

  // Description:
  // Return this object's modified time.
  virtual unsigned long GetMTime();

  // Description:
  // Methods invoked by print to print information about the object
  // including superclasses. Typically not called by the user (use
  // Print() instead) but used in the hierarchical print process to
  // combine the output of several classes.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is a global flag that controls whether any debug, warning
  // or error messages are displayed.
  static void SetGlobalWarningDisplay(int val);
  static void GlobalWarningDisplayOn(){vtkObject::SetGlobalWarningDisplay(1);};
  static void GlobalWarningDisplayOff()
    {vtkObject::SetGlobalWarningDisplay(0);};
  static int  GetGlobalWarningDisplay();

//BTX
  // Description:
  // Allow people to add/remove/invoke observers (callbacks) to any VTK
  // object.  This is an implementation of the subject/observer design
  // pattern. An observer is added by specifying an event to respond to
  // and a vtkCommand to execute. It returns an unsigned long tag which
  // can be used later to remove the event or retrieve the command.
  // When events are invoked, the observers are called in the order they
  // were added. If a priority value is specified, then the higher
  // priority commands are called first. A command may set an abort
  // flag to stop processing of the event. (See vtkCommand.h for more
  // information.)
  unsigned long AddObserver(unsigned long event, vtkCommand *,
                            float priority=0.0f);
  unsigned long AddObserver(const char *event, vtkCommand *,
                            float priority=0.0f);
  vtkCommand *GetCommand(unsigned long tag);
  void RemoveObserver(vtkCommand*);
  void RemoveObservers(unsigned long event, vtkCommand *);
  void RemoveObservers(const char *event, vtkCommand *);
  int HasObserver(unsigned long event, vtkCommand *);
  int HasObserver(const char *event, vtkCommand *);
//ETX
  void RemoveObserver(unsigned long tag);
  void RemoveObservers(unsigned long event);
  void RemoveObservers(const char *event);
  void RemoveAllObservers(); //remove every last one of them
  int HasObserver(unsigned long event);
  int HasObserver(const char *event);

//BTX
  // Description:
  // Overloads to AddObserver that allow developers to add class member
  // functions as callbacks for events.  The callback function can
  // be one of these two types:
  // \code
  // void foo(void);\n
  // void foo(vtkObject*, unsigned long, void*);
  // \endcode
  // If the callback is a member of a vtkObjectBase-derived object,
  // then the callback will automatically be disabled if the object
  // destructs (but the observer will not automatically be removed).
  // If the callback is a member of any other type of object, then
  // the observer must be removed before the object destructs or else
  // its dead pointer will be used the next time the event occurs.
  // Typical usage of these functions is as follows:
  // \code
  // SomeClassOfMine* observer = SomeClassOfMine::New();\n
  // to_observe->AddObserver(event, observer, \&SomeClassOfMine::SomeMethod);
  // \endcode
  // Note that this does not affect the reference count of a
  // vtkObjectBase-derived \c observer, which can be safely deleted
  // with the observer still in place. For non-vtkObjectBase observers,
  // the observer should never be deleted before it is removed.
  // Return value is a tag that can be used to remove the observer.
  template <class U, class T>
  unsigned long AddObserver(unsigned long event,
    U observer, void (T::*callback)(), float priority=0.0f)
    {
    vtkClassMemberCallback<T> *callable =
      new vtkClassMemberCallback<T>(observer, callback);
    // callable is deleted when the observer is cleaned up (look at
    // vtkObjectCommandInternal)
    return this->AddTemplatedObserver(event, callable, priority);
    }
  template <class U, class T>
  unsigned long AddObserver(unsigned long event,
    U observer, void (T::*callback)(vtkObject*, unsigned long, void*),
    float priority=0.0f)
    {
    vtkClassMemberCallback<T> *callable =
      new vtkClassMemberCallback<T>(observer, callback);
    // callable is deleted when the observer is cleaned up (look at
    // vtkObjectCommandInternal)
    return this->AddTemplatedObserver(event, callable, priority);
    }

  // Description:
  // Allow user to set the AbortFlagOn() with the return value of the callback
  // method.
  template <class U, class T>
  unsigned long AddObserver(unsigned long event,
    U observer, bool (T::*callback)(vtkObject*, unsigned long, void*),
    float priority=0.0f)
    {
    vtkClassMemberCallback<T> *callable =
      new vtkClassMemberCallback<T>(observer, callback);
    // callable is deleted when the observer is cleaned up (look at
    // vtkObjectCommandInternal)
    return this->AddTemplatedObserver(event, callable, priority);
    }
//ETX

//BTX
  // Description:
  // This method invokes an event and return whether the event was
  // aborted or not. If the event was aborted, the return value is 1,
  // otherwise it is 0.
  int InvokeEvent(unsigned long event, void *callData);
  int InvokeEvent(const char *event, void *callData);
//ETX
  int InvokeEvent(unsigned long event) { return this->InvokeEvent(event, NULL); };
  int InvokeEvent(const char *event) { return this->InvokeEvent(event, NULL); };

protected:
  vtkObject();
  virtual ~vtkObject();

  // See vtkObjectBase.h.
  virtual void RegisterInternal(vtkObjectBase*, int check);
  virtual void UnRegisterInternal(vtkObjectBase*, int check);

  unsigned char     Debug;      // Enable debug messages
  vtkTimeStamp      MTime;      // Keep track of modification time
  vtkSubjectHelper *SubjectHelper; // List of observers on this object

//BTX
  // Description:
  // These methods allow a command to exclusively grab all events. (This
  // method is typically used by widgets to grab events once an event
  // sequence begins.)  These methods are provided in support of the
  // public methods found in the class vtkInteractorObserver. Note that
  // these methods are designed to support vtkInteractorObservers since
  // they use two separate vtkCommands to watch for mouse and keypress events.
  void InternalGrabFocus(vtkCommand *mouseEvents, vtkCommand *keypressEvents=NULL);
  void InternalReleaseFocus();
//ETX
//BTX
private:
  vtkObject(const vtkObject&);  // Not implemented.
  void operator=(const vtkObject&);  // Not implemented.

  // Description:
  // Following classes (vtkClassMemberCallbackBase,
  // vtkClassMemberCallback, and vtkClassMemberHanderPointer)
  // along with vtkObjectCommandInternal are for supporting
  // templated AddObserver() overloads that allow developers
  // to add event callbacks that are class member functions.
  class vtkClassMemberCallbackBase
    {
  public:
    // Description:
    // Called when the event is invoked
    virtual bool operator()(vtkObject*, unsigned long, void*) = 0;
    virtual ~vtkClassMemberCallbackBase(){}
    };

  // Description:
  // This is a weak pointer for vtkObjectBase and a regular
  // void pointer for everything else
  template<class T>
    class vtkClassMemberHandlerPointer
      {
    public:
      void operator=(vtkObjectBase *o)
        {
        // The cast is needed in case "o" has multi-inheritance,
        // to offset the pointer to get the vtkObjectBase.
        if ((this->VoidPointer = dynamic_cast<T*>(o)) == 0)
          {
          // fallback to just using its vtkObjectBase as-is.
          this->VoidPointer = o;
          }
        this->WeakPointer = o;
        this->UseWeakPointer = true;
        }
      void operator=(void *o)
        {
        this->VoidPointer = o;
        this->WeakPointer = 0;
        this->UseWeakPointer = false;
        }
      T *GetPointer()
        {
        if (this->UseWeakPointer && !this->WeakPointer.GetPointer())
          {
          return 0;
          }
        return static_cast<T*>(this->VoidPointer);
        }
    private:
      vtkWeakPointerBase WeakPointer;
      void *VoidPointer;
      bool UseWeakPointer;
      };

  // Description:
  // Templated member callback.
  template <class T>
    class vtkClassMemberCallback : public vtkClassMemberCallbackBase
      {
      vtkClassMemberHandlerPointer<T> Handler;
      void (T::*Method1)();
      void (T::*Method2)(vtkObject*, unsigned long, void*);
      bool (T::*Method3)(vtkObject*, unsigned long, void*);

    public:
      vtkClassMemberCallback(T* handler, void (T::*method)())
        {
        this->Handler = handler;
        this->Method1 = method;
        this->Method2 = NULL;
        this->Method3 = NULL;
        }

      vtkClassMemberCallback(
        T* handler, void (T::*method)(vtkObject*, unsigned long, void*))
        {
        this->Handler = handler;
        this->Method1 = NULL;
        this->Method2 = method;
        this->Method3 = NULL;
        }

      vtkClassMemberCallback(
        T* handler, bool (T::*method)(vtkObject*, unsigned long, void*))
        {
        this->Handler = handler;
        this->Method1 = NULL;
        this->Method2 = NULL;
        this->Method3 = method;
        }
      virtual ~vtkClassMemberCallback() { }

      // Called when the event is invoked
      virtual bool operator()(
        vtkObject* caller, unsigned long event, void* calldata)
        {
        T *handler = this->Handler.GetPointer();
        if (handler)
          {
          if (this->Method1)
            {
            (handler->*this->Method1)();
            }
          else if (this->Method2)
            {
            (handler->*this->Method2)(caller, event, calldata);
            }
          else if (this->Method3)
            {
            return (handler->*this->Method3)(caller, event, calldata);
            }
          }
        return false;
        }
      };

  // Description:
  // Called by templated variants of AddObserver.
  unsigned long AddTemplatedObserver(
    unsigned long event, vtkClassMemberCallbackBase* callable, float priority);
  // Friend to access AddTemplatedObserver().
  friend class vtkObjectCommandInternal;
//ETX
};

#endif
// VTK-HeaderTest-Exclude: vtkObject.h
