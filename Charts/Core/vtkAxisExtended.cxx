/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxisExtended.h"

#include "vtkMath.h" // for VTK_DBL_EPSILON
#include "vtkStdString.h"
#include "vtkObjectFactory.h"

#include <sstream>

#include <cmath>
#include <algorithm>


vtkStandardNewMacro(vtkAxisExtended);

vtkAxisExtended::vtkAxisExtended()
{
  this->FontSize = 0;
  this->DesiredFontSize = 10;
  this->Precision = 3;
  this->LabelFormat = 0;
  this->Orientation = 0;
  this->LabelLegibilityChanged = true;
  this->IsAxisVertical = false;
}

vtkAxisExtended::~vtkAxisExtended()
{
}

// This method return a value to make step sizes corresponding to low q and j values more preferable
double vtkAxisExtended::Simplicity(int qIndex, int qLength, int j, double lmin,
                                   double lmax, double lstep)
{
  double eps = VTK_DBL_EPSILON * 100;
  int v = 1 ;
  ++qIndex;

  double rem = fmod(lmin,lstep);
  if((rem < eps || (lstep - rem ) < eps ) &&  lmin <= 0 && lmax >= 0)
  {
    v = 0;
  }
  else
  {
    v = 1;  // v is 1 is lebelling includes zero
  }

  return 1.0 - (qIndex - 1.0) / (qLength - 1.0) - j + v;
}

// This method returns the maximum possible value of simplicity value given q
// and j
double vtkAxisExtended::SimplicityMax(int qIndex, int qLength, int j)
{
  int v = 1;
  ++qIndex;
  return 1.0 - (qIndex - 1.0) / (qLength - 1.0) - j + v;
}

// This method makes the data range approximately same as the labeling range
// more preferable
double vtkAxisExtended::Coverage(double dmin, double dmax, double lmin,
                                 double lmax)
{
  double coverage = 1.0 - 0.5 * (pow(dmax - lmax, 2) + pow(dmin - lmin, 2)
                                 / pow(0.1 * (dmax - dmin), 2));
  return coverage;
}


//This gives the maximum possible value of coverage given the step size
double vtkAxisExtended::CoverageMax(double dmin, double dmax, double span)
{
  double range = dmax - dmin;
  if (span > range)
  {
    double half = (span - range)/2;
    return 1- 0.5 * (pow(half, 2) + pow(half, 2) / pow(0.1*(range),2));
  }
  else
  {
    return 1.0;
  }
}

// This method return a value to make the density of the labels close to the
// user given value
double vtkAxisExtended::Density(int k, double m, double dmin, double dmax,
                                double lmin, double lmax)
{
  double r = (k-1)/(lmax-lmin);
  double rt = (m-1) / (std::max(lmax,dmax) - std::min(dmin,lmin));

  return 2 - std::max(r/rt, rt/r);
}

// Derives the maximum values for density given k (number of ticks) and m
// (user given)
double vtkAxisExtended::DensityMax(int k, double m)
{
  if(k >= m)
  {
    return 2 - (k-1) / (m-1);
  }
  else
  {
    return 1;
  }
}

// This methods gives a weighing factor for each label depending on the range
// The coding for the different formats
//   1 - Scientific 5 * 10^6
//   2 - Decimal e.g. 5000
//   3 - K e.g. 5K
//   4 - Factored K e.g. 5(K)
//   5 - M e.g. 5M
//   6 - Factored M e.g. 5(M)
//   7 - Factored Decimals e.g. 5 (thousands)
//   8 - Factored Scientific 5 (10^6)
double vtkAxisExtended::FormatLegibilityScore(double n, int format)
{
  switch(format)
  {
    case 1:
      return 0.25;
    case 2:
      if(std::abs(n) > 0.0001 && std::abs(n) < 1000000)
      {
        return 1.0;
      }
      else
      {
        return 0.0;
      }
    case 3:
      if(std::abs(n) > 1000 &&  std::abs(n) < 1000000)
      {
        return 0.75;
      }
      else
      {
        return 0.0;
      }
    case 4:
      if(std::abs(n) > 1000 && std::abs(n) < 1000000)
      {
        return 0.4;
      }
      else
      {
        return 0.0;
      }
    case 5:
      if(std::abs(n) > 1000000 && std::abs(n) < 1000000000)
      {
        return 0.75;
      }
      else
      {
        return 0.0;
      }
    case 6:
      if(std::abs(n) > 1000000 && std::abs(n) < 1000000000)
      {
        return 0.4;
      }
      else
      {
        return 0.0;
      }
    case 7:
      return 0.5;
    case 8:
      return 0.3;
    default:
      return 0.0;
  }
}


// This method returns the length of the label given the format
int vtkAxisExtended::FormatStringLength(int format, double n, int precision)
{
  std::ostringstream ostr;
  ostr.imbue(std::locale::classic());
  int numSize(0);

  switch(format)
  {
    case 1:
      ostr.precision(precision);
      ostr.setf(std::ios::scientific, std::ios::floatfield);
      ostr<<n;
      numSize = (int) ostr.str().length();
      return numSize;
    case 2:
      ostr << n;
      if((std::ceil(n)-std::floor(n)) != 0.0 )
      {
        ostr.precision(precision);
      }
      // Gets the length of the string with the current format without the end
      // character
      numSize = (int) ostr.str().length()-1;
      return numSize;
    case 3:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000;
      if((std::ceil(n/1000.0)-std::floor(n/1000.0)) != 0.0 )
      {
        ostr.precision(precision);
      }
      numSize = (int) ostr.str().length()-1;
      return numSize+1; // minus three zeros + K
    case 4:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000;
      if((std::ceil(n/1000.0)-std::floor(n/1000.0)) != 0.0)
      {
        ostr.precision(precision);
      }
      numSize = static_cast<int>(ostr.str().length() - 1);
      return numSize; // minus three zeros
    case 5:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000000;
      if((std::ceil(n/1000000.0) - std::floor(n/1000000.0)) != 0.0)
      {
        ostr.precision(precision);
      }
      numSize = (int) ostr.str().length()-1;
      return numSize; // minus six zeros
    case 6:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000000;
      if((std::ceil(n/1000000.0)-std::floor(n/1000000.0)) != 0.0 )
      {
        ostr.precision(precision);
      }
      numSize = (int) ostr.str().length()-1;
      return numSize+1; // minus six zeros + M
    case 7:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000;
      if((std::ceil(n/1000.0)-std::floor(n/1000.0)) != 0.0 )
      {
        ostr.precision(precision);
      }
      numSize = (int) ostr.str().length()-1;
      return numSize;  // Three 0's get reduced
    case 8:
      ostr.precision(precision);
      ostr.setf(std::ios::scientific, std::ios::floatfield);
      ostr<<n/1000;
      numSize = (int) ostr.str().length();
      return numSize;
    default:
      return 0;
  }
}

// This methods determines the optimum notation, font size and orientation of
// labels from an exhaustive search
double vtkAxisExtended::Legibility(double lmin, double lmax, double lstep,
                                   double scaling,
                                   vtkVector<int, 3>& parameters)
{
  int numTicks = static_cast<int>((lmax - lmin) / lstep);
  double* tickPositions = new double[numTicks];
  int fontSizes[8] = { 8, 9, 10, 12, 14, 18, 20, 24 };
  for(int i = 0; i< numTicks; ++i)
  {
    tickPositions[i] = lmax + i*lstep;
  }

 // this->LabelLegibilityChanged = true;
  int bestFormat = 1;
  int bestOrientation = 0;
  int bestFontSize = this->DesiredFontSize;

  double bestLegScore = 0.0;

  for(int iFormat = 1; iFormat < 9; ++iFormat)
  {
    double formatLegSum = 0.0;
    for(int i = 0; i<numTicks; ++i)
    {
      formatLegSum += FormatLegibilityScore(tickPositions[i], iFormat);
    }

    // Average of label legibility scores
    formatLegSum = formatLegSum / numTicks;

    double eps = VTK_DBL_EPSILON * 100;
    int v = 1 ;
    double rem = fmod(lmin,lstep);
    if((rem < eps || (lstep - rem ) < eps ) &&  lmin <=0 && lmax >=0)
    {
      v = 0;
    }
    else
    {
      v = 1;  // v is 1 is lebelling includes zero
    }

    formatLegSum = 0.9 * formatLegSum + 0.1 * v;

    double fontLegSum = 0.0;

    // 8 font sizes are checked
    for (int fontIndex = 0; fontIndex < 8 ; ++fontIndex)
    {
      int iFont = fontSizes[fontIndex];
      if(iFont == this->DesiredFontSize)
      {
        fontLegSum = 1.0;
      }
      // fontSizes[0] is the minimum font size
      else if ( iFont<this->DesiredFontSize && iFont >= fontSizes[0])
      {
        fontLegSum = 0.2 * (iFont - fontSizes[0] + 1)
            / (this->DesiredFontSize - fontSizes[0]);
      }
      else
      {
        fontLegSum = -100.0;
      }

      for(int iOrientation = 0 ; iOrientation <2 ; ++iOrientation)
      {
        double orientLegSum = (iOrientation == 0) ? 1 : -0.5;
        // Here the gap between two consecutive labels is calculated as:
        // 2*Actual distance (in pixels) among two ticks - string lengths of
        // the two largest labels
        double overlapLegSum = 1.0;

        double legScore = (formatLegSum + fontLegSum + orientLegSum
                           + overlapLegSum) / 4;
        if(legScore > bestLegScore )
        {
          if(numTicks>1)
          {
            double fontExtent;
            if((this->IsAxisVertical && iOrientation) ||
               (!this->IsAxisVertical && !iOrientation) )
            {
              fontExtent =
                  (FormatStringLength(iFormat,tickPositions[numTicks-1],
                                      this->Precision) +
                   FormatStringLength(iFormat,tickPositions[numTicks-2],
                                      this->Precision))*iFont;
            }
            else
            {
              fontExtent = iFont * 2;
            }
            double tickDistance = lstep * scaling;
            double labelingGap= 2*(tickDistance) - fontExtent;
            // 1.1 for line spacing
            overlapLegSum = std::min(1.0,2 - 3* iFont *1.1 / labelingGap );
            /*if(labelingGap > 3*iFont)
              {
              overlapLegSum = 1.0;
              }
            else if(labelingGap < 3*iFont && labelingGap > 0)
              {
              overlapLegSum = 2 - 3* iFont / labelingGap ;
              }
            else
              {
              overlapLegSum = -100;
              }*/
          }

          legScore = (formatLegSum + fontLegSum + orientLegSum +
                      overlapLegSum)/4;

          if ( legScore > bestLegScore)
          {
            bestFormat = iFormat;
            bestOrientation = iOrientation;
            bestFontSize = iFont;
            bestLegScore = legScore;
          }
        }
      }
    }
  }

  parameters[0] = bestFormat;
  parameters[1] = bestFontSize;
  parameters[2] = bestOrientation;
  delete [] tickPositions;
  return bestLegScore;
}

// This method implements the algorithm given in the paper
vtkVector3d vtkAxisExtended::GenerateExtendedTickLabels(double dmin,
                                                        double dmax, double m,
                                                        double scaling)
{
  double Q[] = {1, 5, 2, 2.5, 4, 3};
  double w[] = {0.25, 0.2, 0.5, 0.05};
  double eps = VTK_DBL_EPSILON * 100;
  vtkVector3d ans;

  this->LabelLegibilityChanged = false;
  if(dmin > dmax)
  {
    double temp = dmin;
    dmin = dmax;
    dmax = temp;
  }

  if( dmax - dmin < eps)
  {
    ans[0] = dmin; ans[1]= dmax; ans[2]= m;
    //return Sequence(dmin,dmax, m);
    return ans;
  }

  int qLength = 6;//Q.Length(); // Hard Coded

  //list<double> best;
  double bestScore = -2;
  double bestLmin(0), bestLmax(0), bestLstep(0);

  const int INF = 100; //INT_MAX;  Again 100 is hard coded

  int j = 1;
  while(j < INF)
  {
    for(int qIndex = 0; qIndex < qLength; ++qIndex)
    {
      double sm = SimplicityMax(qIndex, qLength, j);
      if((w[0]*sm + w[1] + w[2] + w[3]) < bestScore)
      {
        j = INF;
        break;
      }

      int k = 2;
      while(k < INF)
      {
        double dm = DensityMax(k,m);
        if((w[0]*sm + w[1] + w[2]*dm + w[3]) < bestScore)
        {
          break;
        }
        double delta = (dmax- dmin)/((k+1)*j*Q[qIndex]) ;
        double z = ceil(log10(delta));
        while(z < INF)
        {
          double step = j*Q[qIndex]*pow(10.0,z);
          //double cm = CoverageMax(dmin, dmax, step*(k-1));
          if((w[0]*sm + w[1] + w[2]*dm + w[3]) < bestScore)
          {
            break;
          }

          int minStart = static_cast<int>(std::floor(dmax / step) * j - (k-1) * j);
          int maxStart = static_cast<int>(std::ceil(dmin/step) * j);

          if(minStart > maxStart)
          {
            ++z;
            continue;
          }

          for(int start = minStart; start <= maxStart; ++start)
          {
            double lmin = start * (step/j);
            double lmax = lmin + step*(k-1);
            double lstep = step;

            double s = Simplicity(qIndex, qLength, j, lmin, lmax, lstep);
            double c = Coverage(dmin, dmax, lmin, lmax);
            double g = Density(k,m,dmin, dmax, lmin, lmax);

            double score = w[0]*s + w[1]*c + w[2]*g + w[3];

            if(score < bestScore)
               continue;

            //vtkVector<double,4> l = this->Legibility(lmin, lmax, lstep, scaling);

            vtkVector<int, 3> legibilityIndex;
            double newScore = this->Legibility(lmin, lmax, lstep, scaling,
                                               legibilityIndex);

            score = w[0] * s + w[1] * c + w[2] * g + w[3] * newScore;

            if(score > bestScore)
            {
              bestScore = score;
              bestLmin = lmin;
              bestLmax = lmax;
              bestLstep = lstep;
              this->LabelFormat = legibilityIndex[0]; // label format
              this->FontSize = legibilityIndex[1]; // label font size
              this->Orientation = legibilityIndex[2]; // label orientation
            }
          }
          ++z;
        }
        ++k;
      }
    }
    ++j;
  }
  ans[0] = bestLmin;
  ans[1] = bestLmax;
  ans[2] = bestLstep;
  //vtkVector3d answers(bestLmin, bestLmax, bestLstep);
  // return Sequence(bestLmin, bestLmax, bestLstep);
  return ans;
}

void vtkAxisExtended::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "FontSize: " << this->FontSize << endl;
  os << indent << "DesiredFontSize: " << this->DesiredFontSize << endl;
  os << indent << "Precision: " << this->Precision << endl;
  os << indent << "LabelFormat: " << this->LabelFormat << endl;
}
