/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAmoebaMinimizer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAmoebaMinimizer.h"
#include "vtkObjectFactory.h"

#define  N_STEPS_NO_VALUE_IMPROVEMENT  2
#define  N_STEPS_NO_PARAM_IMPROVEMENT  18

vtkStandardNewMacro(vtkAmoebaMinimizer);

//----------------------------------------------------------------------------
vtkAmoebaMinimizer::vtkAmoebaMinimizer()
{
  this->Function = NULL;
  this->FunctionArg = NULL;
  this->FunctionArgDelete = NULL;

  this->NumberOfParameters = 0;
  this->ParameterNames = NULL;
  this->ParameterValues = NULL;
  this->ParameterScales = NULL;

  this->FunctionValue = 0.0;

  this->ContractionRatio = 0.5;
  this->ExpansionRatio = 2.0;

  this->Tolerance = 1e-4;
  this->ParameterTolerance = 1e-4;
  this->MaxIterations = 1000;
  this->Iterations = 0;
  this->FunctionEvaluations = 0;

  // specific to the amoeba
  this->AmoebaVertices = NULL;
  this->AmoebaValues = NULL;
  this->AmoebaSum = NULL;
  this->AmoebaSize = 0;
  this->AmoebaHighValue = 0;
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
      delete [] this->ParameterNames[i];
    }
    delete [] this->ParameterNames;
    this->ParameterNames = NULL;
  }
  delete [] this->ParameterValues;
  this->ParameterValues = NULL;
  delete [] this->ParameterScales;
  this->ParameterScales = NULL;

  this->NumberOfParameters = 0;
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfParameters: " << this->GetNumberOfParameters() << "\n";
  if (this->NumberOfParameters > 0)
  {
    int i;

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

    os << indent << "ParameterScales: \n";
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
      os << this->GetParameterScale(i) << "\n";
    }
  }

  os << indent << "FunctionValue: " << this->GetFunctionValue() << "\n";
  os << indent << "FunctionEvaluations: " << this->GetFunctionEvaluations()
     << "\n";
  os << indent << "Iterations: " << this->GetIterations() << "\n";
  os << indent << "MaxIterations: " << this->GetMaxIterations() << "\n";
  os << indent << "Tolerance: " << this->GetTolerance() << "\n";
  os << indent << "ParameterTolerance: " << this->GetParameterTolerance() << "\n";
  os << indent << "ContractionRatio: " << this->GetContractionRatio() << "\n";
  os << indent << "ExpansionRatio: " << this->GetExpansionRatio() << "\n";
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
double vtkAmoebaMinimizer::GetParameterValue(const char *name)
{
  for (int i = 0; i < this->NumberOfParameters; i++)
  {
    if (this->ParameterNames[i] && strcmp(name,this->ParameterNames[i]) == 0)
    {
      return this->ParameterValues[i];
    }
  }
  vtkErrorMacro("GetParameterValue: no parameter named " << name);
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetParameterValue(const char *name, double val)
{
  int i;

  for (i = 0; i < this->NumberOfParameters; i++)
  {
    if (this->ParameterNames[i] && strcmp(name,this->ParameterNames[i]) == 0)
    {
      break;
    }
  }

  this->SetParameterValue(i, val);

  if (!this->ParameterNames[i])
  {
    char *cp = new char[strlen(name)+8];
    strcpy(cp,name);
    this->ParameterNames[i] = cp;
  }
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetParameterValue(int i, double val)
{
  if (i < this->NumberOfParameters)
  {
    if (this->ParameterValues[i] != val)
    {
      this->ParameterValues[i] = val;
      this->Iterations = 0; // reset to start
      this->FunctionEvaluations = 0;
      this->Modified();
    }
    return;
  }

  int n = this->NumberOfParameters + 1;

  char **newParameterNames = new char *[n];
  double *newParameterValues = new double[n];
  double *newParameterScales = new double[n];

  for (int j = 0; j < this->NumberOfParameters; j++)
  {
    newParameterNames[j] = this->ParameterNames[j];
    this->ParameterNames[j] = NULL; // or else it will be deleted in Initialize
    newParameterValues[j] = this->ParameterValues[j];
    newParameterScales[j] = this->ParameterScales[j];
  }

  newParameterNames[n-1] = 0;
  newParameterValues[n-1] = val;
  newParameterScales[n-1] = 1.0;

  this->Initialize();

  this->NumberOfParameters = n;
  this->ParameterNames = newParameterNames;
  this->ParameterValues = newParameterValues;
  this->ParameterScales = newParameterScales;

  this->Iterations = 0; // reset to start
  this->FunctionEvaluations = 0;
}

//----------------------------------------------------------------------------
double vtkAmoebaMinimizer::GetParameterScale(const char *name)
{
  for (int i = 0; i < this->NumberOfParameters; i++)
  {
    if (this->ParameterNames[i] && strcmp(name,this->ParameterNames[i]) == 0)
    {
      return this->ParameterScales[i];
    }
  }
  vtkErrorMacro("GetParameterScale: no parameter named " << name);
  return 1.0;
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetParameterScale(const char *name, double scale)
{
  for (int i = 0; i < this->NumberOfParameters; i++)
  {
    if (this->ParameterNames[i] && strcmp(name,this->ParameterNames[i]) == 0)
    {
      this->SetParameterScale(i, scale);
      return;
    }
  }
  vtkErrorMacro("SetParameterScale: no parameter named " << name);
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::SetParameterScale(int i, double scale)
{
  if (i < 0 || i > this->NumberOfParameters)
  {
    vtkErrorMacro("SetParameterScale: parameter number out of range: " << i);
    return;
  }

  if (this->ParameterScales[i] != scale)
  {
    this->ParameterScales[i] = scale;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// reset the number of parameters to zero
void vtkAmoebaMinimizer::Initialize()
{
  if (this->ParameterNames)
  {
    for (int i = 0; i < this->NumberOfParameters; i++)
    {
      delete [] this->ParameterNames[i];
    }
    delete [] this->ParameterNames;
    this->ParameterNames = 0;
  }
  delete [] this->ParameterValues;
  this->ParameterValues = 0;
  delete [] this->ParameterScales;
  this->ParameterScales = 0;

  this->NumberOfParameters = 0;
  this->Iterations = 0;
  this->FunctionEvaluations = 0;
  this->AmoebaSize = 0;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAmoebaMinimizer::EvaluateFunction()
{
  if (this->Function)
  {
    this->Function(this->FunctionArg);
  }
  this->FunctionEvaluations++;
}

//----------------------------------------------------------------------------
int vtkAmoebaMinimizer::CheckParameterTolerance()
{
  int n = this->NumberOfParameters;

  double *vertex0 = this->AmoebaVertices[0];
  double *scales = this->ParameterScales;
  double size = 0;

  for (int i = 1; i <= n; i++)
  {
    double *vertex = this->AmoebaVertices[i];
    for (int j = 0; j < n; j++)
    {
      double d = fabs((vertex[j] - vertex0[j])/scales[j]);
      size = ((d < size) ? size : d);
    }
  }

  if (size != this->AmoebaSize)
  {
    this->AmoebaNStepsNoImprovement = N_STEPS_NO_VALUE_IMPROVEMENT-1;
  }
  this->AmoebaSize = size;
  // if amoeba is static, only make a set number of tries
  if (this->AmoebaNStepsNoImprovement >
      (N_STEPS_NO_VALUE_IMPROVEMENT + N_STEPS_NO_PARAM_IMPROVEMENT))
  {
    return 1;
  }

  return (size <= this->ParameterTolerance);
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
  int paramsWithinTol = 0;
  if (!improved)
  {
    paramsWithinTol = this->CheckParameterTolerance();
  }
  this->GetAmoebaParameterValues();
  this->Iterations++;

  return (improved || !paramsWithinTol);
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
      if (this->CheckParameterTolerance())
      {
        break;
      }
    }
  }

  this->GetAmoebaParameterValues();
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
      this->AmoebaVertices[i][j] = this->ParameterValues[j];
      if( i > 0 && j == i - 1 )
      {
        this->AmoebaVertices[i][j] =
          this->ParameterValues[j] + this->ParameterScales[j];
      }
      this->AmoebaSum[j] += this->ParameterValues[j];
    }
  }
  for( i = 0 ; i < n_parameters+1 ; i++ )
  {
    for( j = 0; j < n_parameters; j++ )
    {
      this->ParameterValues[j] = this->AmoebaVertices[i][j];
    }
    this->EvaluateFunction();
    this->AmoebaValues[i] = this->FunctionValue;
  }

  for ( j = 0 ; j < n_parameters ; j++ )
  {
    this->ParameterValues[j] = this->AmoebaVertices[0][j];
  }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : GetAmoebaParameterValues
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

void vtkAmoebaMinimizer::GetAmoebaParameterValues()
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
    this->ParameterValues[j] = this->AmoebaVertices[low][j];
  }

  this->FunctionValue = this->AmoebaValues[low];
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
  delete [] this->AmoebaValues;
  this->AmoebaValues = NULL;
  delete [] this->AmoebaSum;
  this->AmoebaSum = NULL;
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

  parameters = this->ParameterValues;

  fac1 = (1.0 - fac) / this->NumberOfParameters;
  fac2 = fac - fac1;

  for( j = 0 ; j < this->NumberOfParameters ; j++ )
  {
    parameters[j] = (sum[j] * fac1 + this->AmoebaVertices[high][j] * fac2);
  }

  this->EvaluateFunction();
  y_try = this->FunctionValue;

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

  if( this->AmoebaValues[high] == this->AmoebaHighValue ||
      vtkAmoebaNumericallyClose( this->AmoebaValues[low],
                                 this->AmoebaValues[high],
                                 this->Tolerance ) )
  {
    ++this->AmoebaNStepsNoImprovement;
    if( this->AmoebaNStepsNoImprovement >= N_STEPS_NO_VALUE_IMPROVEMENT )
    {
      improvement_found = 0;
    }
  }
  else
  {
    this->AmoebaNStepsNoImprovement = 0;
  }

  this->AmoebaHighValue = this->AmoebaValues[high];

  y_try = this->TryAmoeba( this->AmoebaSum, high, -1.0 );

  if( y_try <= this->AmoebaValues[low] )
  {
    TryAmoeba( this->AmoebaSum, high, this->ExpansionRatio );
  }
  else if( y_try >= this->AmoebaValues[next_high] )
  {
    y_save = this->AmoebaValues[high];
    y_try = TryAmoeba( this->AmoebaSum, high, this->ContractionRatio );

    if( y_try >= y_save )
    {
      for( i = 0 ; i < this->NumberOfParameters+1 ; i++)
      {
        if( i != low )
        {
          for( j = 0 ; j < this->NumberOfParameters ; j++ )
          {
            this->ParameterValues[j] = (this->AmoebaVertices[i][j] +
                                        this->AmoebaVertices[low][j]) / 2.0;
            this->AmoebaVertices[i][j] = this->ParameterValues[j];
          }

          this->EvaluateFunction();
          this->AmoebaValues[i] = this->FunctionValue;
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

