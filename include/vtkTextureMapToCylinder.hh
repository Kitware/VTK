/*=========================================================================

  Program:   Visualization Library
  Module:    vtkTextureMapToCylinder.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does 
not apply to the related textbook "The Visualization Toolkit" ISBN 
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute
this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. Additionally, the 
authors grant permission to modify this software and its documentation for 
any purpose, provided that such modifications are not distributed without
the explicit consent of the authors and that existing copyright notices are 
retained in all copies.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
MODIFICATIONS.

=========================================================================*/
// .NAME vtkTextureMapToCylinder - generate texture coordinates by mapping points to cylinder
// .SECTION Description
// vtkTextureMapToCylinder is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a cylinder. The cylinder can either be
// user specified or generated automatically. (The cylinder is generated 
// automatically by computing the axis of the cylinder.)  Note that the
// generated texture coordinates for the s-coordinate ranges from (0-1) 
// (corresponding to angle of 0->360 around axis), while the mapping of 
// the t-coordinate is controlled by the projection of points along the axis.
//
// To specify a cylinder manually, you must provide two points that
// define the axis of the cylinder. The length of the axis will affect the
// t-coordinates.
// 
// A special ivar controls how the s-coordinate is generated. If PreventSeam
// is set to true, the s-texture varies from 0->1 and then 1->0 (corresponding
// to angles of 0->180 and 180->360).

// .Section Caveats
// Since the resulting texture s-coordinate will lie between (0,1), and the 
// origin of the texture coordinates is not user-controllable, you may want to se 
// the class vtkTransformTexture to linearly scale and shift the origin of the texture
// coordinates.

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToSphere vtkTextureMapToBox
// vtkTransformTexture vtkThresholdTextureCoords

#ifndef __vtkTextureMapToCylinder_h
#define __vtkTextureMapToCylinder_h

#include "vtkDataSetToDataSetFilter.hh"

class vtkTextureMapToCylinder : public vtkDataSetToDataSetFilter 
{
public:
  vtkTextureMapToCylinder();
  char *GetClassName() {return "vtkTextureMapToCylinder";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the first point defining the cylinder axis,
  vtkSetVector3Macro(Point1,float);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Specify the second point defining the cylinder axis,
  vtkSetVector3Macro(Point2,float);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Turn on/off automatic cylinder generation. This means it automatically finds 
  // the cylinder center and axis.
  vtkSetMacro(AutomaticCylinderGeneration,int);
  vtkGetMacro(AutomaticCylinderGeneration,int);
  vtkBooleanMacro(AutomaticCylinderGeneration,int);

  // Description:
  // Control how the texture coordinates are generated. If PreventSeam is set, the
  // s-coordinate ranges from 0->1 and 1->0 corresponding to the angle variation
  // from 0->180 and 180->0. Otherwise, the s-ccordinate ranges from 0->1 from
  // 0->360 degrees.
  vtkSetMacro(PreventSeam,int);
  vtkGetMacro(PreventSeam,int);
  vtkBooleanMacro(PreventSeam,int);

protected:
  void Execute();

  float Point1[3];
  float Point2[3];
  int AutomaticCylinderGeneration;
  int PreventSeam;

};

#endif


