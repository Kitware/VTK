/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWinCE.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWinCE - support for C++ functionality missing in WinCE
// .SECTION Description
// This is a half baked implementaiton of some of the key features
// missing from WindowsCE. Half of this is implemented and half is
// missing.

#ifndef vtkWinCE_h

#define vtkWinCE_h
#define VTK_LEAN_AND_MEAN

struct ostream    // Output stream
{
  ostream & operator <<(const char *str);   // strings
  ostream & operator <<(void *str);   // pointers
  ostream & operator <<(short x) ; // integers
  ostream & operator <<(unsigned short x) ; // integers
  ostream & operator <<(int x) ; // integers
  ostream & operator <<(unsigned int x) ; // integers
  ostream &operator <<(long l) ; // long integers
  ostream &operator <<(unsigned long l) ; // long integers
  ostream & operator << (char ch) ;  // characters
  ostream & operator <<(float fl ) ;  // floats
  ostream & operator << (double dbl);  // double floats

  write(void *data, size_t size);
  void flush() {};
};

struct ofstream    // Output stream
{
  ofstream & operator <<(const char *str);   // strings
  ofstream & operator <<(void *str);   // pointers
  ofstream & operator <<(short x) ; // integers
  ofstream & operator <<(unsigned short x) ; // integers
  ofstream & operator <<(int x) ; // integers
  ofstream &operator <<(long l) ; // long integers
  ofstream &operator <<(unsigned long l) ; // long integers
  ofstream & operator << (char ch) ;  // characters
  ofstream & operator <<(float fl ) ;  // floats
  ofstream & operator << (double dbl);  // double floats
  
  write(void *data, size_t size);
};

struct ostrhelp
{
  freeze(int i);
};

struct ostrstream    // Output stream
{
  ostrstream & operator <<(char *str);   // strings
  ostrstream & operator <<(void *str);   // pointers
  ostrstream & operator <<(int x) ; // integers
  ostrstream &operator <<(long l) ; // long integers
  ostrstream & operator << (char ch) ;  // characters
  ostrstream & operator <<(float fl ) ;  // floats
  ostrstream & operator << (double dbl);  // double floats

  char *str();
  ostrhelp *rdbuf();
};
 

struct istream
{
  int get(char &c) { c = (char)getchar(); return c;}
  int putback(char &c) { return ungetc(c,stdin);}
  istream & operator >>(char *str);  // strings
  istream & operator >>(int  &x) ; // integers
  istream & operator >> (char  &ch) ;  // characters
  istream & operator >> (float &fl ) ;  // floats
  istream & operator >> (double &dbl );  // doubles
};

 

extern  char *endl;   
extern ostream cout;
extern ostream cerr;  
extern istream cin;  

#endif vtkWinCE_h
