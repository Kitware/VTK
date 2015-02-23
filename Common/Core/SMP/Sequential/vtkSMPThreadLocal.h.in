 /*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPThreadLocal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPThreadLocal - A simple thread local implementation for sequential operations.
// .SECTION Description
// A thread local object is one that maintains a copy of an object of the
// template type for each thread that processes data. vtkSMPThreadLocal
// creates storage for all threads but the actual objects are created
// the first time Local() is called. Note that some of the vtkSMPThreadLocal
// API is not thread safe. It can be safely used in a multi-threaded
// environment because Local() returns storage specific to a particular
// thread, which by default will be accessed sequentially. It is also
// thread-safe to iterate over vtkSMPThreadLocal as long as each thread
// creates its own iterator and does not change any of the thread local
// objects.
//
// A common design pattern in using a thread local storage object is to
// write/accumulate data to local object when executing in parallel and
// then having a sequential code block that iterates over the whole storage
// using the iterators to do the final accumulation.
//
// Note that this particular implementation is designed to work in sequential
// mode and supports only 1 thread.

#ifndef vtkSMPThreadLocal_h
#define vtkSMPThreadLocal_h

#include "vtkSystemIncludes.h"

#include <vector>

template <typename T>
class vtkSMPThreadLocal
{
  typedef std::vector<T> TLS;
  typedef typename TLS::iterator TLSIter;
public:
  // Description:
  // Default constructor. Creates a default exemplar.
  vtkSMPThreadLocal() : NumInitialized(0)
    {
      this->Initialize();
    }

  // Description:
  // Constructor that allows the specification of an exemplar object
  // which is used when constructing objects when Local() is first called.
  // Note that a copy of the exemplar is created using its copy constructor.
  vtkSMPThreadLocal(const T& exemplar) : NumInitialized(0), Exemplar(exemplar)
    {
      this->Initialize();
    }

  // Description:
  // Returns an object of type T that is local to the current thread.
  // This needs to be called mainly within a threaded execution path.
  // It will create a new object (local to the tread so each thread
  // get their own when calling Local) which is a copy of exemplar as passed
  // to the constructor (or a default object if no exemplar was provided)
  // the first time it is called. After the first time, it will return
  // the same object.
  T& Local()
    {
      int tid = this->GetThreadID();
      if (!this->Initialized[tid])
        {
        this->Internal[tid] = this->Exemplar;
        this->Initialized[tid] = true;
        ++this->NumInitialized;
        }
      return this->Internal[tid];
    }

  // Description:
  // Return the number of thread local objects that have been initialized
  size_t size() const
    {
      return this->NumInitialized;
    }

  // Description:
  // Subset of the standard iterator API.
  // The most common design pattern is to use iterators in a sequential
  // code block and to use only the thread local objects in parallel
  // code blocks.
  // It is thread safe to iterate over the thread local containers
  // as long as each thread uses its own iterator and does not modify
  // objects in the container.
  class iterator
  {
  public:
    iterator& operator++()
      {
        this->InitIter++;
        this->Iter++;

        // Make sure to skip uninitialized
        // entries.
        while(this->InitIter != this->EndIter)
          {
          if (*this->InitIter)
            {
            break;
            }
          this->InitIter++;
          this->Iter++;
          }
        return *this;
      }

    iterator operator++(int)
      {
      iterator copy = *this;
      ++(*this);
      return copy;
      }

    bool operator==(const iterator& other)
      {
        return this->Iter == other.Iter;
      }

    bool operator!=(const iterator& other)
      {
        return this->Iter != other.Iter;
      }

    T& operator*()
      {
        return *this->Iter;
      }

    T* operator->()
      {
        return &*this->Iter;
      }

  private:
    friend class vtkSMPThreadLocal<T>;
    std::vector<bool>::iterator InitIter;
    std::vector<bool>::iterator EndIter;
    TLSIter Iter;
  };

  // Description:
  // Returns a new iterator pointing to the beginning of
  // the local storage container. Thread safe.
  iterator begin()
    {
      TLSIter iter = this->Internal.begin();
      std::vector<bool>::iterator iter2 =
        this->Initialized.begin();
      std::vector<bool>::iterator enditer =
        this->Initialized.end();
      // fast forward to first initialized
      // value
      while(iter2 != enditer)
        {
        if (*iter2)
          {
          break;
          }
        iter2++;
        iter++;
        }
      iterator retVal;
      retVal.InitIter = iter2;
      retVal.EndIter = enditer;
      retVal.Iter = iter;
      return retVal;
    };

  // Description:
  // Returns a new iterator pointing to past the end of
  // the local storage container. Thread safe.
  iterator end()
    {
      iterator retVal;
      retVal.InitIter = this->Initialized.end();
      retVal.EndIter = this->Initialized.end();
      retVal.Iter = this->Internal.end();
      return retVal;
    }

private:
  TLS Internal;
  std::vector<bool> Initialized;
  size_t NumInitialized;
  T Exemplar;

  void Initialize()
    {
      this->Internal.resize(this->GetNumberOfThreads());
      this->Initialized.resize(this->GetNumberOfThreads());
      std::fill(this->Initialized.begin(),
                this->Initialized.end(),
                false);
    }

  inline int GetNumberOfThreads()
    {
      return 1;
    }

  inline int GetThreadID()
    {
      return 0;
    }
};
#endif
// VTK-HeaderTest-Exclude: vtkSMPThreadLocal.h
