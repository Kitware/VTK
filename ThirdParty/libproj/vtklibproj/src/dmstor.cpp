/* Convert DMS string to radians */

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "proj.h"
#include "proj_internal.h"

static double proj_strtod(char *nptr, char **endptr);

/* following should be sufficient for all but the ridiculous */
#define MAX_WORK 64
	static const char
*sym = "NnEeSsWw";
	static const double
vm[] = {
	DEG_TO_RAD,
	.0002908882086657216,
	.0000048481368110953599
};
	double
dmstor(const char *is, char **rs) {
	return dmstor_ctx( pj_get_default_ctx(), is, rs );
}

	double
dmstor_ctx(PJ_CONTEXT *ctx, const char *is, char **rs) {
	int n, nl;
	char *s, work[MAX_WORK];
        const char* p;
	double v, tv;

	if (rs)
		*rs = (char *)is;
	/* copy sting into work space */
	while (isspace(*is)) ++is;
	n = MAX_WORK;
	s = work;
	p = (char *)is;
	while (isgraph(*p) && --n)
		*s++ = *p++;
	*s = '\0';
	/* it is possible that a really odd input (like lots of leading
		zeros) could be truncated in copying into work.  But ... */
	int sign = *(s = work);
	if (sign == '+' || sign == '-') s++;
	else sign = '+';
	v = 0.;
	for (nl = 0 ; nl < 3 ; nl = n + 1 ) {
		if (!(isdigit(*s) || *s == '.')) break;
		if ((tv = proj_strtod(s, &s)) == HUGE_VAL)
			return tv;
		switch (*s) {
		case 'D': case 'd':
			n = 0; break;
		case '\'':
			n = 1; break;
		case '"':
			n = 2; break;
		case 'r': case 'R':
			if (nl) {
				proj_context_errno_set( ctx, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE );
				return HUGE_VAL;
			}
			++s;
			v = tv;
			goto skip;
		default:
			v += tv * vm[nl];
		skip:	n = 4;
			continue;
		}
		if (n < nl) {
			proj_context_errno_set( ctx, PROJ_ERR_INVALID_OP_ILLEGAL_ARG_VALUE );
			return HUGE_VAL;
		}
		v += tv * vm[n];
		++s;
	}
		/* postfix sign */
	if (*s && (p = strchr(sym, *s))) {
		sign = (p - sym) >= 4 ? '-' : '+';
		++s;
	}
	if (sign == '-')
		v = -v;
	if (rs) /* return point of next char after valid string */
		*rs = (char *)is + (s - work);
	return v;
}

static double
proj_strtod(char *nptr, char **endptr) 

{
    char c, *cp = nptr;
    double result;

    /*
     * Scan for characters which cause problems with VC++ strtod()
     */
    while ((c = *cp) != '\0') {
        if (c == 'd' || c == 'D') {

            /*
             * Found one, so NUL it out, call strtod(),
             * then restore it and return
             */
            *cp = '\0';
            result = strtod(nptr, endptr);
            *cp = c;
            return result;
        }
        ++cp;
    }

    /* no offending characters, just handle normally */

    return pj_strtod(nptr, endptr);
}

