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
/**
 * @class   vtkAxisExtended
 * @brief   octree-based spatial search object to quickly locate cells
 *
 * This implements the optimization based tick position calculating algorithm in the paper "An Extension of Wilkinson's Algorithm
 * for Positioning Tick Labels on Axes" by Junstin Talbot, Sharon Lin and Pat Hanrahan
 *
 *
 * @sa
 * vtkAxis
*/

#ifndef vtkAxisExtended_h
#define vtkAxisExtended_h
#endif

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkVector.h" // Needed for vtkVector

class VTKCHARTSCORE_EXPORT vtkAxisExtended : public vtkObject
{
public:
   vtkTypeMacro(vtkAxisExtended, vtkObject);
   static vtkAxisExtended *New();
   void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

   /**
    * This method return a value to make step sizes corresponding to low q
    * and j values more preferable
    */
   static double Simplicity(int qIndex, int qLength, int j, double lmin,
                            double lmax, double lstep);

   /**
    * This method returns the maximum possible value of simplicity value given
    * q and j
    */
   static double SimplicityMax(int qIndex, int qLength, int j);

   /**
    * This method makes the data range approximately same as the labeling
    * range more preferable
    */
   static double Coverage(double dmin, double dmax, double lmin, double lmax);

   /**
    * This gives the maximum possible value of coverage given the step size
    */
   static double CoverageMax(double dmin, double dmax, double span);

   /**
    * This method return a value to make the density of the labels close to
    * the user given value
    */
   static double Density(int k, double m, double dmin, double dmax,
                         double lmin, double lmax);

   /**
    * Derives the maximum values for density given k (number of ticks) and
    * m (user given)
    */
   static double DensityMax(int k, double m);

   /**
    * This methods return the legibility score of different formats
    */
   static double FormatLegibilityScore(double n, int format);

   /**
    * This method returns the string length of different format notations.
    */
   static int FormatStringLength(int format, double n, int precision);

   /**
    * This method implements the algorithm given in the paper
    * The method return the minimum tick position, maximum tick position and
    * the tick spacing
    */
   vtkVector3d GenerateExtendedTickLabels(double dmin, double dmax, double m,
                                          double scaling);

   //@{
   /**
    * Set/Get methods for variables
    */
   vtkGetMacro(FontSize, int);
   vtkSetMacro(FontSize, int);
   //@}

   vtkGetMacro(DesiredFontSize, int);
   vtkSetMacro(DesiredFontSize, int);

   vtkGetMacro(Precision, int);
   vtkSetMacro(Precision, int);
   vtkGetMacro(LabelFormat, int);
   vtkSetMacro(LabelFormat, int);

   vtkGetMacro(Orientation, int);
   vtkSetMacro(Orientation, int);

   vtkGetMacro(IsAxisVertical, bool);
   vtkSetMacro(IsAxisVertical, bool);

protected:
  vtkAxisExtended();
  ~vtkAxisExtended();

  /**
   * This method implements an exhaustive search of the legibilty parameters.
   */
  double Legibility(double lmin, double lmax, double lstep, double scaling,
                    vtkVector<int, 3>& parameters);

  int Orientation;
  int FontSize;
  int DesiredFontSize;
  int Precision;
  int LabelFormat;
  bool LabelLegibilityChanged;
  bool IsAxisVertical;

private:
  vtkAxisExtended(const vtkAxisExtended&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAxisExtended&) VTK_DELETE_FUNCTION;
};
