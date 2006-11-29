/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* Id */

#include "nc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "ncx.h"
#include "rnd.h"


/*
 * Free string, and, if needed, its values.
 * Formerly
NC_free_string()
 */
void
free_NC_string(NC_string *ncstrp)
{
  if(ncstrp==NULL)
    return;
  free(ncstrp);
}


/*
 * Verify that a name string is valid
 * CDL syntax, eg, all the characters are
 * alphanumeric, '-', '_', '+', or '.'.
 * Also permit ':', '@', '(', or ')' in names for chemists currently making 
 * use of these characters, but don't document until ncgen and ncdump can 
 * also handle these characters in names.
 */
int
NC_check_name(const char *name)
{
  const char *cp = name;
  assert(name != NULL);

  if(*name == 0)
    return NC_EBADNAME; /* empty names disallowed */

  for(; *cp != 0; cp++)
  {
    int ch = *cp;
    if(!isalnum(ch))
    {
        if(ch != '_' && ch != '-' && ch != '+' && ch != '.' && 
           ch != ':' && ch != '@' && ch != '(' && ch != ')')
        return NC_EBADNAME;
    }
  }
  if(cp - name > NC_MAX_NAME)
    return NC_EMAXNAME;

  return NC_NOERR;
}


/*
 * Allocate a NC_string structure large enough
 * to hold slen characters.
 * Formerly
NC_new_string(count, str)
 */
NC_string *
new_NC_string(size_t slen, const char *str)
{
  NC_string *ncstrp;
  size_t sz = M_RNDUP(sizeof(NC_string)) + slen + 1;

#if 0
  sz = _RNDUP(sz, X_ALIGN);
#endif
    
  ncstrp = (NC_string *)malloc(sz);
  if( ncstrp == NULL )
    return NULL;
  (void) memset(ncstrp, 0, sz);

  ncstrp->nchars = sz - M_RNDUP(sizeof(NC_string)) - 1;
  assert(ncstrp->nchars + 1 > slen);
  ncstrp->cp = (char *)ncstrp + M_RNDUP(sizeof(NC_string));

  if(str != NULL && *str != 0)
  {
    (void) strncpy(ncstrp->cp, str, ncstrp->nchars +1);
    ncstrp->cp[ncstrp->nchars] = 0;
  }
  
  return(ncstrp);
}


/*
 * If possible, change the value of an NC_string to 'str'.
 *
 * Formerly
NC_re_string()
 */
int
set_NC_string(NC_string *ncstrp, const char *str)
{
  size_t slen;
  size_t diff;

  assert(str != NULL && *str != 0);

  slen = strlen(str);

  if(ncstrp->nchars < slen)
    return NC_ENOTINDEFINE;

  (void) memcpy(ncstrp->cp, str, slen);
  diff = ncstrp->nchars - slen;
  if(diff != 0)
    (void) memset(ncstrp->cp + slen, 0, diff);

  return NC_NOERR;
}
