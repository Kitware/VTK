/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWinCE.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include <stdio.h>
#include "vtkWinCE.h"

char *endl = "\n";     // declare endl here
ostream cout;          // declare cout here
ostream cerr;          // declare cerr here
istream cin;          // declare cin  here

 
ostream & ostream::operator <<(const char *str)   
{  
  printf("%s",str);  return *this;  
} 

ostream & ostream::operator <<(int x)  // integers
{  
  printf("%d",x);  
  return *this;  
}

ostream & ostream::operator <<(void *x)  // integers
{  
  printf("%p",x);  
  return *this;  
}

ostream & ostream::operator <<(unsigned int x)  // integers
{  
  printf("%u",x);  
  return *this;  
}
 

ostream & ostream::operator <<(long l)  // long integers
{  
  printf( "%ld",l);  
  return *this;  
}

ostream & ostream::operator <<(unsigned long l)  // long integers
{  
  printf( "%lu",l);  
  return *this;  
} 

ostream & ostream::operator << (char ch)  // characters
{  
  printf("%c",ch);  
  return *this;  
}


ostream & ostream::operator <<(float fl )   // floats
{  
  printf("%f",fl);  
  return *this;  
}


ostream & ostream::operator << (double dbl)  // double floats
{  
  printf("%lf",dbl);  
  return *this;  
} 

////////////////////////// istream members ////////////////////////////////

istream & istream::operator >>(char *str)  // strings
{    
  gets(str);  
  return *this;   
} 

istream & istream::operator >>(int  &x)  // integers
{   
  scanf("%d",&x);  
  return *this;  
} 

istream & istream::operator >> (char  &ch)   // characters
{  
  scanf("%c",&ch);  
  return *this;  
} 

istream & istream::operator >>(float &fl )   // floats
{  
  scanf("%f",&fl);  
  return *this;  
} 

istream & istream::operator >> (double &dbl )  // doubles
{   
  scanf("%lf",&dbl);  
  return *this;  
}

 
