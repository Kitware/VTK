/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAmoebaMinimizer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAmoebaMinimizer.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkAmoebaMinimizer, "1.4");
vtkStandardNewMacro(vtkAmoebaMinimizer);

//----------------------------------------------------------------------------
vtkAmoebaMinimizer::vtkAmoebaMinimizer()
{
  this->Function = NULL;
  this->FunctionArg = NULL;
  this->FunctionArgDelete = NULL;

  this->NumberOfParameters = 0;
  this->ParameterNames = NULL;
  this->Parameters = NULL;
  this->ParameterBrackets = NULL;

  this->Result = 0.0;

  this->Tolerance = 1e-4;
  this->MaxIterations = 1000;
  this->Iterations = 0;

  // specific to the amoeba
  this->AmoebaVertices = NULL;
  this->AmoebaValues = NULL;
  this->AmoebaSum = NULL;
  this->AmoebaNStepsNoImprovement = 0;
}
  
//----------------------------------------------------------------------------
vtkAmoebaMinimizer::~vtkAmoebaMinimizer()
{
  this->TerminateAmoeba();

  if ((this->FunctionArg) && (this->FunctionArgDelete)) 
    {
    (*this->FunctionArgDelete)(this->FunctionArg);
    }
  this->FunctionArg = NULL;
  this->FunctionArgDelete = NULL;
  this->Function = NULL;

  if (this->ParameterNames)
    {
    for (int i = 0; i < this->NumberOfParameters; i++)
      {
      if (this->ParameterNames[i])
        {
        delete [] this->ParameterNames[i];
        }
      }
    delete [] this->ParameterNames;
    this->ParameterNames = NULL;
    }
  if (this->Parameters)
    {
    delete [] this->Parameters;
    this->Parameters = NULL;
    }
  if (this->ParameterBrackets)
    {
    delete [] this->ParameterBrackets;
    this->ParameterBrackets = NULL;
    }

  this->NumberOfParameters = 0;
} 

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  os << indent << "NumberOfParameters: " << this->GetNumberOfParameters() << "\n";
  if (this->NumberOfParameters > 0)
    {
    int i;

    os << indent << "ParameterBrackets: \n";
    for (i = 0; i < this->NumberOfParameters; i++)
      {
      const char *name = this->GetParameterName(i);
      os << indent << "  ";
      if (name)
        {
        os << name << ": ";
        }
      else
        {
        os << i << ": ";
        }
      os << this->GetParameterBracket(i)[0] << " " <<
           this->GetParameterBracket(i)[1] <<"\n";
      }
    os << indent << "ParameterValues: \n";
    for (i = 0; i < this->NumberOfParameters; i++)
      {
      const char *name = this->GetParameterName(i);
      os << indent << "  ";
      if (name)
        {
        os << name << ": ";
        }
      else
        {
        os << i << ": ";
        }
      os << this->GetParameterValue(i) << "\n";
      }
    }
  os << indent << "Result: " << this->GetResult() << "\n";
  os << indent << "MaxIterations: " << this->GetMaxIterations() << "\n";
  os << indent << "Iterations: " << this->GetIterations() << "\n";
  os << indent << "Tolerance: " << this->GetTolerance() << "\n";
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetFunction(void (*f)(void *), void *arg)
{
  if ( f != this->Function || arg != this->FunctionArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->FunctionArg) && (this->FunctionArgDelete))
      {
      (*this->FunctionArgDelete)(this->FunctionArg);
      }
    this->Function = f;
    this->FunctionArg = arg;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetFunctionArgDelete(void (*f)(void *))
{
  if ( f != this->FunctionArgDelete)
    {
    this->FunctionArgDelete = f;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double *vtkAmoebaMinimizer::GetParameterBracket(const char *name)
{
  static double errval[2] = { 0.0, 0.0 };

  for (int i = 0; i < this->NumberOfParameters; i++)
    {
    if (this->ParameterNames[i] && strcmp(name,this->ParameterNames[i]) == 0)
      {
      return this->ParameterBrackets[i];
      }
    }

  vtkErrorMacro("GetParameterBracket: no parameter named " << name);
  return errval;
}

//----------------------------------------------------------------------------
double vtkAmoebaMinimizer::GetParameterValue(const char *name)
{
  for (int i = 0; i < this->NumberOfParameters; i++)
    {
    if (this->ParameterNames[i] && strcmp(name,this->ParameterNames[i]) == 0)
      {
      return this->Parameters[i];
      }
    }
  vtkErrorMacro("GetParameterValue: no parameter named " << name);
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetParameterBracket(const char *name, 
                                             double bmin, double bmax)
{
  int i;

  for (i = 0; i < this->NumberOfParameters; i++)
    {
    if (this->ParameterNames[i] && strcmp(name,this->ParameterNames[i]) == 0)
      {
      break;
      }
    }

  this->SetParameterBracket(i,bmin,bmax);

  if (!this->ParameterNames[i])
    {
    char *cp = new char[strlen(name)+8];
    strcpy(cp,name);
    this->ParameterNames[i] = cp;
    }
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetParameterBracket(int i, 
                                             double bmin, double bmax)
{
  if (i < this->NumberOfParameters)
    {
    if (this->ParameterBrackets[i][0] != bmin ||
        this->ParameterBrackets[i][1] != bmax)
      {
      this->ParameterBrackets[i][0] = bmin;
      this->ParameterBrackets[i][1] = bmax;
      this->Modified();
      }
    return;
    }

  int n = i + 1;
  char **newParameterNames = new char *[n];
  double *newParameters = new double[n];
  double (*newParameterBrackets)[2] = new double[n][2];

  for (int j = 0; j < this->NumberOfParameters; j++)
    {
    newParameterNames[j] = this->ParameterNames[j];
    this->ParameterNames[j] = NULL; // or else it will be deleted in Initialize
    newParameters[j] = this->Parameters[j];
    newParameterBrackets[j][0] = this->ParameterBrackets[j][0];
    newParameterBrackets[j][1] = this->ParameterBrackets[j][1];
    }

  newParameterNames[n-1] = 0;
  newParameters[n-1] = bmin;
  newParameterBrackets[n-1][0] = bmin;
  newParameterBrackets[n-1][1] = bmax;

  this->Initialize();

  this->NumberOfParameters = n;
  this->ParameterNames = newParameterNames;
  this->Parameters = newParameters;
  this->ParameterBrackets = newParameterBrackets;
}

//----------------------------------------------------------------------------
// reset the number of parameters to zero
void vtkAmoebaMinimizer::Initialize()
{
  if (this->ParameterNames)
    {
    for (int i = 0; i < this->NumberOfParameters; i++)
      {
      if (this->ParameterNames[i])
        {
        delete [] this->ParameterNames[i];
        }
      }
    delete [] this->ParameterNames;
    this->ParameterNames = 0;
    }
  if (this->Parameters)
    {
    delete [] this->Parameters;
    this->Parameters = 0;
    }
  if (this->ParameterBrackets)
    {
    delete [] this->ParameterBrackets;
    this->ParameterBrackets = 0;
    }

  this->NumberOfParameters = 0;
  this->Iterations = 0;

  this->Modified();
}

//----------------------------------------------------------------------------
int vtkAmoebaMinimizer::Iterate()
{
  if (this->Iterations == 0)
    {
    if (!this->Function)
      {
      vtkErrorMacro("Iterate: Function is NULL");
      return 0;
      }
    this->InitializeAmoeba();
    }

  int improved = this->PerformAmoeba();
  this->GetAmoebaParameters();
  this->Iterations++;

  return improved;
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::Minimize()
{
  if (this->Iterations == 0)
    {
    if (!this->Function)
      {
      vtkErrorMacro("Minimize: Function is NULL");
      return;
      }
    this->InitializeAmoeba();
    }

  for (; this->Iterations < this->MaxIterations; this->Iterations++)
    {
    int improved = this->PerformAmoeba();
    if (!improved)
      {
      break;
      }
    }

  this->GetAmoebaParameters();
}

/* ----------------------------------------------------------------------------
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */

/* ------------------------------------------------
  This code has been modified from the original.  Several macros
  have been expanded, functions have been renamed to match VTK
  conventions, and the formatting has been changed.
*/

#define  VTK_AMOEBA_FLIP_RATIO      1.0
#define  VTK_AMOEBA_CONTRACT_RATIO  0.5
#define  VTK_AMOEBA_STRETCH_RATIO   2.0

/* ----------------------------- MNI Header -----------------------------------
@NAME       : vtkAmoebaNumericallyClose
@INPUT      : n1
              n2
              threshold_ratio
@OUTPUT     :
@RETURNS    : true if the numbers are within the threshold ratio
@DESCRIPTION: Decides if two numbers are close to each other.
@METHOD     :
@GLOBALS    :
@CALLS      :
@CREATED    :         1993    David MacDonald
@MODIFIED   :         2002    David Gobbi
---------------------------------------------------------------------------- */

#define  VTK_AMOEBA_SMALLEST  1.0e-20

static  int  vtkAmoebaNumericallyClose(double  n1,
                                       double  n2,
                                       double  threshold_ratio )
{
  double  avg, diff, abs_n1, abs_n2;

  diff = n1 - n2;
  if( diff < 0.0 )
    {
    diff = -diff;
    }

  abs_n1 = (n1 < 0.0 ? -n1 : n1);
  abs_n2 = (n2 < 0.0 ? -n2 : n2);

  if( abs_n1 < VTK_AMOEBA_SMALLEST || abs_n2 < VTK_AMOEBA_SMALLEST )
    {
    return( abs_n1 < threshold_ratio && abs_n2 < threshold_ratio );
    }

  avg = (n1 + n2) / 2.0;

  if( avg == 0.0 )
    {
    return( diff <= threshold_ratio );
    }

  if( avg < 0.0 )
    {
    avg = -avg;
    }

  return( (diff / avg) <= threshold_ratio );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : InitializeAmoeba
@INPUT      : 
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Initializes the amoeba structure to minimize the function.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   :         2002    David Gobbi
---------------------------------------------------------------------------- */

void  vtkAmoebaMinimizer::InitializeAmoeba()
{
  int    i, j;

  this->TerminateAmoeba();

  int n_parameters = this->NumberOfParameters;
  this->AmoebaNStepsNoImprovement = 0;
  this->AmoebaVertices = new double *[n_parameters+1];
  this->AmoebaVertices[0] = new double[n_parameters*(n_parameters+1)];

  for( i = 1 ; i < n_parameters+1 ; i++)
    {
    this->AmoebaVertices[i] = this->AmoebaVertices[i-1] + n_parameters;
    }
  
  this->AmoebaValues = new double[n_parameters+1];

  this->AmoebaSum = new double[n_parameters];

  for (j = 0; j < n_parameters; j++)
    {
    this->AmoebaSum[j] = 0.0;
    }

  for( i = 0 ; i < n_parameters+1 ; i++ )
    {
    for( j = 0; j < n_parameters ; j++ )
      {
      this->AmoebaVertices[i][j] = this->ParameterBrackets[j][0];
      if( i > 0 && j == i - 1 )
        {
        this->AmoebaVertices[i][j] = this->ParameterBrackets[j][1];
        }
      this->Parameters[j] = this->AmoebaVertices[i][j];
      this->AmoebaSum[j] += this->Parameters[j];
      }

    this->Function(this->FunctionArg);
    this->AmoebaValues[i] = this->Result;
    }

  for ( j = 0 ; j < n_parameters ; j++ )
    {
    this->Parameters[j] = this->AmoebaVertices[0][j];
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : GetAmoebaParameters
@INPUT      : 
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Passes back the current position of the amoeba (best value),
              and returns the function value at that point.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   :         2002    David Gobbi
---------------------------------------------------------------------------- */

void vtkAmoebaMinimizer::GetAmoebaParameters()
{
  int   i, j, low;

  low = 0;
  for( i = 1 ; i < this->NumberOfParameters+1 ; i++ )
    {
    if( this->AmoebaValues[i] < this->AmoebaValues[low] )
      {
      low = i;
      }
    }

  for( j = 0 ; j < this->NumberOfParameters ; j++ )
    {
    this->Parameters[j] = this->AmoebaVertices[low][j];
    }

  this->Result = this->AmoebaValues[low];
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : TerminateAmoeba
@INPUT      : 
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Frees the amoeba.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   :         2002    David Gobbi
---------------------------------------------------------------------------- */

void  vtkAmoebaMinimizer::TerminateAmoeba()
{
  if (this->AmoebaVertices)
    {
    delete [] this->AmoebaVertices[0];
    delete [] this->AmoebaVertices;
    this->AmoebaVertices = NULL;
    }
  if (this->AmoebaValues)
    {
    delete [] this->AmoebaValues;
    this->AmoebaValues = NULL;
    }
  if (this->AmoebaSum)
    {
    delete [] this->AmoebaSum;
    this->AmoebaSum = NULL;
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : TryAmoeba
@INPUT      : sum
              high
              fac
@OUTPUT     : 
@RETURNS    : value
@DESCRIPTION: Does a modification to the high vertex of the amoeba and
              returns the value of the new point.  If the new point is
              better (smaller value), it replaces the high vertex of the
              amoeba.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   :         2002    David Gobbi
---------------------------------------------------------------------------- */

double  vtkAmoebaMinimizer::TryAmoeba(double  sum[],
                                      int     high,
                                      double  fac )
{
  int    j;
  double y_try, fac1, fac2;
  double  *parameters;

  parameters = this->Parameters;

  fac1 = (1.0 - fac) / this->NumberOfParameters;
  fac2 = fac - fac1;

  for( j = 0 ; j < this->NumberOfParameters ; j++ )
    {
    parameters[j] = (sum[j] * fac1 + this->AmoebaVertices[high][j] * fac2);
    }

  this->Function(this->FunctionArg);
  y_try = this->Result;

  if( y_try < this->AmoebaValues[high] )
    {
    this->AmoebaValues[high] = y_try;
    for( j = 0 ; j < this->NumberOfParameters ; j++ )
      {
      sum[j] += parameters[j] - this->AmoebaVertices[high][j];
      this->AmoebaVertices[high][j] = parameters[j];
      }
    }

  return( y_try );
}

#define  N_STEPS_NO_IMPROVEMENT  20

/* ----------------------------- MNI Header -----------------------------------
@NAME       : PerformAmoeba
@INPUT      : 
@OUTPUT     : 
@RETURNS    : true if numerically significant improvement
@DESCRIPTION: Performs one iteration of an amoeba, returning true if a
              numerically significant improvement has been found recently.
              Even if it returns 0, you can keep calling this function,
              since it may be contracting with no improvement, but will 
              eventually shrink small enough to get an improvment.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   :         2002    David Gobbi
---------------------------------------------------------------------------- */

int vtkAmoebaMinimizer::PerformAmoeba()
{
  int      i, j, low, high, next_high;
  double   y_try, y_save;
  int      improvement_found;

  improvement_found = 1;

  if( this->AmoebaValues[0] > this->AmoebaValues[1] )
    {
    high = 0;
    next_high = 1;
    }
  else
    {
    high = 1;
    next_high = 0;
    }

  low = next_high;

  for( i = 2 ; i < this->NumberOfParameters+1 ; i++ )
    {
    if( this->AmoebaValues[i] < this->AmoebaValues[low] )
      {
      low = i;
      }
    else if( this->AmoebaValues[i] > this->AmoebaValues[high] )
      {
      next_high = high;
      high = i;
      }
    else if( this->AmoebaValues[i] > this->AmoebaValues[next_high] )
      {
      next_high = i;
      }
    }

  if( vtkAmoebaNumericallyClose( this->AmoebaValues[low], 
                                 this->AmoebaValues[high],
                                 this->Tolerance ) )
    {
    ++this->AmoebaNStepsNoImprovement;
    if( this->AmoebaNStepsNoImprovement >= N_STEPS_NO_IMPROVEMENT )
      {
      improvement_found = 0;
      }
    }
  else
    {
    this->AmoebaNStepsNoImprovement = 0;
    }

  y_try = this->TryAmoeba( this->AmoebaSum, high, -VTK_AMOEBA_FLIP_RATIO );

  if( y_try <= this->AmoebaValues[low] )
    {
    y_try = TryAmoeba( this->AmoebaSum, high, VTK_AMOEBA_STRETCH_RATIO );
    }
  else if( y_try >= this->AmoebaValues[next_high] )
    {
    y_save = this->AmoebaValues[high];
    y_try = TryAmoeba( this->AmoebaSum, high, VTK_AMOEBA_CONTRACT_RATIO );

    if( y_try >= y_save )
      {
      for( i = 0 ; i < this->NumberOfParameters+1 ; i++)
        {
        if( i != low )
          {
          for( j = 0 ; j < this->NumberOfParameters ; j++ )
            {
            this->Parameters[j] = (this->AmoebaVertices[i][j] +
                                   this->AmoebaVertices[low][j]) / 2.0f;
            this->AmoebaVertices[i][j] = this->Parameters[j];
            }
          
          this->Function(this->FunctionArg);
          this->AmoebaValues[i] = this->Result;
          }
        }

      for( j = 0 ; j < this->NumberOfParameters ; j++ )
        {
        this->AmoebaSum[j] = 0.0;
        for( i = 0 ; i < this->NumberOfParameters+1 ; i++ )
          {
          this->AmoebaSum[j] += this->AmoebaVertices[i][j];
          }
        }
      }
    }

  return( improvement_found );
}

