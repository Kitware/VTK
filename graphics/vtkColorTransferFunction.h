/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkColorTransferFunction - Defines a transfer function for mapping a property to an RGB color value.
// .SECTION Description
// vtkPiecewiseFunction 
// 

// .SECTION see also
// vtkPiecewiseFunction

#ifndef __vtkColorTransferFunction_h
#define __vtkColorTransferFunction_h

#include "vtkObject.h"
#include "vtkPiecewiseFunction.h"

class VTK_EXPORT vtkColorTransferFunction : public vtkObject 
{
public:
  vtkColorTransferFunction();
  ~vtkColorTransferFunction();
  static vtkColorTransferFunction *New() {return new vtkColorTransferFunction;};
  const char *GetClassName() {return "vtkColorTransferFunction";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the sum of the number of function points used to specify 
  // the three independent functions (R,G,B).
  int  GetTotalSize();
  int  GetRedSize() { return this->Red.GetSize(); }
  int  GetGreenSize() { return this->Green.GetSize(); };
  int  GetBlueSize() { return this->Blue.GetSize(); };

  // Description:
  // Methods to add points to the R, G, B functions
  void AddRedPoint( float x, float r );
  void AddGreenPoint( float x, float g );
  void AddBluePoint( float x, float b );

  // Description:
  // Methods to remove points from the R, G, B functions
  void RemoveRedPoint( float x );
  void RemoveGreenPoint( float x );
  void RemoveBluePoint( float x );

  // Description:
  // Methods to add lines to the R, G, B functions
  void AddRedSegment( float x1, float r1, float x2, float r2 );
  void AddGreenSegment( float x1, float g1, float x2, float g2 );
  void AddBlueSegment( float x1, float b1, float x2, float b2 );

  // Description:
  // Convenience methods to add points and lines to all three
  // independent functions (R, G, B) simultaneously.
  void AddRGBPoint( float x, float r, float g, float b );
  void AddRGBSegment( float x1, float r1, float g1, float b1, 
                           float x2, float r2, float g2, float b2 );

  // Description:
  // Convenience method to remove points from all three
  // independent functions simultaneously.
  void RemoveRGBPoint( float x );

  // Description:
  // Removes all points in all functions
  void RemoveAllPoints();

  // Description:
  // Returns an RGB color at the specified location.
  float *GetValue( float x );
  float GetRedValue( float x ) { return this->Red.GetValue( x ); };
  float GetGreenValue( float x ) { return this->Green.GetValue( x ); };
  float GetBlueValue( float x ) { return this->Blue.GetValue( x ); };

  // Description:
  // Returns min and max position of all function points.
  float *GetRange();

  // Description:
  // Fills in a table of n function values between x1 and x2
  void GetTable( float x1, float x2, int n, float* table );


  // Description:
  // Sets and gets the clamping value for this transfer function.
  void SetClamping(int val);
  int  GetClamping();
protected:

  // Determines the function value outside of defined points
  // in each of the R,G,B transfer functions.
  // Zero = always return 0.0 outside of defined points
  // One  = clamp to the lowest value below defined points and
  //        highest value above defined points
  int Clamping;

  // Transfer functions for each color component
  vtkPiecewiseFunction	Red;
  vtkPiecewiseFunction	Green;
  vtkPiecewiseFunction	Blue;

  // An evaluated color
  float  ColorValue[3];

  // The min and max point locations for all three transfer functions
  float Range[2]; 

  // Description:
  // Calculates the min and max point locations for all three transfer functions
  void UpdateRange();

};

#endif


