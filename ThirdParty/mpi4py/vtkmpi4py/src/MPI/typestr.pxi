# -----------------------------------------------------------------------------

cdef inline char* Datatype2Str(MPI_Datatype datatype) nogil:
    if datatype == MPI_DATATYPE_NULL: return NULL
    # MPI
    elif datatype == MPI_LB     : return ""
    elif datatype == MPI_UB     : return ""
    elif datatype == MPI_PACKED : return "B"
    elif datatype == MPI_BYTE   : return "B"
    elif datatype == MPI_AINT   : return "p"# XXX NumPy-specific
    elif datatype == MPI_OFFSET :
        if   sizeof(MPI_Offset) == sizeof(MPI_Aint)  : return "p"
        elif sizeof(MPI_Offset) == sizeof(long long) : return "q"
        elif sizeof(MPI_Offset) == sizeof(long)      : return "l"
        elif sizeof(MPI_Offset) == sizeof(int)       : return "i"
        else                                         : return ""
    # C - character
    elif datatype == MPI_CHAR  : return "c"
    elif datatype == MPI_WCHAR : return ""#"U"#XXX
    # C - (signed) integral
    elif datatype == MPI_SIGNED_CHAR : return "b"
    elif datatype == MPI_SHORT       : return "h"
    elif datatype == MPI_INT         : return "i"
    elif datatype == MPI_LONG        : return "l"
    elif datatype == MPI_LONG_LONG   : return "q"
    # C - unsigned integral
    elif datatype == MPI_UNSIGNED_CHAR      : return "B"
    elif datatype == MPI_UNSIGNED_SHORT     : return "H"
    elif datatype == MPI_UNSIGNED           : return "I"
    elif datatype == MPI_UNSIGNED_LONG      : return "L"
    elif datatype == MPI_UNSIGNED_LONG_LONG : return "Q"
    # C - (real) floating
    elif datatype == MPI_FLOAT       : return "f"
    elif datatype == MPI_DOUBLE      : return "d"
    elif datatype == MPI_LONG_DOUBLE : return "g"
    # C99 - boolean
    elif datatype == MPI_C_BOOL : return "?"
    # C99 - integral
    elif datatype == MPI_INT8_T   : return "i1"
    elif datatype == MPI_INT16_T  : return "i2"
    elif datatype == MPI_INT32_T  : return "i4"
    elif datatype == MPI_INT64_T  : return "i8"
    elif datatype == MPI_UINT8_T  : return "u1"
    elif datatype == MPI_UINT16_T : return "u2"
    elif datatype == MPI_UINT32_T : return "u4"
    elif datatype == MPI_UINT64_T : return "u8"
    # C99 - complex floating
    elif datatype == MPI_C_COMPLEX             : return "F"
    elif datatype == MPI_C_FLOAT_COMPLEX       : return "F"
    elif datatype == MPI_C_DOUBLE_COMPLEX      : return "D"
    elif datatype == MPI_C_LONG_DOUBLE_COMPLEX : return "G"
    # Fortran
    elif datatype == MPI_CHARACTER        : return "c"
    elif datatype == MPI_LOGICAL          : return ""#"?"# XXX
    elif datatype == MPI_INTEGER          : return "i"
    elif datatype == MPI_REAL             : return "f"
    elif datatype == MPI_DOUBLE_PRECISION : return "d"
    elif datatype == MPI_COMPLEX          : return "F"
    elif datatype == MPI_DOUBLE_COMPLEX   : return "D"
    # Fortran 90
    elif datatype == MPI_LOGICAL1  : return ""#"?1"# XXX
    elif datatype == MPI_LOGICAL2  : return ""#"?2"# XXX
    elif datatype == MPI_LOGICAL4  : return ""#"?4"# XXX
    elif datatype == MPI_LOGICAL8  : return ""#"?8"# XXX
    elif datatype == MPI_INTEGER1  : return "i1"
    elif datatype == MPI_INTEGER2  : return "i2"
    elif datatype == MPI_INTEGER4  : return "i4"
    elif datatype == MPI_INTEGER8  : return "i8"
    elif datatype == MPI_INTEGER16 : return "i16"
    elif datatype == MPI_REAL2     : return "f2"
    elif datatype == MPI_REAL4     : return "f4"
    elif datatype == MPI_REAL8     : return "f8"
    elif datatype == MPI_REAL16    : return "f16"
    elif datatype == MPI_COMPLEX4  : return "c4"
    elif datatype == MPI_COMPLEX8  : return "c8"
    elif datatype == MPI_COMPLEX16 : return "c16"
    elif datatype == MPI_COMPLEX32 : return "c32"

    else : return ""

# -----------------------------------------------------------------------------
