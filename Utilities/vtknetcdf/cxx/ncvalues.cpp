/*********************************************************************
 *   Copyright 1992, University Corporation for Atmospheric Research
 *   See netcdf/README file for copying and redistribution conditions.
 *
 *   Purpose:	implementation of classes of typed arrays for netCDF
 *
 *   $Header: /upc/share/CVS/netcdf-3/cxx/ncvalues.cpp,v 1.12 2008/03/05 16:45:32 russ Exp $
 *********************************************************************/

#include <ncconfig.h>
#include <iostream>
#include <string>
#include <cstring>

#include "ncvalues.h"

// pragma GCC diagnostic is available since gcc>=4.2
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

NcValues::NcValues( void ) : the_type(ncNoType), the_number(0)
{}

NcValues::NcValues(NcType type, long Num)
        : the_type(type), the_number(Num)
{}

NcValues::~NcValues( void )
{}

long NcValues::num( void )
{
    return the_number;
}

std::ostream& operator<< (std::ostream& os, const NcValues& vals)
{
    return vals.print(os);
}

implement(NcValues,ncbyte)
implement(NcValues,char)
implement(NcValues,short)
implement(NcValues,int)
implement(NcValues,nclong)
implement(NcValues,long)
implement(NcValues,float)
implement(NcValues,double)

Ncbytes_for_one_implement(ncbyte)
Ncbytes_for_one_implement(char)
Ncbytes_for_one_implement(short)
Ncbytes_for_one_implement(int)
Ncbytes_for_one_implement(nclong)
Ncbytes_for_one_implement(long)
Ncbytes_for_one_implement(float)
Ncbytes_for_one_implement(double)

as_ncbyte_implement(short)
as_ncbyte_implement(int)
as_ncbyte_implement(nclong)
as_ncbyte_implement(long)
as_ncbyte_implement(float)
as_ncbyte_implement(double)

inline ncbyte NcValues_char::as_ncbyte( long n ) const
{
    return the_values[n];
}

inline ncbyte NcValues_ncbyte::as_ncbyte( long n ) const
{
    return the_values[n];
}

as_char_implement(short)
as_char_implement(int)
as_char_implement(nclong)
as_char_implement(long)
as_char_implement(float)
as_char_implement(double)

inline char NcValues_ncbyte::as_char( long n ) const
{
    return the_values[n] > CHAR_MAX ? ncBad_char : (char) the_values[n];
}

inline char NcValues_char::as_char( long n ) const
{
    return the_values[n];
}

as_short_implement(int)
as_short_implement(nclong)
as_short_implement(long)
as_short_implement(float)
as_short_implement(double)

inline short NcValues_ncbyte::as_short( long n ) const
{
    return the_values[n];
}

inline short NcValues_char::as_short( long n ) const
{
    return the_values[n];
}

inline short NcValues_short::as_short( long n ) const
{
    return the_values[n];
}


as_int_implement(float)
as_int_implement(double)

inline int NcValues_ncbyte::as_int( long n ) const
{
    return the_values[n];
}

inline int NcValues_char::as_int( long n ) const
{
    return the_values[n];
}

inline int NcValues_short::as_int( long n ) const
{
    return the_values[n];
}

inline int NcValues_int::as_int( long n ) const
{
    return the_values[n];
}

inline int NcValues_nclong::as_int( long n ) const
{
    return the_values[n];
}

inline int NcValues_long::as_int( long n ) const
{
    return the_values[n];
}

as_nclong_implement(float)
as_nclong_implement(double)

inline nclong NcValues_ncbyte::as_nclong( long n ) const
{
    return the_values[n];
}

inline nclong NcValues_char::as_nclong( long n ) const
{
    return the_values[n];
}

inline nclong NcValues_short::as_nclong( long n ) const
{
    return the_values[n];
}

inline nclong NcValues_int::as_nclong( long n ) const
{
    return the_values[n];
}

inline nclong NcValues_nclong::as_nclong( long n ) const
{
    return the_values[n];
}

inline nclong NcValues_long::as_nclong( long n ) const
{
    return the_values[n];
}

as_long_implement(float)
as_long_implement(double)

inline long NcValues_ncbyte::as_long( long n ) const
{
    return the_values[n];
}

inline long NcValues_char::as_long( long n ) const
{
    return the_values[n];
}

inline long NcValues_short::as_long( long n ) const
{
    return the_values[n];
}

inline long NcValues_int::as_long( long n ) const
{
    return the_values[n];
}

inline long NcValues_nclong::as_long( long n ) const
{
    return the_values[n];
}

inline long NcValues_long::as_long( long n ) const
{
    return the_values[n];
}

as_float_implement(ncbyte)
as_float_implement(char)
as_float_implement(short)
as_float_implement(int)
as_float_implement(nclong)
as_float_implement(long)
as_float_implement(float)
as_float_implement(double)

as_double_implement(ncbyte)
as_double_implement(char)
as_double_implement(short)
as_double_implement(int)
as_double_implement(nclong)
as_double_implement(long)
as_double_implement(float)
as_double_implement(double)

as_string_implement(short)
as_string_implement(int)
as_string_implement(nclong)
as_string_implement(long)
as_string_implement(float)
as_string_implement(double)

inline char* NcValues_ncbyte::as_string( long n ) const
{
    char* s = new char[the_number + 1];
    s[the_number] = '\0';
    strncpy(s, (const char*)the_values + n, (int)the_number);
    return s;
}

inline char* NcValues_char::as_string( long n ) const
{
    char* s = new char[the_number + 1];
    s[the_number] = '\0';
    strncpy(s, (const char*)the_values + n, (int)the_number);
    return s;
}

std::ostream& NcValues_short::print(std::ostream& os) const
{
    for(int i = 0; i < the_number - 1; i++)
      os << the_values[i] << ", ";
    if (the_number > 0)
      os << the_values[the_number-1] ;
    return os;
}

std::ostream& NcValues_int::print(std::ostream& os) const
{
    for(int i = 0; i < the_number - 1; i++)
      os << the_values[i] << ", ";
    if (the_number > 0)
      os << the_values[the_number-1] ;
    return os;
}

std::ostream& NcValues_nclong::print(std::ostream& os) const
{
    for(int i = 0; i < the_number - 1; i++)
      os << the_values[i] << ", ";
    if (the_number > 0)
      os << the_values[the_number-1] ;
    return os;
}

std::ostream& NcValues_long::print(std::ostream& os) const
{
    for(int i = 0; i < the_number - 1; i++)
      os << the_values[i] << ", ";
    if (the_number > 0)
      os << the_values[the_number-1] ;
    return os;
}

std::ostream& NcValues_ncbyte::print(std::ostream& os) const
{
    for(int i = 0; i < the_number - 1; i++)
      os << the_values[i] << ", ";
    if (the_number > 0)
      os << the_values[the_number-1] ;
    return os;
}

std::ostream& NcValues_char::print(std::ostream& os) const
{
    os << '"';
    long len = the_number;
    while (the_values[--len] == '\0') // don't output trailing null bytes
        ;
    for(int i = 0; i <= len; i++)
        os << the_values[i] ;
    os << '"';

    return os;
}

std::ostream& NcValues_float::print(std::ostream& os) const
{
    std::streamsize save=os.precision();
    os.precision(7);
    for(int i = 0; i < the_number - 1; i++)
      os << the_values[i] << ", ";
    if (the_number > 0)
      os << the_values[the_number-1] ;
    os.precision(save);
    return os;
}

std::ostream& NcValues_double::print(std::ostream& os) const
{
    std::streamsize save=os.precision();
    os.precision(15);
    for(int i = 0; i < the_number - 1; i++)
      os << the_values[i] << ", ";
    if (the_number > 0)
      os << the_values[the_number-1];
    os.precision(save);
    return os;
}
