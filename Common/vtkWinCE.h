/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWinCE.h
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
