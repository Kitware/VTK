/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

/* -------------------------------------------------------------------------- */

#include <Python.h>

#define MPICH_IGNORE_CXX_SEEK 1
#define OMPI_IGNORE_CXX_SEEK 1
#include <mpi.h>

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

static int PyMPI_Main(int, char **);
#if PY_MAJOR_VERSION >= 3
static int Py3_Main(int, char **);
#else
static int Py2_Main(int, char **);
#endif

/* -------------------------------------------------------------------------- */

int
main(int argc, char **argv)
{
#ifdef __FreeBSD__
  fp_except_t m;
  m = fpgetmask();
  fpsetmask(m & ~FP_X_OFL);
#endif
  return PyMPI_Main(argc, argv);
}

static int
PyMPI_Main(int argc, char **argv)
{
  int sts=0, flag=1, finalize=0;

  /* MPI initalization */
  (void)MPI_Initialized(&flag);
  if (!flag) {
#if (defined(MPI_VERSION) && (MPI_VERSION > 1))
    int required = MPI_THREAD_MULTIPLE;
    int provided = MPI_THREAD_SINGLE;
    (void)MPI_Init_thread(&argc, &argv, required, &provided);
#else
    (void)MPI_Init(&argc, &argv);
#endif
    finalize = 1;
  }

  /* Python main */
#if PY_MAJOR_VERSION >= 3
  sts = Py3_Main(argc, argv);
#else
  sts = Py2_Main(argc, argv);
#endif

  /* MPI finalization */
  (void)MPI_Finalized(&flag);
  if (!flag) {
    if (sts != 0) (void)MPI_Abort(MPI_COMM_WORLD, sts);
    if (finalize) (void)MPI_Finalize();
  }

  return sts;
}

/* -------------------------------------------------------------------------- */

#if PY_MAJOR_VERSION <= 2
static int
Py2_Main(int argc, char **argv)
{
  return Py_Main(argc,argv);
}
#endif

#if PY_MAJOR_VERSION >= 3
#include <locale.h>
static wchar_t **mk_wargs(int, char **);
static wchar_t **cp_wargs(int, wchar_t **);
static void rm_wargs(wchar_t **, int);

static int
Py3_Main(int argc, char **argv)
{
  int sts = 0;
  wchar_t **wargv  = mk_wargs(argc, argv);
  wchar_t **wargv2 = cp_wargs(argc, wargv);
  if (wargv && wargv2)
    sts = Py_Main(argc, wargv);
  else
    sts = 1;
  rm_wargs(wargv2, 1);
  rm_wargs(wargv,  0);
  return sts;
}

#if PY_VERSION_HEX < 0x03020000
static wchar_t *_Py_char2wchar(const char *, size_t *);
#elif defined(__APPLE__)
#ifdef __cplusplus
extern "C" {
#endif
extern wchar_t* _Py_DecodeUTF8_surrogateescape(const char *, Py_ssize_t);
#ifdef __cplusplus
}
#endif
#endif

static wchar_t **
mk_wargs(int argc, char **argv)
{
  int i; char *saved_locale = NULL;
  wchar_t **args = NULL;

  args = (wchar_t **)PyMem_Malloc((size_t)(argc+1)*sizeof(wchar_t *));
  if (!args) goto oom;

  saved_locale = strdup(setlocale(LC_ALL, NULL));
  if (!saved_locale) goto oom;
  setlocale(LC_ALL, "");

  for (i=0; i<argc; i++) {
#if defined(__APPLE__) && PY_VERSION_HEX >= 0x03020000
    args[i] = _Py_DecodeUTF8_surrogateescape(argv[i], strlen(argv[i]));
#else
    args[i] = _Py_char2wchar(argv[i], NULL);
#endif
    if (!args[i]) goto oom;
  }
  args[argc] = NULL;

  setlocale(LC_ALL, saved_locale);
  free(saved_locale);

  return args;

 oom:
  fprintf(stderr, "out of memory\n");
  if (saved_locale) {
    setlocale(LC_ALL, saved_locale);
    free(saved_locale);
  }
  if (args)
    rm_wargs(args, 1);
  return NULL;
}

static wchar_t **
cp_wargs(int argc, wchar_t **args)
{
  int i; wchar_t **args_copy = NULL;
  if (!args) return NULL;
  args_copy = (wchar_t **)PyMem_Malloc((size_t)(argc+1)*sizeof(wchar_t *));
  if (!args_copy) goto oom;
  for (i=0; i<(argc+1); i++) { args_copy[i] = args[i]; }
  return args_copy;
 oom:
  fprintf(stderr, "out of memory\n");
  return NULL;
}

static void
rm_wargs(wchar_t **args, int deep)
{
  int i = 0;
  if (args && deep)
    while (args[i])
      PyMem_Free(args[i++]);
  if (args)
    PyMem_Free(args);
}

#if PY_VERSION_HEX < 0x03020000
static wchar_t *
_Py_char2wchar(const char* arg, size_t *size)
{
  wchar_t *res;
#ifdef HAVE_BROKEN_MBSTOWCS
  /* Some platforms have a broken implementation of
   * mbstowcs which does not count the characters that
   * would result from conversion.  Use an upper bound.
   */
  size_t argsize = strlen(arg);
#else
  size_t argsize = mbstowcs(NULL, arg, 0);
#endif
  size_t count;
  unsigned char *in;
  wchar_t *out;
#ifdef HAVE_MBRTOWC
  mbstate_t mbs;
#endif
  if (argsize != (size_t)-1) {
    res = (wchar_t *)PyMem_Malloc((argsize+1)*sizeof(wchar_t));
    if (!res)
      goto oom;
    count = mbstowcs(res, arg, argsize+1);
    if (count != (size_t)-1) {
      wchar_t *tmp;
      /* Only use the result if it contains no
         surrogate characters. */
      for (tmp = res; *tmp != 0 &&
             (*tmp < 0xd800 || *tmp > 0xdfff); tmp++)
        ;
      if (*tmp == 0) {
        if (size != NULL)
          *size = count;
        return res;
      }
    }
    PyMem_Free(res);
  }
  /* Conversion failed. Fall back to escaping with surrogateescape. */
#ifdef HAVE_MBRTOWC
  /* Try conversion with mbrtwoc (C99), and escape non-decodable bytes. */

  /* Overallocate; as multi-byte characters are in the argument, the
     actual output could use less memory. */
  argsize = strlen(arg) + 1;
  res = (wchar_t *)PyMem_Malloc(argsize*sizeof(wchar_t));
  if (!res) goto oom;
  in = (unsigned char*)arg;
  out = res;
  memset(&mbs, 0, sizeof mbs);
  while (argsize) {
    size_t converted = mbrtowc(out, (char*)in, argsize, &mbs);
    if (converted == 0)
      /* Reached end of string; null char stored. */
      break;
    if (converted == (size_t)-2) {
      /* Incomplete character. This should never happen,
         since we provide everything that we have -
         unless there is a bug in the C library, or I
         misunderstood how mbrtowc works. */
      fprintf(stderr, "unexpected mbrtowc result -2\n");
      return NULL;
    }
    if (converted == (size_t)-1) {
      /* Conversion error. Escape as UTF-8b, and start over
         in the initial shift state. */
      *out++ = 0xdc00 + *in++;
      argsize--;
      memset(&mbs, 0, sizeof mbs);
      continue;
    }
    if (*out >= 0xd800 && *out <= 0xdfff) {
      /* Surrogate character.  Escape the original
         byte sequence with surrogateescape. */
      argsize -= converted;
      while (converted--)
        *out++ = 0xdc00 + *in++;
      continue;
    }
    /* successfully converted some bytes */
    in += converted;
    argsize -= converted;
    out++;
  }
#else
  /* Cannot use C locale for escaping; manually escape as if charset
     is ASCII (i.e. escape all bytes > 128. This will still roundtrip
     correctly in the locale's charset, which must be an ASCII superset. */
  res = (wchar_t *)PyMem_Malloc((strlen(arg)+1)*sizeof(wchar_t));
  if (!res) goto oom;
  in = (unsigned char*)arg;
  out = res;
  while(*in)
    if(*in < 128)
      *out++ = *in++;
    else
      *out++ = 0xdc00 + *in++;
  *out = 0;
#endif
  if (size != NULL)
    *size = (size_t)(out - res);
  return res;
 oom:
  fprintf(stderr, "out of memory\n");
  return NULL;
}
#endif

#endif /* !(PY_MAJOR_VERSION >= 3) */

/* -------------------------------------------------------------------------- */

/*
   Local variables:
   c-basic-offset: 2
   indent-tabs-mode: nil
   End:
*/
