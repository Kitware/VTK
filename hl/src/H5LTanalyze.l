/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* NOTE!
 *
 * If you make any changes to H5LTanalyze.l, please run bin/genparser to
 * recreate the output files.
 */

%{
#include <stdlib.h>
#include <string.h>
#include <hdf5.h>
#include "H5LTparse.h"

/* Turn off suggest const attribute warning in gcc */
#if __GNUC__ >= 4 && __GNUC_MINOR__ >=2 
#pragma GCC diagnostic ignored "-Wsuggest-attribute=const"
#endif 

int my_yyinput(char *, int);
#undef YY_INPUT
#define YY_INPUT(b, r, ms) (r=my_yyinput(b, ms))
#define token(x)        (int)x
#define hid(x)          (hid_t)x

#ifdef  YY_BUF_SIZE
#undef  YY_BUF_SIZE
#endif
#define YY_BUF_SIZE        262144    /*Define read buffer to be 256K*/

extern char *myinput;
extern size_t input_len;

#define STACK_SIZE      16

/*variables for compound type*/
struct cmpd_info {
    hid_t       id;
    hbool_t         is_field;
    hbool_t         first_memb;
};
extern struct cmpd_info cmpd_stack[STACK_SIZE];
extern int csindex;

/*variables for array type*/
struct arr_info {
    hsize_t             dims[H5S_MAX_RANK];
    int                 ndim;
    hbool_t             is_dim;
};
extern struct arr_info arr_stack[STACK_SIZE];
extern int asindex;

/*variables for enumerate type*/
extern hbool_t     is_enum;
extern hbool_t     is_enum_memb;

/*variables for string type*/
extern hbool_t is_str_size;

/*variables for opaque type*/
extern hbool_t is_opq_size;
extern hbool_t is_opq_tag;

hbool_t        first_quote = 1;

%}

%s TAG_STRING

%%

H5T_STD_I8BE     {return hid(H5T_STD_I8BE_TOKEN);}
H5T_STD_I8LE     {return hid(H5T_STD_I8LE_TOKEN);}
H5T_STD_I16BE    {return hid(H5T_STD_I16BE_TOKEN);}
H5T_STD_I16LE    {return hid(H5T_STD_I16LE_TOKEN);}
H5T_STD_I32BE    {return hid(H5T_STD_I32BE_TOKEN);}
H5T_STD_I32LE    {return hid(H5T_STD_I32LE_TOKEN);}
H5T_STD_I64BE    {return hid(H5T_STD_I64BE_TOKEN);}
H5T_STD_I64LE    {return hid(H5T_STD_I64LE_TOKEN);}

H5T_STD_U8BE     {return hid(H5T_STD_U8BE_TOKEN);}
H5T_STD_U8LE     {return hid(H5T_STD_U8LE_TOKEN);}
H5T_STD_U16BE    {return hid(H5T_STD_U16BE_TOKEN);}
H5T_STD_U16LE    {return hid(H5T_STD_U16LE_TOKEN);}
H5T_STD_U32BE    {return hid(H5T_STD_U32BE_TOKEN);}
H5T_STD_U32LE    {return hid(H5T_STD_U32LE_TOKEN);}
H5T_STD_U64BE    {return hid(H5T_STD_U64BE_TOKEN);}
H5T_STD_U64LE    {return hid(H5T_STD_U64LE_TOKEN);}

H5T_NATIVE_CHAR  {return hid(H5T_NATIVE_CHAR_TOKEN);}
H5T_NATIVE_SCHAR {return hid(H5T_NATIVE_SCHAR_TOKEN);}
H5T_NATIVE_UCHAR {return hid(H5T_NATIVE_UCHAR_TOKEN);}
H5T_NATIVE_SHORT {return hid(H5T_NATIVE_SHORT_TOKEN);}
H5T_NATIVE_USHORT  {return hid(H5T_NATIVE_USHORT_TOKEN);}
H5T_NATIVE_INT   {return hid(H5T_NATIVE_INT_TOKEN);}
H5T_NATIVE_UINT  {return hid(H5T_NATIVE_UINT_TOKEN);}
H5T_NATIVE_LONG  {return hid(H5T_NATIVE_LONG_TOKEN);}
H5T_NATIVE_ULONG {return hid(H5T_NATIVE_ULONG_TOKEN);}
H5T_NATIVE_LLONG   {return hid(H5T_NATIVE_LLONG_TOKEN);}
H5T_NATIVE_ULLONG  {return hid(H5T_NATIVE_ULLONG_TOKEN);}

H5T_IEEE_F32BE   {return hid(H5T_IEEE_F32BE_TOKEN);}
H5T_IEEE_F32LE   {return hid(H5T_IEEE_F32LE_TOKEN);}
H5T_IEEE_F64BE   {return hid(H5T_IEEE_F64BE_TOKEN);}
H5T_IEEE_F64LE   {return hid(H5T_IEEE_F64LE_TOKEN);}
H5T_NATIVE_FLOAT   {return hid(H5T_NATIVE_FLOAT_TOKEN);}
H5T_NATIVE_DOUBLE  {return hid(H5T_NATIVE_DOUBLE_TOKEN);}
H5T_NATIVE_LDOUBLE {return hid(H5T_NATIVE_LDOUBLE_TOKEN);}

H5T_STRING       {return token(H5T_STRING_TOKEN);}
STRSIZE          {return token(STRSIZE_TOKEN);}
STRPAD           {return token(STRPAD_TOKEN);}
CSET             {return token(CSET_TOKEN);}
CTYPE            {return token(CTYPE_TOKEN);}
H5T_STR_NULLTERM {return token(H5T_STR_NULLTERM_TOKEN);} 
H5T_STR_NULLPAD  {return token(H5T_STR_NULLPAD_TOKEN);} 
H5T_STR_SPACEPAD {return token(H5T_STR_SPACEPAD_TOKEN);} 
H5T_CSET_ASCII   {return token(H5T_CSET_ASCII_TOKEN);}
H5T_CSET_UTF8    {return token(H5T_CSET_UTF8_TOKEN);}
H5T_C_S1         {return token(H5T_C_S1_TOKEN);}
H5T_FORTRAN_S1   {return token(H5T_FORTRAN_S1_TOKEN);}
H5T_VARIABLE     {return token(H5T_VARIABLE_TOKEN);}

H5T_COMPOUND     {return token(H5T_COMPOUND_TOKEN);}
H5T_ENUM         {return token(H5T_ENUM_TOKEN);}
H5T_ARRAY        {return token(H5T_ARRAY_TOKEN);}
H5T_VLEN         {return token(H5T_VLEN_TOKEN);}

H5T_OPAQUE       {return token(H5T_OPAQUE_TOKEN);}
OPQ_SIZE         {return token(OPQ_SIZE_TOKEN);}
OPQ_TAG          {return token(OPQ_TAG_TOKEN);}

[0-9]+           {    
                        if( is_str_size || (is_enum && is_enum_memb) || 
                            is_opq_size || (asindex>-1 && arr_stack[asindex].is_dim) ||
                            (csindex>-1 && cmpd_stack[csindex].is_field) ) {
                            H5LTyylval.ival = atoi(yytext);
                            return NUMBER; 
                        } else
                            REJECT;
                 }

"\""             {
                    /*if it's first quote, and is a compound field name or an enum symbol*/
                    if((is_opq_tag || is_enum || (csindex>-1 && cmpd_stack[csindex].is_field)) 
                        && first_quote) {
                        first_quote = 0;
                        BEGIN TAG_STRING;
                    } else /*if it's second quote*/
                        first_quote = 1;
                    return token('"');
                 }
<TAG_STRING>[^\"]+       {
#ifdef H5_HAVE_WIN32_API
                    H5LTyylval.sval = _strdup(yytext);
#else /* H5_HAVE_WIN32_API */
                    H5LTyylval.sval = strdup(yytext);
#endif  /* H5_HAVE_WIN32_API */
                    BEGIN INITIAL;
                    return STRING;
                 }
                 
"{"              {return token('{');}
"}"              {return token('}');}
"["              {return token('[');}
"]"              {return token(']');}
":"              {return token(':');}
";"              {return token(';');}
[ \t\n]*         ;
"\n"             { return 0; }

%%
int my_yyinput(char *buf, int max_size)
{
   int ret;
    
   memcpy(buf, myinput, input_len); 
   ret = (int)input_len;
   return ret;
}

int H5LTyyerror(const char *msg)
{
   printf("ERROR: %s before \"%s\".\n", msg, yytext);
   return 0;
}

int yywrap()
{
    return(1);
}
