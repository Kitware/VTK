/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWinCE.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

 
