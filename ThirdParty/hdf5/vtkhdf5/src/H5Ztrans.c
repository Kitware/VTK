/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define H5Z_PACKAGE		/*suppress error about including H5Zpkg	  */


#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5VMprivate.h"		/* H5VM_array_fill			*/
#include "H5Zpkg.h"		/* Data filters				*/


/* Token types */
typedef enum {
    H5Z_XFORM_ERROR,
    H5Z_XFORM_INTEGER, /* this represents an integer type in the data transform expression */
    H5Z_XFORM_FLOAT,  /* this represents a floating point type in the data transform expression */
    H5Z_XFORM_SYMBOL,
    H5Z_XFORM_PLUS,
    H5Z_XFORM_MINUS,
    H5Z_XFORM_MULT,
    H5Z_XFORM_DIVIDE,
    H5Z_XFORM_LPAREN,
    H5Z_XFORM_RPAREN,
    H5Z_XFORM_END
} H5Z_token_type;


typedef struct {
    unsigned int	num_ptrs;
    void**	ptr_dat_val;
} H5Z_datval_ptrs;


/* Used to represent values in transform expression */
typedef union {
    void   *dat_val;
    long    int_val;
    double  float_val;
} H5Z_num_val;

typedef struct H5Z_node {
    struct H5Z_node    *lchild;
    struct H5Z_node    *rchild;
    H5Z_token_type      type;
    H5Z_num_val         value;
} H5Z_node;

struct H5Z_data_xform_t {
    char*       xform_exp;
    H5Z_node*       parse_root;
    H5Z_datval_ptrs*	dat_val_pointers;
};

typedef struct result {
    H5Z_token_type type;
    H5Z_num_val    value;
} H5Z_result;


/* The token */
typedef struct {
    const char *tok_expr;       /* Holds the original expression        */

    /* Current token values */
    H5Z_token_type  tok_type;       /* The type of the current token        */
    const char *tok_begin;      /* The beginning of the current token   */
    const char *tok_end;        /* The end of the current token         */

    /* Previous token values */
    H5Z_token_type  tok_last_type;  /* The type of the last token           */
    const char *tok_last_begin; /* The beginning of the last token      */
    const char *tok_last_end;   /* The end of the last token            */
} H5Z_token;

/* Local function prototypes */
static H5Z_token *H5Z_get_token(H5Z_token *current);
static H5Z_node *H5Z_parse_expression(H5Z_token *current, H5Z_datval_ptrs* dat_val_pointers);
static H5Z_node *H5Z_parse_term(H5Z_token *current, H5Z_datval_ptrs* dat_val_pointers);
static H5Z_node *H5Z_parse_factor(H5Z_token *current, H5Z_datval_ptrs* dat_val_pointers);
static H5Z_node *H5Z_new_node(H5Z_token_type type);
static void H5Z_do_op(H5Z_node* tree);
static hbool_t H5Z_op_is_numbs(H5Z_node* _tree);
static hbool_t H5Z_op_is_numbs2(H5Z_node* _tree);
static hid_t H5Z_xform_find_type(const H5T_t* type);
static herr_t H5Z_xform_eval_full(H5Z_node *tree, const size_t array_size, const hid_t array_type, H5Z_result* res);
static void H5Z_xform_destroy_parse_tree(H5Z_node *tree);
static void* H5Z_xform_parse(const char *expression, H5Z_datval_ptrs* dat_val_pointers);
static void* H5Z_xform_copy_tree(H5Z_node* tree, H5Z_datval_ptrs* dat_val_pointers, H5Z_datval_ptrs* new_dat_val_pointers);
static void H5Z_xform_reduce_tree(H5Z_node* tree);
#ifdef H5Z_XFORM_DEBUG
static void H5Z_XFORM_DEBUG(H5Z_node *tree);
static void H5Z_print(H5Z_node *tree, FILE *stream);
#endif  /* H5Z_XFORM_DEBUG */

/* PGCC (11.8-0) has trouble with the command *p++ = *p OP tree_val. It increments P first before 
 * doing the operation.  So I break down the command into two lines:
 *     *p = *p OP tree_val; p++;
 * Actually, the behavior of *p++ = *p OP tree_val is undefined. (SLU - 2012/3/19) 
 */
#define H5Z_XFORM_DO_OP1(RESL,RESR,TYPE,OP,SIZE)                            \
{   								  	    \
    size_t u;                                                               \
                                                                            \
    if(((RESL).type == H5Z_XFORM_SYMBOL) && ((RESR).type != H5Z_XFORM_SYMBOL)) \
    {   								    \
	TYPE* p;                                                            \
	double tree_val;                                                    \
		                                                            \
	tree_val = ((RESR).type==H5Z_XFORM_INTEGER ? (double)(RESR).value.int_val : (RESR).value.float_val); \
	p = (TYPE*)(RESL).value.dat_val;                                    \
                                                                            \
	for(u=0; u<(SIZE); u++) {                                           \
	    *p = *p OP tree_val;                                            \
            p++;                                                            \
        }                                                                   \
    }									    \
    else if(((RESR).type == H5Z_XFORM_SYMBOL) && ((RESL).type != H5Z_XFORM_SYMBOL)) \
    {   								    \
	TYPE* p;                                                            \
	double tree_val;                                                    \
		                                                            \
        /* The case that the left operand is nothing, like -x or +x */      \
	if((RESL).type == H5Z_XFORM_ERROR)                                  \
            tree_val = 0;                                                   \
	else                                                                \
	    tree_val = ((RESL).type==H5Z_XFORM_INTEGER ? (double)(RESL).value.int_val : (RESL).value.float_val); \
                                                                            \
	p = (TYPE*)(RESR).value.dat_val;                                    \
	for(u=0; u<(SIZE); u++) {                                           \
	    *p = tree_val OP *p;                                            \
            p++;                                                            \
        }                                                                   \
    }									    \
    else if( ((RESL).type == H5Z_XFORM_SYMBOL) && ((RESR).type==H5Z_XFORM_SYMBOL))  \
    {										\
	TYPE* pl = (TYPE*)(RESL).value.dat_val;                             	\
	TYPE* pr = (TYPE*)(RESR).value.dat_val;                             	\
										\
	for(u=0; u<(SIZE); u++) {                                            	\
	    *pl = *pl OP *pr;                                           	\
            pl++; pr++;                                                         \
        }                                                                       \
    }										\
    else									\
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Unexpected type conversion operation")	\
}

/* Due to the undefined nature of embedding macros/conditionals within macros, we employ
 * this clever little hack.  We always compile in the code for the type conversion, even if
 * it isn't supported in the compiler.  To avoid errors, we define  ULLONG_TO_FP_XFORM_TYPE_OP_ERROR on
 * unsupported compilers, which will cause the code to execute HGOTO_ERROR and skip the code
 * that does the actual conversion */


#ifndef H5_ULLONG_TO_FP_CAST_WORKS
#define H5Z_XFORM_ULL_DO_OP1(RESL,RESR,TYPE,OP,SIZE)                    \
{									\
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Cannot convert from unsigned long long to double: required for data transform") \
}
#else
#define H5Z_XFORM_ULL_DO_OP1(RESL,RESR,TYPE,OP,SIZE)                    \
    H5Z_XFORM_DO_OP1(RESL,RESR,TYPE,OP,SIZE)
#endif

/* Windows Intel 8.1 compiler has error converting long long to double.
 * Hard code it in.
 */
#ifndef H5_LLONG_TO_FP_CAST_WORKS
#define H5Z_XFORM_LL_DO_OP1(RESL,RESR,TYPE,OP,SIZE)                     \
{									\
    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Cannot convert from long long to double: required for data transform") \
}
#else
#define H5Z_XFORM_LL_DO_OP1(RESL,RESR,TYPE,OP,SIZE)                     \
    H5Z_XFORM_DO_OP1(RESL,RESR,TYPE,OP,SIZE)
#endif
#if H5_SIZEOF_LONG_DOUBLE !=0
#define H5Z_XFORM_TYPE_OP(RESL,RESR,TYPE,OP,SIZE)			\
{									\
    if((TYPE) == H5T_NATIVE_CHAR)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), char, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_UCHAR)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned char, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_SCHAR)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), signed char, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_SHORT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), short, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_USHORT)				\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned short, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_INT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), int, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_UINT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned int, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_LONG)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), long, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_ULONG)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned long, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_LLONG)					\
	H5Z_XFORM_LL_DO_OP1((RESL), (RESR), long long, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_ULLONG)				\
	H5Z_XFORM_ULL_DO_OP1((RESL), (RESR), unsigned long long, OP, (SIZE)) \
    else if((TYPE) == H5T_NATIVE_FLOAT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), float, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_DOUBLE)				\
	H5Z_XFORM_DO_OP1((RESL), (RESR), double, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_LDOUBLE)				\
	H5Z_XFORM_DO_OP1((RESL), (RESR), long double, OP, (SIZE))	\
}
#else
#define H5Z_XFORM_TYPE_OP(RESL,RESR,TYPE,OP,SIZE)			\
{									\
    if((TYPE) == H5T_NATIVE_CHAR)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), char, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_UCHAR)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned char, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_SCHAR)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), signed char, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_SHORT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), short, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_USHORT)				\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned short, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_INT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), int, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_UINT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned int, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_LONG)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), long, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_ULONG)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), unsigned long, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_LLONG)					\
	H5Z_XFORM_LL_DO_OP1((RESL), (RESR), long long, OP, (SIZE))	\
    else if((TYPE) == H5T_NATIVE_ULLONG)				\
	H5Z_XFORM_ULL_DO_OP1((RESL), (RESR), unsigned long long, OP, (SIZE)) \
    else if((TYPE) == H5T_NATIVE_FLOAT)					\
	H5Z_XFORM_DO_OP1((RESL), (RESR), float, OP, (SIZE))		\
    else if((TYPE) == H5T_NATIVE_DOUBLE)				\
	H5Z_XFORM_DO_OP1((RESL), (RESR), double, OP, (SIZE))		\
}
#endif /*H5_SIZEOF_LONG_DOUBLE */

#define H5Z_XFORM_DO_OP3(OP)                                                                                                                    \
{                                                                                                                                               \
        if((tree->lchild->type == H5Z_XFORM_INTEGER) && (tree->rchild->type==H5Z_XFORM_INTEGER))                                                \
        {																	\
            tree->type = H5Z_XFORM_INTEGER;													\
            tree->value.int_val = tree->lchild->value.int_val OP tree->rchild->value.int_val;							\
            H5MM_xfree(tree->lchild);														\
            H5MM_xfree(tree->rchild);														\
            tree->lchild = NULL;														\
            tree->rchild = NULL;														\
        }																	\
	else if( ( (tree->lchild->type == H5Z_XFORM_FLOAT) || (tree->lchild->type == H5Z_XFORM_INTEGER)) && 					\
	            ( (tree->rchild->type == H5Z_XFORM_FLOAT) || (tree->rchild->type == H5Z_XFORM_INTEGER)))					\
	{																	\
	    tree->type = H5Z_XFORM_FLOAT;													\
           tree->value.float_val = ((tree->lchild->type == H5Z_XFORM_FLOAT) ? tree->lchild->value.float_val : (double)tree->lchild->value.int_val) OP 	\
				    ((tree->rchild->type == H5Z_XFORM_FLOAT) ? tree->rchild->value.float_val : (double)tree->rchild->value.int_val);	\
            H5MM_xfree(tree->lchild);														\
            H5MM_xfree(tree->rchild);														\
            tree->lchild = NULL;														\
            tree->rchild = NULL;														\
        }																	\
}

#define H5Z_XFORM_DO_OP4(TYPE)                                                                                          \
{                                                                                                                       \
    if ((ret_value = (H5Z_node*) H5MM_malloc(sizeof(H5Z_node))) == NULL)                                                \
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Ran out of memory trying to copy parse tree")                     \
    else                                                                                                                \
    {                                                                                                                   \
        ret_value->type = (TYPE);                                                                                       \
        ret_value->lchild = (H5Z_node*) H5Z_xform_copy_tree(tree->lchild, dat_val_pointers, new_dat_val_pointers);      \
        ret_value->rchild = (H5Z_node*) H5Z_xform_copy_tree(tree->rchild, dat_val_pointers, new_dat_val_pointers);      \
    }                                                                                                                   \
}

#define H5Z_XFORM_DO_OP5(TYPE, SIZE)                                                               \
{                                                                                                  \
    TYPE val = ((tree->type == H5Z_XFORM_INTEGER) ? (TYPE)tree->value.int_val : (TYPE)tree->value.float_val); \
    H5VM_array_fill(array, &val, sizeof(TYPE), (SIZE));                                             \
}

/* The difference of this macro from H5Z_XFORM_DO_OP3 is that it handles the operations when the left operand is empty, like -x or +x.  
 * The reason that it's seperated from H5Z_XFORM_DO_OP3 is because compilers don't accept operations like *x or /x.  So in H5Z_do_op, 
 * these two macros are called in different ways. (SLU 2012/3/20) 
 */
#define H5Z_XFORM_DO_OP6(OP)                                                                                                                    \
{                                                                                                                                               \
        if(!tree->lchild && (tree->rchild->type==H5Z_XFORM_INTEGER))                                                                            \
        {																	\
            tree->type = H5Z_XFORM_INTEGER;													\
            tree->value.int_val = OP tree->rchild->value.int_val;							                        \
            H5MM_xfree(tree->rchild);														\
            tree->rchild = NULL;														\
        }	                                                                                                                                \
        else if(!tree->lchild && (tree->rchild->type==H5Z_XFORM_FLOAT))                                                                         \
        {																	\
            tree->type = H5Z_XFORM_FLOAT;													\
            tree->value.float_val = OP tree->rchild->value.float_val;							                        \
            H5MM_xfree(tree->rchild);														\
            tree->rchild = NULL;														\
        }	                                                                                                                                \
        else if((tree->lchild->type == H5Z_XFORM_INTEGER) && (tree->rchild->type==H5Z_XFORM_INTEGER))                                           \
        {																	\
            tree->type = H5Z_XFORM_INTEGER;													\
            tree->value.int_val = tree->lchild->value.int_val OP tree->rchild->value.int_val;							\
            H5MM_xfree(tree->lchild);														\
            H5MM_xfree(tree->rchild);														\
            tree->lchild = NULL;														\
            tree->rchild = NULL;														\
        }																	\
	else if( ( (tree->lchild->type == H5Z_XFORM_FLOAT) || (tree->lchild->type == H5Z_XFORM_INTEGER)) && 					\
	            ( (tree->rchild->type == H5Z_XFORM_FLOAT) || (tree->rchild->type == H5Z_XFORM_INTEGER)))					\
	{																	\
	    tree->type = H5Z_XFORM_FLOAT;													\
            tree->value.float_val = ((tree->lchild->type == H5Z_XFORM_FLOAT) ? tree->lchild->value.float_val : (double)tree->lchild->value.int_val) OP 	\
				    ((tree->rchild->type == H5Z_XFORM_FLOAT) ? tree->rchild->value.float_val : (double)tree->rchild->value.int_val);	\
            H5MM_xfree(tree->lchild);														\
            H5MM_xfree(tree->rchild);														\
            tree->lchild = NULL;														\
            tree->rchild = NULL;														\
        }																	\
}

/*
 *  Programmer: Bill Wendling <wendling@ncsa.uiuc.edu>
 *              25. August 2003
 */

/*
 * This is the context-free grammar for our expressions:
 *
 * expr     :=  term    | term '+ term      | term '-' term
 * term     :=  factor  | factor '*' factor | factor '/' factor
 * factor   :=  number      |
 *              symbol      |
 *              '-' factor  |   // unary minus
 *              '+' factor  |   // unary plus
 *              '(' expr ')'
 * symbol   :=  [a-zA-Z][a-zA-Z0-9]*
 * number   :=  H5Z_XFORM_INTEGER | FLOAT
 *      // H5Z_XFORM_INTEGER is a C long int
 *      // FLOAT is a C double
 */


/*-------------------------------------------------------------------------
 * Function:    H5Z_unget_token
 * Purpose:     Rollback the H5Z_token to the previous H5Z_token retrieved. There
 *              should only need to be one level of rollback necessary
 *              for our grammar.
 * Return:      Always succeeds.
 * Programmer:  Bill Wendling
 *              26. August 2003
 * Modifications:
  *              Leon Arber:  Added FUNC_ENTER / FUNC_LEAVE pairs
*
 *-------------------------------------------------------------------------
 */
static void
H5Z_unget_token(H5Z_token *current)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(current);

    current->tok_type = current->tok_last_type;
    current->tok_begin = current->tok_last_begin;
    current->tok_end = current->tok_last_end;

    FUNC_LEAVE_NOAPI_VOID
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_get_token
 *
 * Purpose:     Determine what the next valid H5Z_token is in the expression
 *              string. The current position within the H5Z_token string is
 *              kept internal to the H5Z_token and handled by this and the
 *              unget_H5Z_token function.
 *
 * Return:      Succeess:       The passed in H5Z_token with a valid tok_type
 *                              field.
 *              Failure:        The passed in H5Z_token but with the tok_type
 *                              field set to ERROR.
 *
 * Programmer:  Bill Wendling
 *              26. August 2003
 *
 *-------------------------------------------------------------------------
 */
static H5Z_token *
H5Z_get_token(H5Z_token *current)
{
    H5Z_token *ret_value = current;

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(current);

    /* Save the last position for possible ungets */
    current->tok_last_type = current->tok_type;
    current->tok_last_begin = current->tok_begin;
    current->tok_last_end = current->tok_end;

    current->tok_begin = current->tok_end;

    while (current->tok_begin[0] != '\0') {
        if (HDisspace(current->tok_begin[0])) {
            /* ignore whitespace */
        } else if (HDisdigit(current->tok_begin[0]) ||
                   current->tok_begin[0] == '.') {
            current->tok_end = current->tok_begin;

            /*
             * H5Z_XFORM_INTEGER          :=  digit-sequence
             * digit-sequence   :=  digit | digit digit-sequence
             * digit            :=  0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
             */
            if (current->tok_end[0] != '.') {
                /* is number */
                current->tok_type = H5Z_XFORM_INTEGER;

                while (HDisdigit(current->tok_end[0]))
                    ++current->tok_end;
            }

            /*
             * float            :=  digit-sequence exponent |
             *                      dotted-digits exponent?
             * dotted-digits    :=  digit-sequence '.' digit-sequence?  |
             *                      '.' digit-sequence
             * exponent         :=  [Ee] [-+]? digit-sequence
             */
            if (current->tok_end[0] == '.' ||
                    current->tok_end[0] == 'e' ||
                    current->tok_end[0] == 'E') {
                current->tok_type = H5Z_XFORM_FLOAT;

                if (current->tok_end[0] == '.')
                    do {
                        ++current->tok_end;
                    } while (HDisdigit(current->tok_end[0]));

                if (current->tok_end[0] == 'e' ||
                    current->tok_end[0] == 'E') {
                    ++current->tok_end;

                    if (current->tok_end[0] == '-' ||
                        current->tok_end[0] == '+')
                        ++current->tok_end;

                    if (!HDisdigit(current->tok_end[0])) {
                        current->tok_type = H5Z_XFORM_ERROR;
                        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, current, "Invalidly formatted floating point number")
                    }

                    while (HDisdigit(current->tok_end[0]))
                        ++current->tok_end;
                }

                /* Check that this is a properly formatted numerical value */
                if (HDisalpha(current->tok_end[0]) || current->tok_end[0] == '.') {
                    current->tok_type = H5Z_XFORM_ERROR;
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, current, "Invalidly formatted floating point number")
                }
            }

            break;
        } else if (HDisalpha(current->tok_begin[0])) {
            /* is symbol */
            current->tok_type = H5Z_XFORM_SYMBOL;
            current->tok_end = current->tok_begin;

            while (HDisalnum(current->tok_end[0]))
                ++current->tok_end;

            break;
        } else {
            /* should be +, -, *, /, (, or ) */
            switch (current->tok_begin[0]) {
                case '+':   current->tok_type = H5Z_XFORM_PLUS;    break;
                case '-':   current->tok_type = H5Z_XFORM_MINUS;   break;
                case '*':   current->tok_type = H5Z_XFORM_MULT;    break;
                case '/':   current->tok_type = H5Z_XFORM_DIVIDE;  break;
                case '(':   current->tok_type = H5Z_XFORM_LPAREN;  break;
                case ')':   current->tok_type = H5Z_XFORM_RPAREN;  break;
                default:
                    current->tok_type = H5Z_XFORM_ERROR;
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, current, "Unknown H5Z_token in data transform expression ")
            }

            current->tok_end = current->tok_begin + 1;
            break;
        }

        ++current->tok_begin;
    }

    if (current->tok_begin[0] == '\0')
        current->tok_type = H5Z_XFORM_END;

    /* Set return value */
    ret_value = current;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_xform_destroy_parse_tree
 * Purpose:     Recursively destroys the expression tree.
 * Return:      Nothing
 * Programmer:  Bill Wendling
 *              25. August 2003
 * Modifications:
 *              Leon Arber: Added FUNC_ENTER / FUNC_LEAVE pairs
 *
 *-------------------------------------------------------------------------
 */
static void
H5Z_xform_destroy_parse_tree(H5Z_node *tree)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (tree)
    {
	H5Z_xform_destroy_parse_tree(tree->lchild);
	H5Z_xform_destroy_parse_tree(tree->rchild);
	H5MM_xfree(tree);
	tree = NULL;
    }

    FUNC_LEAVE_NOAPI_VOID
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_parse
 *
 * Purpose:     Entry function for parsing the expression string.
 *
 * Return:      Success:    Valid H5Z_node ptr to an expression tree.
 *              NULLure:    NULL
 *
 * Programmer:  Bill Wendling
 *              26. August 2003
 *
 *-------------------------------------------------------------------------
 */
static void *
H5Z_xform_parse(const char *expression, H5Z_datval_ptrs* dat_val_pointers)
{
    H5Z_token tok;
    void* ret_value;

    FUNC_ENTER_NOAPI(NULL)

    if(!expression)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "No expression provided?")

    /* Set up the initial H5Z_token for parsing */
    tok.tok_expr = tok.tok_begin = tok.tok_end = expression;

    ret_value = (void*)H5Z_parse_expression(&tok, dat_val_pointers);

    H5Z_xform_reduce_tree((H5Z_node*)ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_parse_expression
 * Purpose:     Beginning of the recursive descent parser to parse the
 *              expression. An expression is:
 *
 *                  expr     :=  term | term '+' term | term '-' term
 *
 * Return:      Success:    Valid H5Z_node ptr to expression tree
 *              NULLure:    NULL
 * Programmer:  Bill Wendling
 *              26. August 2003
 * Modifications:
  *              Leon Arber: Added FUNC_ENTER / FUNC_LEAVE pairs
*
 *-------------------------------------------------------------------------
 */
static H5Z_node *
H5Z_parse_expression(H5Z_token *current, H5Z_datval_ptrs* dat_val_pointers)
{
    H5Z_node *expr;
    H5Z_node *ret_value;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    expr = H5Z_parse_term(current, dat_val_pointers);

    for (;;) {
        H5Z_node *new_node;

        current = H5Z_get_token(current);

        switch(current->tok_type) {
            case H5Z_XFORM_PLUS:
                new_node = H5Z_new_node(H5Z_XFORM_PLUS);

                if (!new_node) {
                    H5Z_xform_destroy_parse_tree(expr);
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")
                }

                new_node->lchild = expr;
                new_node->rchild = H5Z_parse_term(current, dat_val_pointers);

                if (!new_node->rchild) {
                    H5Z_xform_destroy_parse_tree(new_node);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }

                expr = new_node;
                break;

            case H5Z_XFORM_MINUS:
                new_node = H5Z_new_node(H5Z_XFORM_MINUS);

                if (!new_node) {
                    H5Z_xform_destroy_parse_tree(expr);
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")
                }

                new_node->lchild = expr;
                new_node->rchild = H5Z_parse_term(current, dat_val_pointers);

                if (!new_node->rchild) {
                    H5Z_xform_destroy_parse_tree(new_node);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }

                expr = new_node;
                break;

            case H5Z_XFORM_RPAREN:
                H5Z_unget_token(current);
                HGOTO_DONE(expr)

            case H5Z_XFORM_END:
                HGOTO_DONE(expr)

            case H5Z_XFORM_ERROR:
            case H5Z_XFORM_INTEGER:
            case H5Z_XFORM_FLOAT:
            case H5Z_XFORM_SYMBOL:
            case H5Z_XFORM_MULT:
            case H5Z_XFORM_DIVIDE:
            case H5Z_XFORM_LPAREN:
            default:
                H5Z_xform_destroy_parse_tree(expr);
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_parse_term
 * Purpose:     Parses a term in our expression language. A term is:
 *
 *                  term :=  factor | factor '*' factor | factor '/' factor
 *
 * Return:      Success:    Valid H5Z_node ptr to expression tree
 *              NULLure:    NULL
 * Programmer:  Bill Wendling
 *              26. August 2003
 * Modifications:
  *              Leon Arber: Added FUNC_ENTER / FUNC_LEAVE pairs
*
 *-------------------------------------------------------------------------
 */
static H5Z_node *
H5Z_parse_term(H5Z_token *current, H5Z_datval_ptrs* dat_val_pointers)
{
    H5Z_node *term = NULL;
    H5Z_node *ret_value;

    FUNC_ENTER_NOAPI_NOINIT

    term = H5Z_parse_factor(current, dat_val_pointers);

    for (;;) {
        H5Z_node *new_node;

        current = H5Z_get_token(current);

        switch (current->tok_type) {
            case H5Z_XFORM_MULT:
                new_node = H5Z_new_node(H5Z_XFORM_MULT);

                if (!new_node) {
                    H5Z_xform_destroy_parse_tree(term);
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")
                }

                new_node->lchild = term;
                new_node->rchild = H5Z_parse_factor(current, dat_val_pointers);

                if (!new_node->rchild) {
                    H5Z_xform_destroy_parse_tree(new_node);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }

                term = new_node;
                break;

            case H5Z_XFORM_DIVIDE:
                new_node = H5Z_new_node(H5Z_XFORM_DIVIDE);

                if (!new_node) {
                    H5Z_xform_destroy_parse_tree(term);
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")
                }

                new_node->lchild = term;
                new_node->rchild = H5Z_parse_factor(current, dat_val_pointers);
                term = new_node;

                if (!new_node->rchild) {
                    H5Z_xform_destroy_parse_tree(new_node);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }
                break;

            case H5Z_XFORM_RPAREN:
                H5Z_unget_token(current);
                HGOTO_DONE(term)

            case H5Z_XFORM_END:
                HGOTO_DONE(term)

            default:
                H5Z_unget_token(current);
                HGOTO_DONE(term)
	}
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_parse_factor
 * Purpose:     Parses a factor in our expression language. A factor is:
 *
 *                  factor   :=  number      |  // C long or double
 *                               symbol      |  // C identifier
 *                               '-' factor  |  // unary minus
 *                               '+' factor  |  // unary plus
 *                               '(' expr ')'
 *
 * Return:      Success:    Valid H5Z_node ptr to expression tree
 *              NULLure:    NULL
 * Programmer:  Bill Wendling
 *              26. August 2003
 * Modifications:
 *              Leon Arber: Added FUNC_ENTER / FUNC_LEAVE pairs
 *
 *-------------------------------------------------------------------------
 */
static H5Z_node *
H5Z_parse_factor(H5Z_token *current, H5Z_datval_ptrs* dat_val_pointers)
{
    H5Z_node 	*factor=NULL;
    H5Z_node 	*new_node;
    H5Z_node    *ret_value;

    FUNC_ENTER_NOAPI_NOINIT

    current = H5Z_get_token(current);

    switch (current->tok_type) {
	case H5Z_XFORM_INTEGER:
	    factor = H5Z_new_node(H5Z_XFORM_INTEGER);

	    if (!factor)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")
            sscanf(current->tok_begin, "%ld", &factor->value.int_val);
	    break;

	case H5Z_XFORM_FLOAT:
	    factor = H5Z_new_node(H5Z_XFORM_FLOAT);

	    if (!factor)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")
            sscanf(current->tok_begin, "%lf", &factor->value.float_val);
	    break;

	case H5Z_XFORM_SYMBOL:
	    factor = H5Z_new_node(H5Z_XFORM_SYMBOL);

	    if (!factor)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")

            factor->value.dat_val = &(dat_val_pointers->ptr_dat_val[dat_val_pointers->num_ptrs]);
	    dat_val_pointers->num_ptrs++;
	    break;

	case H5Z_XFORM_LPAREN:
	    factor = H5Z_parse_expression(current, dat_val_pointers);

	    if (!factor)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Unable to allocate new node")

            current = H5Z_get_token(current);

	    if (current->tok_type != H5Z_XFORM_RPAREN) {
		H5Z_xform_destroy_parse_tree(factor);
		HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Syntax error in data transform expression")
	    }
	    break;

	case H5Z_XFORM_RPAREN:
	    /* We shouldn't see a ) right now */
	    H5Z_xform_destroy_parse_tree(factor);
	    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Syntax error: unexpected ')' ")

	case H5Z_XFORM_PLUS:
            /* unary + */
            new_node = H5Z_parse_factor(current, dat_val_pointers);

            if (new_node) {
                if (new_node->type != H5Z_XFORM_INTEGER && new_node->type != H5Z_XFORM_FLOAT &&
                        new_node->type != H5Z_XFORM_SYMBOL) {
                    H5Z_xform_destroy_parse_tree(new_node);
                    H5Z_xform_destroy_parse_tree(factor);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }

                factor = new_node;
                new_node = H5Z_new_node(H5Z_XFORM_PLUS);

                if (!new_node) {
                    H5Z_xform_destroy_parse_tree(factor);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }

                new_node->rchild = factor;
                factor = new_node;
            } else {
                H5Z_xform_destroy_parse_tree(factor);
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
            }
            break;

	case H5Z_XFORM_MINUS:
            /* unary - */
            new_node = H5Z_parse_factor(current, dat_val_pointers);

            if (new_node) {
                if (new_node->type != H5Z_XFORM_INTEGER && new_node->type != H5Z_XFORM_FLOAT &&
                        new_node->type != H5Z_XFORM_SYMBOL) {
                    H5Z_xform_destroy_parse_tree(new_node);
                    H5Z_xform_destroy_parse_tree(factor);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }

                factor = new_node;
                new_node = H5Z_new_node(H5Z_XFORM_MINUS);

                if (!new_node) {
                    H5Z_xform_destroy_parse_tree(factor);
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
                }

                new_node->rchild = factor;
                factor = new_node;
            } else {
                H5Z_xform_destroy_parse_tree(factor);
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error parsing data transform expression")
            }
            break;

	case H5Z_XFORM_END:
            break;

	default:
	    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Invalid token while parsing data transform expression")

    }

    /* Set return value */
    ret_value=factor;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_new_node
 * Purpose:     Create and initilize a new H5Z_node structure.
 * Return:      Success:    Valid H5Z_node ptr
 *              NULLure:    NULL
 * Programmer:  Bill Wendling
 *              26. August 2003
 * Modifications:
 *              Leon Arber: Added FUNC_ENTER / FUNC_LEAVE pairs
 *
 *-------------------------------------------------------------------------
 */
static H5Z_node *
H5Z_new_node(H5Z_token_type type)
{
    H5Z_node *ret_value;

    FUNC_ENTER_NOAPI_NOINIT

    if(NULL == (ret_value = (H5Z_node *)H5MM_calloc(sizeof(H5Z_node))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Ran out of memory trying to allocate space for nodes in the parse tree")

    ret_value->type = type;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_xform_eval
 * Purpose: 	If the transform is trivial, this function applies it.
 * 		Otherwise, it calls H5Z_xform_eval_full to do the full
 * 		transform.
 * Return:      SUCCEED if transform applied succesfully, FAIL otherwise
 * Programmer:  Leon Arber
 * 		5/1/04
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Z_xform_eval(H5Z_data_xform_t *data_xform_prop, void* array, size_t array_size, const H5T_t *buf_type)
{
    H5Z_node *tree;
    hid_t array_type;
    H5Z_result res;
    size_t i;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(data_xform_prop);

    tree = data_xform_prop->parse_root;

    /* Get the datatype ID for the buffer's type */
    if((array_type = H5Z_xform_find_type(buf_type)) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Cannot perform data transform on this type.")

    /* After this point, we're assured that the type of the array is handled by the eval code,
     *  so we no longer have to check for valid types
     */

    /* If it's a trivial data transform, perform it */
    if(tree->type == H5Z_XFORM_INTEGER || tree->type == H5Z_XFORM_FLOAT) {
        if(array_type == H5T_NATIVE_CHAR)
            H5Z_XFORM_DO_OP5(char, array_size)
        else if(array_type ==  H5T_NATIVE_UCHAR)
            H5Z_XFORM_DO_OP5(unsigned char, array_size)
        else if(array_type == H5T_NATIVE_SCHAR)
            H5Z_XFORM_DO_OP5(signed char, array_size)
        else if(array_type == H5T_NATIVE_SHORT)
            H5Z_XFORM_DO_OP5(short, array_size)
        else if(array_type == H5T_NATIVE_USHORT)
            H5Z_XFORM_DO_OP5(unsigned short, array_size)
        else if(array_type == H5T_NATIVE_INT)
            H5Z_XFORM_DO_OP5(int, array_size)
        else if(array_type ==  H5T_NATIVE_UINT)
            H5Z_XFORM_DO_OP5(unsigned int, array_size)
        else if(array_type == H5T_NATIVE_LONG)
            H5Z_XFORM_DO_OP5(long, array_size)
        else if(array_type == H5T_NATIVE_ULONG)
            H5Z_XFORM_DO_OP5(unsigned long, array_size)
        else if(array_type == H5T_NATIVE_LLONG)
            H5Z_XFORM_DO_OP5(long long, array_size)
        else if(array_type == H5T_NATIVE_ULLONG)
            H5Z_XFORM_DO_OP5(unsigned long long, array_size)
        else if(array_type == H5T_NATIVE_FLOAT)
            H5Z_XFORM_DO_OP5(float, array_size)
	else if(array_type == H5T_NATIVE_DOUBLE)
            H5Z_XFORM_DO_OP5(double, array_size)
#if H5_SIZEOF_LONG_DOUBLE !=0
	else if(array_type == H5T_NATIVE_LDOUBLE)
            H5Z_XFORM_DO_OP5(long double, array_size)
#endif

    } /* end if */
    /* Otherwise, do the full data transform */
    else {
	/* Optimization for linear transform: */
	if(data_xform_prop->dat_val_pointers->num_ptrs == 1)
	    data_xform_prop->dat_val_pointers->ptr_dat_val[0] = array;

	/* If it's a quadratic transform, we have no choice but to store multiple copies of the data */
	else {
	    for(i = 0; i < data_xform_prop->dat_val_pointers->num_ptrs; i++) {
		if(NULL == (data_xform_prop->dat_val_pointers->ptr_dat_val[i] = (void*)H5MM_malloc(array_size * H5T_get_size((H5T_t *)H5I_object(array_type)))))
		    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "Ran out of memory trying to allocate space for data in data transform")

                HDmemcpy(data_xform_prop->dat_val_pointers->ptr_dat_val[i], array, array_size * H5T_get_size((H5T_t *)H5I_object(array_type)));
	    } /* end for */
	} /* end else */

	if(H5Z_xform_eval_full(tree, array_size, array_type, &res) < 0)
	    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "error while performing data transform")

	if(data_xform_prop->dat_val_pointers->num_ptrs > 1)
	    HDmemcpy(array, res.value.dat_val, array_size * H5T_get_size((H5T_t *)H5I_object(array_type)));

        /* Free the temporary arrays we used */
        if(data_xform_prop->dat_val_pointers->num_ptrs > 1)
            for(i=0; i<data_xform_prop->dat_val_pointers->num_ptrs; i++)
                HDfree(data_xform_prop->dat_val_pointers->ptr_dat_val[i]);
    } /* end else */

done:
    if(ret_value < 0) {
	/* If we ran out of memory above copying the array for temp storage (which we easily can for
	 * polynomial transforms of high order) we free those arrays which we already allocated */
	if(data_xform_prop->dat_val_pointers->num_ptrs > 1)
	    for(i = 0; i < data_xform_prop->dat_val_pointers->num_ptrs; i++)
		if(data_xform_prop->dat_val_pointers->ptr_dat_val[i])
		    HDfree(data_xform_prop->dat_val_pointers->ptr_dat_val[i]);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5Z_xform_eval() */


/*-------------------------------------------------------------------------
 * Function:    H5Z_xform_eval_full
 * Purpose: 	Does a full evaluation of the parse tree contained in tree
 * 		and applies this transform to array.
 * Return:      Nothing
 * Programmer:  Leon Arber
 * 		5/1/04
 * Modifications:
 *
 *
 * Notes:   In the case of a polynomial data transform (ie, the left and right subtree
 * are both of type H5Z_XFORM_SYMBOL), the convention is that the left hand side
 * will accumulate changes and, at the end, the new data will be copied from the lhs.
 *-------------------------------------------------------------------------
 */
static herr_t
H5Z_xform_eval_full(H5Z_node *tree, const size_t array_size,  const hid_t array_type, H5Z_result *res)
{
    H5Z_result resl, resr;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(tree);

    HDmemset(&resl, 0, sizeof(H5Z_result));
    HDmemset(&resr, 0, sizeof(H5Z_result));

    if (tree->type == H5Z_XFORM_INTEGER) {
	res->type = H5Z_XFORM_INTEGER;
	res->value.int_val = tree->value.int_val;
    } /* end if */
    else if (tree->type == H5Z_XFORM_FLOAT) {
	res->type = H5Z_XFORM_FLOAT;
	res->value.float_val = tree->value.float_val;
    } /* end if */
    else if (tree->type == H5Z_XFORM_SYMBOL) {
	res->type = H5Z_XFORM_SYMBOL;

	/*since dat_val stores the address of the array which is really stored in the dat_val_pointers,
	 * here we make dat_val store a pointer to the array itself instead of the address of it so that the
	 * rest of the code below works normally. */
	res->value.dat_val = *((void**)(tree->value.dat_val));
    } /* end if */
    else {
	if(tree->lchild && H5Z_xform_eval_full(tree->lchild, array_size, array_type, &resl) < 0)
	    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "error while performing data transform")
	if(H5Z_xform_eval_full(tree->rchild, array_size, array_type, &resr) < 0)
	    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "error while performing data transform")

	res->type = H5Z_XFORM_SYMBOL;

	/* For each type of operation:
	 * 1.  See if "x" is on left hand side, right hand side, or if both sides are "x"
	 * 2.  Figure out what type of data we're going to be manipulating
	 * 3.  Do the operation on the data. */


	switch (tree->type) {
	    case H5Z_XFORM_PLUS:
	 	H5Z_XFORM_TYPE_OP(resl, resr, array_type, +, array_size)
		break;

	    case H5Z_XFORM_MINUS:
	 	H5Z_XFORM_TYPE_OP(resl, resr, array_type, -, array_size)
		break;

	    case H5Z_XFORM_MULT:
		H5Z_XFORM_TYPE_OP(resl, resr, array_type, *, array_size)
		break;

	    case H5Z_XFORM_DIVIDE:
	 	H5Z_XFORM_TYPE_OP(resl, resr, array_type, /, array_size)
		break;

	    default:
		HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Invalid expression tree")
	} /* end switch */

	/* The result stores a pointer to the new data */
	/* So, if the left hand side got its data modified, the result stores a pointers
	 * to the left hand side's data, ditto for rhs */
	if(resl.type ==  H5Z_XFORM_SYMBOL)
	    res->value.dat_val = resl.value.dat_val;
	else if(resr.type == H5Z_XFORM_SYMBOL)
	    res->value.dat_val = resr.value.dat_val;
	else
	    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "error during transform evaluation")
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5Z_xform_eval_full() */


/*-------------------------------------------------------------------------
 * Function:    H5Z_find_type
 * Return:      Native type of datatype that is passed in
 * Programmer:  Leon Arber, 4/20/04
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
static hid_t
H5Z_xform_find_type(const H5T_t* type)
{
    H5T_t *tmp;                         /* Temporary datatype */
    hid_t ret_value = SUCCEED;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(type);

    /* Check for SHORT type */
    if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_SHORT)) 
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_SHORT)
    /* Check for INT type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_INT)) 
            && 0 == H5T_cmp(type, tmp, FALSE))
        HGOTO_DONE(H5T_NATIVE_INT)
    /* Check for LONG type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_LONG)) 
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_LONG)
    /* Check for LONGLONG type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_LLONG))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_LLONG)
    /* Check for UCHAR type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_UCHAR))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_UCHAR)
    /* Check for CHAR type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_CHAR))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_CHAR)
    /* Check for SCHAR type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_SCHAR))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_SCHAR)
    /* Check for USHORT type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_USHORT))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_USHORT)
    /* Check for UINT type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_UINT))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_UINT)
    /* Check for ULONG type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_ULONG))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_ULONG)
    /* Check for ULONGLONG type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_ULLONG))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_ULLONG)
    /* Check for FLOAT type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_FLOAT))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_FLOAT)
    /* Check for DOUBLE type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_DOUBLE))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_DOUBLE)
#if H5_SIZEOF_LONG_DOUBLE !=0
    /* Check for LONGDOUBLE type */
    else if((tmp = (H5T_t *)H5I_object(H5T_NATIVE_LDOUBLE))
            && 0 == H5T_cmp(type, tmp, FALSE))
	HGOTO_DONE(H5T_NATIVE_LDOUBLE)
#endif
    else
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "could not find matching type")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5Z_xform_find_type() */


/*-------------------------------------------------------------------------
 * Function:    H5Z_xform_copy_tree
 * Purpose:     Makes a copy of the parse tree passed in.
 * Return:      A pointer to a root for a new parse tree which is a copy
 *              of the one passed in.
 * Programmer:  Leon Arber
 *              April 1, 2004.
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5Z_xform_copy_tree(H5Z_node* tree, H5Z_datval_ptrs* dat_val_pointers, H5Z_datval_ptrs* new_dat_val_pointers)
{
    H5Z_node* ret_value=NULL;

    FUNC_ENTER_NOAPI(NULL)

    HDassert(tree);

    if(tree->type == H5Z_XFORM_INTEGER)
    {
        if ((ret_value = (H5Z_node*) H5MM_malloc(sizeof(H5Z_node))) == NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Ran out of memory trying to copy parse tree")
        else
        {
	    ret_value -> type = H5Z_XFORM_INTEGER;
	    ret_value ->value.int_val = tree->value.int_val;
	    ret_value -> lchild = NULL;
	    ret_value -> rchild = NULL;
	}
    }
    else if (tree->type == H5Z_XFORM_FLOAT)
    {
        if ((ret_value = (H5Z_node*) H5MM_malloc(sizeof(H5Z_node))) == NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Ran out of memory trying to copy parse tree")
        else
	{
	    ret_value -> type = H5Z_XFORM_FLOAT;
	    ret_value ->value.float_val = tree->value.float_val;
	    ret_value -> lchild = NULL;
	    ret_value -> rchild = NULL;
	}
    }
    else if(tree->type == H5Z_XFORM_SYMBOL)
    {
        if ((ret_value = (H5Z_node*) H5MM_malloc(sizeof(H5Z_node))) == NULL)
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "Ran out of memory trying to copy parse tree")
	else
	{
	    ret_value -> type = H5Z_XFORM_SYMBOL;

	    ret_value -> value.dat_val = &(new_dat_val_pointers->ptr_dat_val[new_dat_val_pointers->num_ptrs]);
	    new_dat_val_pointers->num_ptrs++;
	    ret_value -> lchild = NULL;
	    ret_value -> rchild = NULL;
	}
    }
    else if(tree->type == H5Z_XFORM_MULT)
        H5Z_XFORM_DO_OP4(H5Z_XFORM_MULT)
    else if(tree->type == H5Z_XFORM_PLUS)
        H5Z_XFORM_DO_OP4(H5Z_XFORM_PLUS)
    else if(tree->type == H5Z_XFORM_MINUS)
        H5Z_XFORM_DO_OP4(H5Z_XFORM_MINUS)
    else if(tree->type == H5Z_XFORM_DIVIDE)
        H5Z_XFORM_DO_OP4(H5Z_XFORM_DIVIDE)

    else
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "Error in parse tree while trying to copy")



            done:
            FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_op_is_numbs
 * Purpose:     Internal function to facilitate the condition check in 
 *              H5Z_xform_reduce_tree to reduce the bulkiness of the code.
 * Return:      TRUE or FALSE
 * Programmer:  Raymond Lu
 *              15 March 2012
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5Z_op_is_numbs(H5Z_node* _tree)
{
    hbool_t ret_value = FALSE;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(_tree);

    if(((_tree->lchild->type == H5Z_XFORM_INTEGER) || (_tree->lchild->type == H5Z_XFORM_FLOAT)) && ((_tree->rchild->type == H5Z_XFORM_INTEGER) || (_tree->rchild->type == H5Z_XFORM_FLOAT)))
      ret_value = TRUE;

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_op_is_numbs2
 * Purpose:     Internal function to facilitate the condition check in 
 *              H5Z_xform_reduce_tree to reduce the bulkiness of the code.
 *              The difference from H5Z_op_is_numbs is that the left child
 *              can be empty, like -x or +x.
 * Return:      TRUE or FALSE
 * Programmer:  Raymond Lu
 *              15 March 2012
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hbool_t
H5Z_op_is_numbs2(H5Z_node* _tree)
{
    hbool_t ret_value = FALSE;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(_tree);

    if((!_tree->lchild && ((_tree->rchild->type == H5Z_XFORM_INTEGER) || (_tree->rchild->type == H5Z_XFORM_FLOAT))) ||
       ((_tree->lchild && ((_tree->lchild->type == H5Z_XFORM_INTEGER) || (_tree->lchild->type == H5Z_XFORM_FLOAT))) && (_tree->rchild && ((_tree->rchild->type == H5Z_XFORM_INTEGER) || (_tree->rchild->type == H5Z_XFORM_FLOAT)))))
      ret_value = TRUE;

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_xform_reduce_tree
 * Purpose:     Simplifies parse tree passed in by performing any obvious
 *              and trivial arithemtic calculations.
 *
 * Return:      None.
 * Programmer:  Leon Arber
 *              April 1, 2004.
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
H5Z_xform_reduce_tree(H5Z_node* tree)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(tree) {
        if((tree->type == H5Z_XFORM_DIVIDE) || (tree->type == H5Z_XFORM_MULT))
        {
            if(H5Z_op_is_numbs(tree))
                H5Z_do_op(tree);
            else
            {
                H5Z_xform_reduce_tree(tree->lchild);
                if(H5Z_op_is_numbs(tree))
                    H5Z_do_op(tree);
                else {
                    H5Z_xform_reduce_tree(tree->rchild);
                    if(H5Z_op_is_numbs(tree))
                        H5Z_do_op(tree);
                }
            }
        } else if((tree->type == H5Z_XFORM_PLUS) || (tree->type == H5Z_XFORM_MINUS)) {
            if(H5Z_op_is_numbs2(tree))
                H5Z_do_op(tree);
            else
            {
                H5Z_xform_reduce_tree(tree->lchild);
                if(H5Z_op_is_numbs2(tree))
                    H5Z_do_op(tree);
                else {
                    H5Z_xform_reduce_tree(tree->rchild);
                    if(H5Z_op_is_numbs2(tree))
                        H5Z_do_op(tree);
                }
            }
        }

    }

    FUNC_LEAVE_NOAPI_VOID;
}


/*-------------------------------------------------------------------------
 * Function:    H5Z_do_op
 * Purpose:     If the root of the tree passed in points to a simple
 *              arithmetic operation and the left and right subtrees are both
 *              integer or floating point values, this function does that
 *              operation, free the left and rigt subtrees, and replaces
 *              the root with the result of the operation.
 * Return:      None.
 * Programmer:  Leon Arber
 *              April 1, 2004.
 * Modifications:
 *              Raymond Lu
 *              15 March 2012
 *              I added a new macro H5Z_XFORM_DO_OP6 to handle the special
 *              operations like -x or +x when the left operand is empty.
 *
 *-------------------------------------------------------------------------
 */
static void
H5Z_do_op(H5Z_node* tree)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(tree->type == H5Z_XFORM_DIVIDE)
	H5Z_XFORM_DO_OP3(/)
    else if(tree->type == H5Z_XFORM_MULT)
	H5Z_XFORM_DO_OP3(*)
    else if(tree->type == H5Z_XFORM_PLUS)
	H5Z_XFORM_DO_OP6(+)
    else if(tree->type == H5Z_XFORM_MINUS)
	H5Z_XFORM_DO_OP6(-)

    FUNC_LEAVE_NOAPI_VOID;
}


/*-------------------------------------------------------------------------
 * Function: H5D_xform_create
 *
 * Purpose: Create a new data transform object from a string.
 *
 * Return:
 *      Success: SUCCEED
 *      Failure: FAIL
 *
 * Programmer: Quincey Koziol, koziol@ncsa.uiuc.edu
 *
 * Date: May 4, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5Z_data_xform_t *
H5Z_xform_create(const char *expr)
{
    H5Z_data_xform_t *data_xform_prop=NULL;
    H5Z_data_xform_t *ret_value;
    unsigned int i;
    unsigned int count = 0;

    FUNC_ENTER_NOAPI(NULL)

    HDassert(expr);

    /* Allocate space for the data transform information */
    if(NULL == (data_xform_prop = (H5Z_data_xform_t *)H5MM_calloc(sizeof(H5Z_data_xform_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate memory for data transform info")

    if(NULL == (data_xform_prop->dat_val_pointers = (H5Z_datval_ptrs *)H5MM_malloc(sizeof(H5Z_datval_ptrs))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate memory for data transform array storage")

    /* copy the user's string into the property */
    if(NULL == (data_xform_prop->xform_exp = (char *)H5MM_xstrdup(expr)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate memory for data transform expression")

    /* Find the number of times "x" is used in this equation, and allocate room for storing that many points */
    for(i = 0; i < HDstrlen(expr); i++)
	if(HDisalpha(expr[i]))
	    count++;

    /* When there are no "x"'s in the equation (ie, simple transform case),
     * we don't need to allocate any space since no array will have to be
     * stored */
    if(count > 0)
	if(NULL == (data_xform_prop->dat_val_pointers->ptr_dat_val = (void **)H5MM_calloc(count * sizeof(void *))))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate memory for pointers in transform array")

    /* Initialize the num_ptrs field, which will be used to keep track of the number of copies
     * of the data we have for polynomial transforms */
    data_xform_prop->dat_val_pointers->num_ptrs = 0;

     /* we generate the parse tree right here and store a poitner to its root in the property. */
    if((data_xform_prop->parse_root = (H5Z_node *)H5Z_xform_parse(expr, data_xform_prop->dat_val_pointers))==NULL)
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to generate parse tree from expression")

    /* Sanity check
     * count should be the same num_ptrs */
    if(count != data_xform_prop->dat_val_pointers->num_ptrs)
         HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "error copying the parse tree, did not find correct number of \"variables\"")

    /* Assign return value */
    ret_value=data_xform_prop;


done:
    /* Clean up on error */
    if(ret_value==NULL) {
        if(data_xform_prop) {
            if(data_xform_prop->parse_root)
                H5Z_xform_destroy_parse_tree(data_xform_prop->parse_root);
            if(data_xform_prop->xform_exp)
                H5MM_xfree(data_xform_prop->xform_exp);
	    if(count > 0 && data_xform_prop->dat_val_pointers->ptr_dat_val)
		H5MM_xfree(data_xform_prop->dat_val_pointers->ptr_dat_val);
	    if(data_xform_prop->dat_val_pointers)
		H5MM_xfree(data_xform_prop->dat_val_pointers);
            H5MM_xfree(data_xform_prop);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5Z_xform_create() */


/*-------------------------------------------------------------------------
 * Function: H5Z_xform_destroy
 *
 * Purpose: Destroy a data transform object.
 *
 * Return:
 *      Success: SUCCEED
 *      Failure: FAIL
 *
 * Programmer: Quincey Koziol, koziol@ncsa.uiuc.edu
 *
 * Date: May 4, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Z_xform_destroy(H5Z_data_xform_t *data_xform_prop)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(data_xform_prop) {
	/* Destroy the parse tree */
        H5Z_xform_destroy_parse_tree(data_xform_prop->parse_root);

        /* Free the expression */
        H5MM_xfree(data_xform_prop->xform_exp);

	/* Free the pointers to the temp. arrays, if there are any */
	if(data_xform_prop->dat_val_pointers->num_ptrs > 0)
	    H5MM_xfree(data_xform_prop->dat_val_pointers->ptr_dat_val);

	/* Free the data storage struct */
	H5MM_xfree(data_xform_prop->dat_val_pointers);

        /* Free the node */
        H5MM_xfree(data_xform_prop);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5Z_xform_destroy() */


/*-------------------------------------------------------------------------
 * Function: H5Z_xform_copy
 *
 * Purpose: Clone a data transform object.
 *
 * Return:
 *      Success: SUCCEED
 *      Failure: FAIL
 *
 * Programmer: Quincey Koziol, koziol@ncsa.uiuc.edu
 *
 * Date: May 4, 2004
 *
 * Comments: This is an "in-place" copy, since this routine gets called
 *      after the top-level copy has been performed and this routine finishes
 *      the "deep" part of the copy.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Z_xform_copy(H5Z_data_xform_t **data_xform_prop)
{
    unsigned int i;
    unsigned int count = 0;
    H5Z_data_xform_t *new_data_xform_prop=NULL;
    herr_t ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    if(*data_xform_prop) {
        /* Allocate new node */
        if(NULL == (new_data_xform_prop = (H5Z_data_xform_t *)H5MM_calloc(sizeof(H5Z_data_xform_t))))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory for data transform info")

        /* Copy string */
        if(NULL == (new_data_xform_prop->xform_exp = (char *)H5MM_xstrdup((*data_xform_prop)->xform_exp)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory for data transform expression")

	if(NULL == (new_data_xform_prop->dat_val_pointers = (H5Z_datval_ptrs *)H5MM_malloc(sizeof(H5Z_datval_ptrs))))
	    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory for data transform array storage")

	/* Find the number of times "x" is used in this equation, and allocate room for storing that many points */
	for(i = 0; i < HDstrlen(new_data_xform_prop->xform_exp); i++)
	    if(HDisalpha(new_data_xform_prop->xform_exp[i]))
		count++;

	if(count > 0)
	    if(NULL == (new_data_xform_prop->dat_val_pointers->ptr_dat_val = (void **)H5MM_calloc(count * sizeof(void *))))
		HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory for pointers in transform array")

	/* Zero out num_pointers prior to H5Z_xform_cop_tree call; that call will increment it to the right amount */
	new_data_xform_prop->dat_val_pointers->num_ptrs = 0;

        /* Copy parse tree */
        if((new_data_xform_prop->parse_root = (H5Z_node*)H5Z_xform_copy_tree((*data_xform_prop)->parse_root, (*data_xform_prop)->dat_val_pointers, new_data_xform_prop->dat_val_pointers)) == NULL)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "error copying the parse tree")

	/* Sanity check
	 * count should be the same num_ptrs */
	if(count != new_data_xform_prop->dat_val_pointers->num_ptrs)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "error copying the parse tree, did not find correct number of \"variables\"")

        /* Copy new information on top of old information */
        *data_xform_prop=new_data_xform_prop;
    } /* end if */

done:
    /* Clean up on error */
    if(ret_value<0) {
        if(new_data_xform_prop) {
            if(new_data_xform_prop->parse_root)
                H5Z_xform_destroy_parse_tree(new_data_xform_prop->parse_root);
            if(new_data_xform_prop->xform_exp)
                H5MM_xfree(new_data_xform_prop->xform_exp);
            H5MM_xfree(new_data_xform_prop);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5Z_xform_copy() */


/*-------------------------------------------------------------------------
 * Function: H5Z_xform_noop
 *
 * Purpose: Checks if a data transform will be performed
 *
 * Return:  TRUE for no data transform, FALSE for a data transform
 *
 * Programmer: Quincey Koziol, koziol@ncsa.uiuc.edu
 *
 * Date: May 4, 2004
 *
 * Comments: Can't fail
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5Z_xform_noop(const H5Z_data_xform_t *data_xform_prop)
{
    hbool_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    ret_value=(data_xform_prop ? FALSE : TRUE);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5Z_xform_noop() */


/*-------------------------------------------------------------------------
 * Function: H5Z_xform_extract_xform_str
 *
 * Purpose: Extracts the pointer to the data transform strings from the
 * 		data transform property.`
 * Return:
 *          Pointer to a copy of the string in the data_xform property.
 *
 * Programmer: Leon Arber, larber@ncsa.uiuc.edu
 *
 * Date: Sept. 4, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
const char *
H5Z_xform_extract_xform_str(const H5Z_data_xform_t *data_xform_prop)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* There should be no way that this can be NULL since the function
     * that calls this one checks to make sure it isn't before
     * pasing them */
    HDassert(data_xform_prop);

    FUNC_LEAVE_NOAPI(data_xform_prop->xform_exp)
} /* H5Z_xform_extract_xform_str() */

