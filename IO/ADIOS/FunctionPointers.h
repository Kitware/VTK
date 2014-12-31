/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FunctionPoilters.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __FunctionPointers_h_
#define __FunctionPointers_h_

// Arbitrary functors
class BaseFunctor
{
public:
  virtual ~BaseFunctor() { }
  virtual void ExecVoid() = 0;
};

template <typename TReturn>
class Functor0Args : public BaseFunctor
{
public:
  virtual ~Functor0Args() { }
  virtual void ExecVoid()
  {
    (*this)();
  }

  virtual TReturn operator() () = 0;
};

template<typename TObject, typename TReturn>
class MemberFunction0Args : public Functor0Args<TReturn>
{
public:
  typedef TReturn (TObject::*TFunctor)(void);
  MemberFunction0Args(TObject *instance, TFunctor functionPtr)
  : Instance(instance), FunctionPtr(functionPtr)
  { }

  virtual ~MemberFunction0Args() { }

  virtual TReturn operator() ()
  { return (this->Instance->*this->FunctionPtr)(); }

private:
  TObject *Instance;
  TFunctor FunctionPtr;
};

template<typename TObject, typename TReturn, typename TArg1>
class MemberFunction1Arg : public Functor0Args<TReturn>
{
public:
  typedef TReturn (TObject::*TFunctor)(TArg1);
  MemberFunction1Arg(TObject *instance, TFunctor functionPtr, TArg1 arg1)
  : Instance(instance), FunctionPtr(functionPtr), Arg1(arg1)
  { }

  virtual ~MemberFunction1Arg() { }

  virtual TReturn operator() ()
  { return (this->Instance->*this->FunctionPtr)(this->Arg1); }

private:
  TObject *Instance;
  TFunctor FunctionPtr;
  TArg1 Arg1;
};

template<typename TObject, typename TReturn, typename TArg1, typename TArg2>
class MemberFunction2Args : public Functor0Args<TReturn>
{
public:
  typedef TReturn (TObject::*TFunctor)(TArg1, TArg2);
  MemberFunction2Args(TObject *instance, TFunctor functionPtr,
    TArg1 arg1, TArg2 arg2)
  : Instance(instance), FunctionPtr(functionPtr), Arg1(arg1), Arg2(arg2)
  { }

  virtual ~MemberFunction2Args() { }

  virtual TReturn operator() ()
  { return (this->Instance->*this->FunctionPtr)(this->Arg1, this->Arg2); }

private:
  TObject *Instance;
  TFunctor FunctionPtr;
  TArg1 Arg1;
  TArg2 Arg2;
};

template<typename TObject, typename TReturn, typename TArg1, typename TArg2,
   typename TArg3>
class MemberFunction3Args : public Functor0Args<TReturn>
{
public:
  typedef TReturn (TObject::*TFunctor)(TArg1, TArg2, TArg3);
  MemberFunction3Args(TObject *instance, TFunctor functionPtr,
    TArg1 arg1, TArg2 arg2, TArg3 arg3)
  : Instance(instance), FunctionPtr(functionPtr), Arg1(arg1), Arg2(arg2),
    Arg3(arg3)
  { }

  virtual ~MemberFunction3Args() { }

  virtual TReturn operator() ()
  {
    return (this->Instance->*this->FunctionPtr)
      (this->Arg1, this->Arg2, this->Arg3);
  }

private:
  TObject *Instance;
  TFunctor FunctionPtr;
  TArg1 Arg1;
  TArg2 Arg2;
  TArg3 Arg3;
};

#endif
// VTK-HeaderTest-Exclude: FunctionPointers.h
