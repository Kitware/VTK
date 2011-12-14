/*
 *	Copyright 1997, University Corporation for Atmospheric Research
 *	See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: onstack.h,v 2.7 2006/09/15 20:40:39 ed Exp $ */

#ifndef _ONSTACK_H_
#define _ONSTACK_H_
/* include after ncncconfig.h */
/**
 * This file provides definitions which allow us to
 * "allocate" arrays on the stack where possible.
 * (Where not possible, malloc and free are used.)
 *
 * The macro ALLOC_ONSTACK(name, type, nelems) is used to declare
 * an array of 'type' named 'name' which is 'nelems' long.
 * FREE_ONSTACK(name) is placed at the end of the scope of 'name'
 * to call 'free' if necessary.
 *
 * The macro ALLOC_ONSTACK wraps a call to alloca() on most systems.
 */

#if HAVE_ALLOCA
/*
 * Implementation based on alloca()
 */

#if defined(__GNUC__)
# if !defined(alloca)
# define alloca __builtin_alloca
# endif
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# elif defined(_AIX)
#  pragma alloca
# endif /* HAVE_ALLOCA_H */
#endif /* __GNUC__ */

# if !defined(ALLOCA_ARG_T)
# define ALLOCA_ARG_T int /* the usual type of the alloca argument */
# endif

# define ALLOC_ONSTACK(name, type, nelems) \
        type *const name = (type *) alloca((ALLOCA_ARG_T)((nelems) * sizeof(type)))

# define FREE_ONSTACK(name)

#elif defined(_CRAYC) && !defined(__crayx1) && !__cplusplus && __STDC__ > 1
/*
 * Cray C allows sizing of arrays with non-constant values.
 */

# define ALLOC_ONSTACK(name, type, nelems) \
        type name[nelems]

# define FREE_ONSTACK(name)

#else
/*
 * Default implementation. When all else fails, use malloc/free.
 */

# define ALLOC_ONSTACK(name, type, nelems) \
        type *const name = (type *) malloc((nelems) * sizeof(type))

# define FREE_ONSTACK(name) \
        free(name)

#endif

#endif /* _ONSTACK_H_ */
