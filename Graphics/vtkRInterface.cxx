/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRInterface.cxx

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

#include "vtkRInterface.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkRAdapter.h"

vtkCxxRevisionMacro(vtkRInterface, "1.1");
vtkStandardNewMacro(vtkRInterface);

#include "R.h"
#include "Rmath.h"
#include "Rembedded.h"
#include "Rversion.h"
#include "Rdefines.h"

#ifndef WIN32
#define R_INTERFACE_PTRS
#include "Rinterface.h"
#endif

#include "R_ext/Parse.h"

namespace {
vtkstd::string tmpFilePath;
}

class vtkImplementationRSingletonDestroyer;

class vtkImplementationRSingleton
{
public:
  static vtkImplementationRSingleton* Instance();

  void InitializeR()
  {

  if(this->Rinitialized)
    {
    this->refcount++;
    return;
    }

#ifndef WIN32
    R_SignalHandlers = 0;
#endif

#ifdef CSTACK_DEFNS
     R_CStackLimit = (uintptr_t)-1; 
#endif

    char *R_argv[]= {"vtkRInterface", "--gui=none", "--no-save", "--no-readline", "--silent"};
    Rf_initEmbeddedR(sizeof(R_argv)/sizeof(R_argv[0]), R_argv);

    this->Rinitialized = 1;
    this->refcount++;

  };

  void CloseR()
  {

  this->refcount--;

  if(this->refcount == 0)
    {
    R_CleanTempDir();
    Rf_endEmbeddedR(0);
    }

  };

protected:

  friend class vtkImplementationRSingletonDestroyer;

  ~vtkImplementationRSingleton();

  vtkImplementationRSingleton();

  vtkImplementationRSingleton(const vtkImplementationRSingleton&);

  vtkImplementationRSingleton& operator=(const vtkImplementationRSingleton&);

private:

  int refcount;
  int Rinitialized;
  static vtkImplementationRSingleton* ins;
  static vtkImplementationRSingletonDestroyer destroyer;

};

class vtkImplementationRSingletonDestroyer {
public:
  vtkImplementationRSingletonDestroyer();
  ~vtkImplementationRSingletonDestroyer();

  void SetSingleton(vtkImplementationRSingleton* s);
private:
  vtkImplementationRSingleton* _singleton;
};


vtkImplementationRSingleton* vtkImplementationRSingleton::ins = 0;

vtkImplementationRSingletonDestroyer vtkImplementationRSingleton::destroyer;

vtkImplementationRSingletonDestroyer::vtkImplementationRSingletonDestroyer () {
  _singleton = 0;
}

vtkImplementationRSingletonDestroyer::~vtkImplementationRSingletonDestroyer () {

  delete _singleton;
}

void vtkImplementationRSingletonDestroyer::SetSingleton (vtkImplementationRSingleton* s) {
  _singleton = s;
}


vtkImplementationRSingleton* vtkImplementationRSingleton::Instance()
{

  if(ins == 0)
    {
    ins = new vtkImplementationRSingleton;
    destroyer.SetSingleton(ins);
    }

  ins->InitializeR();
  return(ins);

}


vtkImplementationRSingleton::~vtkImplementationRSingleton()
{


}

vtkImplementationRSingleton::vtkImplementationRSingleton()
{

  this->refcount = 0;
  this->Rinitialized = 0;

}

//----------------------------------------------------------------------------
vtkRInterface::vtkRInterface()
{

  vtkstd::string  rcommand;
  this->rs = vtkImplementationRSingleton::Instance();
  rcommand.append("f<-file(paste(tempdir(), \"/Routput.txt\", sep = \"\"), open=\"wt+\")\n");
  tmpFilePath.clear();
  tmpFilePath.append(R_TempDir);
#ifdef WIN32
  tmpFilePath.append("\\Routput.txt");
#else
  tmpFilePath.append("/Routput.txt");
#endif
  this->EvalRscript(rcommand.c_str(), false);
  this->EvalRscript("sink(f)\n", false);
  this->buffer = 0;
  this->buffer_size = 0;

}

//----------------------------------------------------------------------------
vtkRInterface::~vtkRInterface()
{

  this->rs->CloseR();

}

int vtkRInterface::EvalRscript(const char *string, bool showRoutput)
{

  ParseStatus status;
  SEXP cmdSexp, cmdexpr = R_NilValue;
  SEXP ans;
  int i; 
  int error;

  PROTECT(cmdSexp = allocVector(STRSXP, 1));
  SET_STRING_ELT(cmdSexp, 0, mkChar(string));

  cmdexpr = PROTECT(R_ParseVector(cmdSexp, -1, &status, R_NilValue));
  switch (status)
    {
    case PARSE_OK:
      for(i = 0; i < length(cmdexpr); i++)
        {
        ans = R_tryEval(VECTOR_ELT(cmdexpr, i),NULL,&error);
        if (error)
          {
          return 1;
          }
        if(showRoutput)
          {
          PrintValue(ans);
          }
        }
      break;
    case PARSE_INCOMPLETE:
      vtkErrorMacro(<<"R parse status is PARSE_INCOMPLETE");
      /* need to read another line */
      break;
    case PARSE_NULL:
      vtkErrorMacro(<<"R parse status is PARSE_NULL");
      return 1;
      break;
    case PARSE_ERROR:
      vtkErrorMacro(<<"R parse status is PARSE_ERROR");
      return 1;
      break;
    case PARSE_EOF:
      vtkErrorMacro(<<"R parse status is PARSE_EOF");
      break;
    default:
      vtkErrorMacro(<<"R parse status is NOT DOCUMENTED");
      return 1;
      break;
    }
  UNPROTECT(2);
  this->FillOutputBuffer();
  return 0;

}

int vtkRInterface::EvalRcommand(const char *funcName, int param)
{

  SEXP e;
  SEXP arg;
  int errorOccurred;

  PROTECT(arg = allocVector(INTSXP, 1));
  INTEGER(arg)[0]  = param;
  PROTECT(e = lang2(install(funcName), arg));

  R_tryEval(e, R_GlobalEnv, &errorOccurred);

  UNPROTECT(2);
  return(errorOccurred);

}

void vtkRInterface::AssignVTKDataArrayToRVariable(vtkDataArray* da, const char* RVariableName)
{

  SEXP s;
  s = vtkRAdapter::VTKDataArrayToR(da);
  defineVar(install(RVariableName), s, R_GlobalEnv);

}

void vtkRInterface::AssignVTKArrayToRVariable(vtkArray* da, const char* RVariableName)
{

  SEXP s;
  s = vtkRAdapter::VTKArrayToR(da);
  defineVar(install(RVariableName), s, R_GlobalEnv);

}

vtkDataArray* vtkRInterface::AssignRVariableToVTKDataArray(const char* RVariableName)
{

  SEXP s;

  s = findVar(install(RVariableName), R_GlobalEnv);

  if(s != R_UnboundValue)
    return(vtkRAdapter::RToVTKDataArray(s));
  else
    return(0);

}

vtkArray* vtkRInterface::AssignRVariableToVTKArray(const char* RVariableName)
{

  SEXP s;

  s = findVar(install(RVariableName), R_GlobalEnv);

  if(s != R_UnboundValue)
    return(vtkRAdapter::RToVTKArray(s));
  else
    return(0);

}

vtkTable* vtkRInterface::AssignRVariableToVTKTable(const char* RVariableName)
{

  SEXP s;

  s = findVar(install(RVariableName), R_GlobalEnv);

  if(s != R_UnboundValue)
    return(vtkRAdapter::RToVTKTable(s));
  else
    return(0);

}

void vtkRInterface::AssignVTKTableToRVariable(vtkTable* table, const char* RVariableName)
{

  SEXP s;
  s = vtkRAdapter::VTKTableToR(table);
  defineVar(install(RVariableName), s, R_GlobalEnv);

}

int vtkRInterface::OutputBuffer(char* p, int n)
{

  this->buffer = p;
  this->buffer_size = n;
  if(this->buffer && (this->buffer_size > 0) )
    {
    this->buffer[0] = '\0';
    }
  return(1);

}

int vtkRInterface::FillOutputBuffer()
{

  FILE *fp;
  long len;

  if(this->buffer && (this->buffer_size > 0) )
    {
    fp = fopen(tmpFilePath.c_str(),"rb");

    if(!fp)
      {
      vtkErrorMacro(<<"Can't open input file named " << tmpFilePath.c_str());
      return(0);
      }

    fseek(fp,0,SEEK_END);
    len = ftell(fp);
    
    if(len == 0)
      {
      return(1);
      }
      
    if(len >= this->buffer_size)
      {
      fseek(fp,len-this->buffer_size+1,SEEK_SET);
      fread(this->buffer,1,this->buffer_size-1,fp);
      this->buffer[this->buffer_size-1] = '\0';
      }
    else
      {
      fseek(fp,0,SEEK_SET);
      fread(this->buffer,1,len,fp);
      this->buffer[len] = '\0';
      }
    
    
    fclose(fp);

    return(1);

    }
  else
    {
    return(0);
    }

}

void vtkRInterface::PrintSelf(ostream& os, vtkIndent indent)
{

  this->Superclass::PrintSelf(os, indent);

}

