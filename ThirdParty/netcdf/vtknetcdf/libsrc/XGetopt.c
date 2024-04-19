// XGetopt.cpp  Version 1.2
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// Description:
//     XGetopt.cpp implements getopt(), a function to parse command lines.
//
// History
//     Version 1.2 - 2003 May 17
//     - Added Unicode support
//
//     Version 1.1 - 2002 March 10
//     - Added example to XGetopt.cpp module header 
//
// This software is released into the public domain.
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// if you are not using precompiled headers then include these lines:

///////////////////////////////////////////////////////////////////////////////


#ifndef DLL_EXPORT
#define DLL_EXPORT
#endif

#include "XGetopt.h"
#include "nclist.h"
#include "ncbytes.h"

#ifdef _MSC_VER
static void XCommandLineToArgvA(int* argcp, char*** argvp);
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  X G e t o p t . c p p
//
//
//  NAME
//       getopt -- parse command line options
//
//  SYNOPSIS
//       int getopt(int argc, TCHAR *argv[], TCHAR *optstring)
//
//       extern TCHAR *optarg;
//       extern int optind;
//
//  DESCRIPTION
//       The getopt() function parses the command line arguments. Its
//       arguments argc and argv are the argument count and array as
//       passed into the application on program invocation.  In the case
//       of Visual C++ programs, argc and argv are available via the
//       variables __argc and __argv (double underscores), respectively.
//       getopt returns the next option letter in argv that matches a
//       letter in optstring.  (Note:  Unicode programs should use
//       __targv instead of __argv.  Also, all character and string
//       literals should be enclosed in _T( ) ).
//
//       optstring is a string of recognized option letters;  if a letter
//       is followed by a colon, the option is expected to have an argument
//       that may or may not be separated from it by white space.  optarg
//       is set to point to the start of the option argument on return from
//       getopt.
//
//       Option letters may be combined, e.g., "-ab" is equivalent to
//       "-a -b".  Option letters are case sensitive.
//
//       getopt places in the external variable optind the argv index
//       of the next argument to be processed.  optind is initialized
//       to 0 before the first call to getopt.
//
//       When all options have been processed (i.e., up to the first
//       non-option argument), getopt returns EOF, optarg will point
//       to the argument, and optind will be set to the argv index of
//       the argument.  If there are no non-option arguments, optarg
//       will be set to NULL.
//
//       The special option "--" may be used to delimit the end of the
//       options;  EOF will be returned, and "--" (and everything after it)
//       will be skipped.
//
//  RETURN VALUE
//       For option letters contained in the string optstring, getopt
//       will return the option letter.  getopt returns a question mark (?)
//       when it encounters an option letter not included in optstring.
//       EOF is returned when processing is finished.
//
//  BUGS
//       1)  Long options are not supported.
//       2)  The GNU double-colon extension is not supported.
//       3)  The environment variable POSIXLY_CORRECT is not supported.
//       4)  The + syntax is not supported.
//       5)  The automatic permutation of arguments is not supported.
//       6)  This implementation of getopt() returns EOF if an error is
//           encountered, instead of -1 as the latest standard requires.
//
//  EXAMPLE
//       BOOL CMyApp::ProcessCommandLine(int argc, TCHAR *argv[])
//       {
//           int c;
//
//           while ((c = getopt(argc, argv, _T("aBn:"))) != EOF)
//           {
//               switch (c)
//               {
//                   case _T('a'):
//                       TRACE(_T("option a\n"));
//                       //
//                       // set some flag here
//                       //
//                       break;
//
//                   case _T('B'):
//                       TRACE( _T("option B\n"));
//                       //
//                       // set some other flag here
//                       //
//                       break;
//
//                   case _T('n'):
//                       TRACE(_T("option n: value=%d\n"), atoi(optarg));
//                       //
//                       // do something with value here
//                       //
//                       break;
//
//                   case _T('?'):
//                       TRACE(_T("ERROR: illegal option %s\n"), argv[optind-1]);
//                       return FALSE;
//                       break;
//
//                   default:
//                       TRACE(_T("WARNING: no handler for option %c\n"), c);
//                       return FALSE;
//                       break;
//               }
//           }
//           //
//           // check for non-option args here
//           //
//           return TRUE;
//       }
//
///////////////////////////////////////////////////////////////////////////////

GTOPT_EXTRA TCHAR	*optarg;		// global argument pointer
GTOPT_EXTRA int		optind = 0; 	// global argv index
GTOPT_EXTRA int		opterr;		// print error

GTOPT_EXTRA
int getopt(int argc, TCHAR *argv[], TCHAR *optstring)
{
	static TCHAR *next = NULL;
	TCHAR c;
	TCHAR *cp = malloc(sizeof(TCHAR)*1024);

#ifdef _MSC_VER
	{
	    int xargc = -1;
	    char** xargv = NULL;
	    XCommandLineToArgvA(&xargc, &xargv);
	}
#endif

	if (optind == 0)
		next = NULL;

	optarg = NULL;

	if (next == NULL || *next == _T('\0'))
	{
		if (optind == 0)
			optind++;

		if (optind >= argc || argv[optind][0] != _T('-') || argv[optind][1] == _T('\0'))
		{
			optarg = NULL;
			if (optind < argc)
				optarg = argv[optind];
			return EOF;
		}

		if (_tcscmp(argv[optind], _T("--")) == 0)
		{
			optind++;
			optarg = NULL;
			if (optind < argc)
				optarg = argv[optind];
			return EOF;
		}

		next = argv[optind];
		next++;		// skip past -
		optind++;
	}

	c = *next++;
	cp = strchr(optstring, c);

	if (cp == NULL || c == _T(':'))
		return _T('?');

	cp++;
	if (*cp == _T(':'))
	{
		if (*next != _T('\0'))
		{
			optarg = next;
			next = NULL;
		}
		else if (optind < argc)
		{
			optarg = argv[optind];
			optind++;
		}
		else
		{
			return _T('?');
		}
	}

	return c;
}

/**************************************************/
#define ESCAPE '\\'
#define SQUOTE '\''
#define DQUOTE '"'

/* Convert a UTF8 command line into a series of words for use by XGetOpt */
/* Note only checks for ASCII '\' '\'' '"' */
static void
XCommandLineToArgvA(int* argcp, char*** argvp)
{
    const char* line = NULL;
    size_t len;
    int whitespace;
    int quote = 0;
    enum State state;
    NClist* argv = NULL;
    NCbytes* word = NULL;
    char** p;

    line = GetCommandLineA();
    len = strlen(line);
    argv = nclistnew();
    word = ncbytesnew();
    whitespace = 1; /* start in whitespace mode */
    for(p=line;*p;p++) {
	int c = *p;
        if(whitespace && c <= ' ' || c == 127) continue; /* more whitespace */
        if(!whitespace && c <= ' ' || c == 127) {
	    whitespace = 1; /* end of word */
	    ncbytesnull(word);
	    nclistpush(argv,ncbytesextract(word)); /* capture the word */
	    continue;
	}
	whitespace = 0; /* end whitespace */
	if(c == ESCAPE) {
	    c = *(++p); /* move to next char */
	} else if(c == SQUOTE || c == DQUOTE) {
	    if(!quote) {quote = c; continue;} /* Start quoted text */
	    if(quote == c) {quote = 0; continue;} /* end quoted text */
	}
	/* Just collect the character as part of the current word */
        ncbytesappend(word,c);
    }	
    /* Return parsed words */
    if(argcp) *argcp = nclistlength(argv);
    nclistpush(argv,NULL); /* Just to be sure */
    if(argvp) *argvp = (char**)nclistextract(argv);
    nclistfree(argv);
    ncbytesfree(word);
}
