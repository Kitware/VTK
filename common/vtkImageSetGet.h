/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSetGet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this file.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// ImageSetGet provides macros which expand the number of ways a method
// can be called.


#ifndef __vtkImageSetGet_h
#define __vtkImageSetGet_h



// These macro are for creating the many convenience functions used 
// for accessing instance variables.  They could simplify this class.
#define vtkImageRegionSetMacro(name,type) \
virtual void Set##name (type *_tmp) { this->Set##name (_tmp, 5);} \
virtual void Set##name (type _name0,type _name1,type _name2, type _name3, \
			type _name4)\
{ \
  type _tmp[5]; \
  _tmp[0] = _name0; _tmp[1] = _name1; _tmp[2] = _name2; \
  _tmp[3] = _name3; _tmp[4] = _name4; \
  this->Set##name (_tmp,5); \
} \
virtual void Set##name (type _name0,type _name1,type _name2, type _name3) \
{ \
  type _tmp[4]; \
  _tmp[0] = _name0; _tmp[1] = _name1; \
  _tmp[2] = _name2; _tmp[3] = _name3; \
  this->Set##name (_tmp,4); \
} \
virtual void Set##name (type _name0,type _name1,type _name2) \
{ \
  type _tmp[3]; \
  _tmp[0] = _name0; _tmp[1] = _name1; _tmp[2] = _name2; \
  this->Set##name (_tmp,3); \
} \
virtual void Set##name (type _name0,type _name1) \
{ \
  type _tmp[2]; \
  _tmp[0] = _name0; _tmp[1] = _name1; \
  this->Set##name (_tmp,2); \
} \
virtual void Set##name (type _name0) \
{ \
  type _tmp[1]; \
  _tmp[0] = _name0; \
  this->Set##name (_tmp,1); \
} 
#define vtkImageRegionGetMacro(name,type) \
virtual type *Get##name () { return this->##name ;}  \
virtual void Get##name (type _tmp[5]) { this->Get##name (_tmp, 5);} \
virtual void Get##name (type &_name0,type &_name1,type &_name2,type &_name3, \
			type &_name4) \
{ \
  type _tmp[5]; \
  this->Get##name (_tmp,5); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; _name2 = _tmp[2]; \
  _name3 = _tmp[3]; _name4 = _tmp[4]; \
} \
virtual void Get##name (type &_name0,type &_name1,type &_name2,type &_name3) \
{ \
  type _tmp[4]; \
  this->Get##name (_tmp,4); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; _name2 = _tmp[2]; _name3 = _tmp[3]; \
} \
virtual void Get##name (type &_name0,type &_name1,type &_name2) \
{ \
  type _tmp[3]; \
  this->Get##name (_tmp,3); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; _name2 = _tmp[2]; \
} \
virtual void Get##name (type &_name0,type &_name1) \
{ \
  type _tmp[2]; \
  this->Get##name (_tmp,2); \
  _name0 = _tmp[0]; _name1 = _tmp[1]; \
} \
virtual void Get##name (type &_name0) \
{ \
  type _tmp[1]; \
  this->Get##name (_tmp,1); \
  _name0 = _tmp[0]; \
} 
#define vtkImageRegionSetExtentMacro(name) \
virtual void Set##name (int _tmp[10]) { this->Set##name (_tmp, 5);} \
virtual void Set##name (int _min0,int _max0,int _min1,int _max1, \
		int _min2,int _max2,int _min3,int _max3, \
		int _min4,int _max4) \
{ \
  int _tmp[10]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  _tmp[4] = _min2; _tmp[5] = _max2; \
  _tmp[6] = _min3; _tmp[7] = _max3; \
  _tmp[8] = _min4; _tmp[9] = _max4; \
  this->Set##name (_tmp,5); \
} \
virtual void Set##name (int _min0,int _max0,int _min1,int _max1, \
		int _min2,int _max2,int _min3,int _max3) \
{ \
  int _tmp[8]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  _tmp[4] = _min2; _tmp[5] = _max2; \
  _tmp[6] = _min3; _tmp[7] = _max3; \
  this->Set##name (_tmp,4); \
} \
virtual void Set##name (int _min0,int _max0,int _min1,int _max1, \
		int _min2,int _max2) \
{ \
  int _tmp[6]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  _tmp[4] = _min2; _tmp[5] = _max2; \
  this->Set##name (_tmp,3); \
} \
virtual void Set##name (int _min0,int _max0,int _min1,int _max1) \
{ \
  int _tmp[4]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  _tmp[2] = _min1; _tmp[3] = _max1; \
  this->Set##name (_tmp,2); \
} \
virtual void Set##name (int _min0,int _max0) \
{ \
  int _tmp[2]; \
  _tmp[0] = _min0; _tmp[1] = _max0; \
  this->Set##name (_tmp,1); \
} 
#define vtkImageRegionGetExtentMacro(name) \
int *Get##name () { return this->##name ;}  \
virtual void Get##name (int _tmp[10]) { this->Get##name (_tmp, 5);} \
virtual void Get##name (int &_min0,int &_max0,int &_min1,int &_max1, \
		int &_min2,int &_max2,int &_min3,int &_max3, \
		int &_min4,int &_max4) \
{ \
  int _tmp[10]; \
  this->Get##name (_tmp,5); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
  _min2 = _tmp[4]; _max2 = _tmp[5]; \
  _min3 = _tmp[6]; _max3 = _tmp[7]; \
  _min4 = _tmp[8]; _max4 = _tmp[9]; \
} \
virtual void Get##name (int &_min0,int &_max0,int &_min1,int &_max1, \
		int &_min2,int &_max2,int &_min3,int &_max3) \
{ \
  int _tmp[8]; \
  this->Get##name (_tmp,4); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
  _min2 = _tmp[4]; _max2 = _tmp[5]; \
  _min3 = _tmp[6]; _max3 = _tmp[7]; \
} \
virtual void Get##name (int &_min0,int &_max0,int &_min1,int &_max1, \
		int &_min2,int &_max2) \
{ \
  int _tmp[6]; \
  this->Get##name (_tmp,3); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
  _min2 = _tmp[4]; _max2 = _tmp[5]; \
} \
virtual void Get##name (int &_min0,int &_max0,int &_min1,int &_max1) \
{ \
  int _tmp[4]; \
  this->Get##name (_tmp,2); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
  _min1 = _tmp[2]; _max1 = _tmp[3]; \
} \
virtual void Get##name (int &_min0,int &_max0) \
{ \
  int _tmp[2]; \
  this->Get##name (_tmp,1); \
  _min0 = _tmp[0]; _max0 = _tmp[1]; \
}


#endif


