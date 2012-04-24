/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatlabEngineInterface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkObjectFactory.h"
#include "vtkMatlabEngineInterface.h"
#include "vtkMatlabMexAdapter.h"
#include "vtkErrorCode.h"
#include "vtkDataArray.h"
#include "vtkArray.h"
#include "engine.h"

#include <stdlib.h>
#include <string>
#include <vtksys/ios/sstream>
#include <sys/stat.h>
#include <string>

vtkStandardNewMacro(vtkMatlabEngineInterface);

class vtkMatlabEngineSingletonDestroyer;

class vtkMatlabEngineSingleton
{
public:
  static vtkMatlabEngineSingleton* Instance();

  int EngineOpen()
  {
    if(this->ep)
      return(1);
    else
      return(0);
  };

  void OpenEngine()
  {
    if(this->ep)
      {
      this->refcount++;
      return;
      }

    if(!(this->ep = engOpen("\0")))
      {
      cerr << endl << "Can't start MATLAB engine" << endl;
      return;
      }

    engSetVisible(this->ep, 0);
    this->refcount++;

  };

  void CloseEngine()
  {
    this->refcount--;

    if(this->refcount == 0)
      {
      if(engClose(this->ep))
        {
        cerr << "Can't shutdown MATLAB engine" << endl;
        }
      }
  };

  int EngEvalString(const char* string)
  {
    return(engEvalString(this->ep, string));
  };

  int EngPutVariable(const char *name, const mxArray *mxa)
  {
    return(engPutVariable(this->ep, name, mxa));
  };

  mxArray* EngGetVariable(const char *name)
  {
    return(engGetVariable(this->ep, name));
  };

  int EngSetVisible(bool value)
  {
    return(engSetVisible(this->ep, value));
  };

  int EngOutputBuffer(char* p, int n)
  {
    return(engOutputBuffer(this->ep, p, n));
  };

protected:

  friend class vtkMatlabEngineSingletonDestroyer;

  ~vtkMatlabEngineSingleton();

  vtkMatlabEngineSingleton();

  vtkMatlabEngineSingleton(const vtkMatlabEngineSingleton&);

  vtkMatlabEngineSingleton& operator=(const vtkMatlabEngineSingleton&);

private:

  int refcount;
  static vtkMatlabEngineSingleton* ins;
  static vtkMatlabEngineSingletonDestroyer destroyer;
  Engine* ep;

};

class vtkMatlabEngineSingletonDestroyer {
public:
  vtkMatlabEngineSingletonDestroyer();
  ~vtkMatlabEngineSingletonDestroyer();

  void SetSingleton(vtkMatlabEngineSingleton* s);
private:
  vtkMatlabEngineSingleton* _singleton;
};

vtkMatlabEngineSingleton* vtkMatlabEngineSingleton::ins = 0;

vtkMatlabEngineSingletonDestroyer vtkMatlabEngineSingleton::destroyer;

vtkMatlabEngineSingletonDestroyer::vtkMatlabEngineSingletonDestroyer () {
  _singleton = 0;
}

vtkMatlabEngineSingletonDestroyer::~vtkMatlabEngineSingletonDestroyer () {
  delete _singleton;
}

void vtkMatlabEngineSingletonDestroyer::SetSingleton (vtkMatlabEngineSingleton* s) {
  _singleton = s;
}



vtkMatlabEngineSingleton* vtkMatlabEngineSingleton::Instance()
{

  if(ins == 0)
    {
    ins = new vtkMatlabEngineSingleton;
    destroyer.SetSingleton(ins);
    }

  ins->OpenEngine();
  return(ins);

}


vtkMatlabEngineSingleton::~vtkMatlabEngineSingleton()
{


}

vtkMatlabEngineSingleton::vtkMatlabEngineSingleton()
{

  this->refcount = 0;
  this->ep = 0;

}

vtkMatlabEngineInterface::vtkMatlabEngineInterface()
{

  this->meng = vtkMatlabEngineSingleton::Instance();
  this->vmma = vtkMatlabMexAdapter::New();

}

vtkMatlabEngineInterface::~vtkMatlabEngineInterface()
{

  this->meng->CloseEngine();
  this->vmma->Delete();

}

void vtkMatlabEngineInterface::PrintSelf(ostream& os, vtkIndent indent)
{

  this->Superclass::PrintSelf(os,indent);

  if(this->vmma)
    {
    this->vmma->PrintSelf(os, indent);
    }


}

int vtkMatlabEngineInterface::EvalString(const char* string)
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute EvalString() command" << endl;
    return(1);
    }

  return(this->meng->EngEvalString(string));

}

int vtkMatlabEngineInterface::PutVtkDataArray(const char* name, vtkDataArray* vda)
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute PutVtkDataArray() command" << endl;
    return(1);
    }

  mxArray* mxa = this->vmma->vtkDataArrayToMxArray(vda);

  int rc = this->meng->EngPutVariable(name, mxa);

  mxDestroyArray(mxa);

  return(rc);

}

vtkDataArray* vtkMatlabEngineInterface::GetVtkDataArray(const char* name)
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute GetVtkDataArray() command" << endl;
    return(0);
    }

  mxArray* mxa = meng->EngGetVariable(name);

  vtkDataArray* vda = this->vmma->mxArrayTovtkDataArray(mxa);

  mxDestroyArray(mxa);

  return(vda);

}


int vtkMatlabEngineInterface::PutVtkArray(const char* name, vtkArray* vda)
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute PutVtkArray() command" << endl;
    return(1);
    }

  mxArray* mxa = this->vmma->vtkArrayToMxArray(vda);

  int rc = this->meng->EngPutVariable(name, mxa);

  mxDestroyArray(mxa);

  return(rc);

}


vtkArray* vtkMatlabEngineInterface::GetVtkArray(const char* name)
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute GetVtkArray() command" << endl;
    return(0);
    }

  mxArray* mxa = meng->EngGetVariable(name);

  vtkArray* vda = this->vmma->mxArrayTovtkArray(mxa);

  mxDestroyArray(mxa);

  return(vda);

}

int vtkMatlabEngineInterface::OutputBuffer(char* p, int n)
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute OuputBuffer() command" << endl;
    return(1);
    }

  return(this->meng->EngOutputBuffer(p, n));

}

int vtkMatlabEngineInterface::SetVisibleOn()
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute SetVisibleOn() command" << endl;
    return(1);
    }

  return(this->meng->EngSetVisible(1));

}

int vtkMatlabEngineInterface::SetVisibleOff()
{

  if(!this->meng->EngineOpen())
    {
    cerr << "Matlab engine not open, cannot execute SetVisibleOff() command" << endl;
    return(1);
    }

  return(this->meng->EngSetVisible(0));

}

int vtkMatlabEngineInterface::EngineOpen()
{

  return(this->meng->EngineOpen());

}
