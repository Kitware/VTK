# Opening modes
# -------------

MODE_RDONLY          = MPI_MODE_RDONLY           #: Read only
MODE_WRONLY          = MPI_MODE_WRONLY           #: Write only
MODE_RDWR            = MPI_MODE_RDWR             #: Reading and writing
MODE_CREATE          = MPI_MODE_CREATE           #: Create the file if it does not exist
MODE_EXCL            = MPI_MODE_EXCL             #: Error if creating file that already exists
MODE_DELETE_ON_CLOSE = MPI_MODE_DELETE_ON_CLOSE  #: Delete file on close
MODE_UNIQUE_OPEN     = MPI_MODE_UNIQUE_OPEN      #: File will not be concurrently opened elsewhere
MODE_SEQUENTIAL      = MPI_MODE_SEQUENTIAL       #: File will only be accessed sequentially
MODE_APPEND          = MPI_MODE_APPEND           #: Set initial position of all file pointers to end of file


# Positioning
# -----------

SEEK_SET = MPI_SEEK_SET  #: File pointer is set to offset
SEEK_CUR = MPI_SEEK_CUR  #: File pointer is set to the current position plus offset
SEEK_END = MPI_SEEK_END  #: File pointer is set to the end plus offset
DISPLACEMENT_CURRENT = MPI_DISPLACEMENT_CURRENT  #: Special displacement value for files opened in sequential mode
DISP_CUR             = MPI_DISPLACEMENT_CURRENT  #: Convenience alias for `DISPLACEMENT_CURRENT`


cdef class File:

    """
    File
    """

    def __cinit__(self, File file=None):
        self.ob_mpi = MPI_FILE_NULL
        if file is not None:
            self.ob_mpi = file.ob_mpi

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( del_File(&self.ob_mpi) )

    def __richcmp__(self, other, int op):
        if not isinstance(self,  File): return NotImplemented
        if not isinstance(other, File): return NotImplemented
        cdef File s = <File>self, o = <File>other
        if   op == Py_EQ: return (s.ob_mpi == o.ob_mpi)
        elif op == Py_NE: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError("only '==' and '!='")

    def __bool__(self):
        return self.ob_mpi != MPI_FILE_NULL

    # [9.2] File Manipulation
    # -----------------------

    # [9.2.1] Opening a File
    # ----------------------

    @classmethod
    def Open(cls, Intracomm comm not None, filename,
             int amode=MODE_RDONLY, Info info=INFO_NULL):
        """
        Open a file
        """
        cdef char *cfilename = NULL
        filename = asmpistr(filename, &cfilename, NULL)
        cdef MPI_Info cinfo = arg_Info(info)
        cdef File file = <File>cls()
        with nogil: CHKERR( MPI_File_open(
            comm.ob_mpi, cfilename, amode, cinfo, &file.ob_mpi) )
        return file

    # [9.2.2] Closing a File
    # ----------------------

    def Close(self):
        """
        Close a file
        """
        with nogil: CHKERR( MPI_File_close(&self.ob_mpi) )

    # [9.2.3] Deleting a File
    # -----------------------

    @classmethod
    def Delete(cls, filename, Info info=INFO_NULL):
        """
        Delete a file
        """
        cdef char *cfilename = NULL
        filename = asmpistr(filename, &cfilename, NULL)
        cdef MPI_Info cinfo = arg_Info(info)
        with nogil: CHKERR( MPI_File_delete(cfilename, cinfo) )

    # [9.2.4] Resizing a File
    # -----------------------

    def Set_size(self, Offset size):
        """
        Sets the file size
        """
        with nogil: CHKERR( MPI_File_set_size(self.ob_mpi, size) )

    # [9.2.5] Preallocating Space for a File
    # --------------------------------------

    def Preallocate(self, Offset size):
        """
        Preallocate storage space for a file
        """
        with nogil: CHKERR( MPI_File_preallocate(self.ob_mpi, size) )

    # [9.2.6] Querying the Size of a File
    # -----------------------------------
    def Get_size(self):
        """
        Return the file size
        """
        cdef MPI_Offset size = 0
        with nogil: CHKERR( MPI_File_get_size(self.ob_mpi, &size) )
        return size

    property size:
        """file size"""
        def __get__(self):
            return self.Get_size()

    # [9.2.7] Querying File Parameters
    # --------------------------------

    def Get_group(self):
        """
        Return the group of processes
        that opened the file
        """
        cdef Group group = <Group>Group.__new__(Group)
        with nogil: CHKERR( MPI_File_get_group(self.ob_mpi, &group.ob_mpi) )
        return group

    property group:
        """file group"""
        def __get__(self):
            return self.Get_group()

    def Get_amode(self):
        """
        Return the file access mode
        """
        cdef int amode = 0
        with nogil: CHKERR( MPI_File_get_amode(self.ob_mpi, &amode) )
        return amode

    property amode:
        """file access mode"""
        def __get__(self):
            return self.Get_amode()

    # [9.2.8] File Info
    # -----------------

    def Set_info(self, Info info not None):
        """
        Set new values for the hints
        associated with a file
        """
        with nogil: CHKERR( MPI_File_set_info(self.ob_mpi, info.ob_mpi) )

    def Get_info(self):
        """
        Return the hints for a file that
        are actually being used by MPI
        """
        cdef Info info = <Info>Info.__new__(Info)
        with nogil: CHKERR( MPI_File_get_info(self.ob_mpi, &info.ob_mpi) )
        return info

    property info:
        """file info"""
        def __get__(self):
            return self.Get_info()
        def __set__(self, info):
            self.Set_info(info)

    # [9.3] File Views
    # ----------------

    def Set_view(self, Offset disp=0,
                 Datatype etype=None, Datatype filetype=None,
                 object datarep=None, Info info=INFO_NULL):
        """
        Set the file view
        """
        cdef char *cdatarep = b"native"
        if datarep is not None: datarep = asmpistr(datarep, &cdatarep, NULL)
        cdef MPI_Datatype cetype = MPI_BYTE
        if etype is not None: cetype = etype.ob_mpi
        cdef MPI_Datatype cftype = cetype
        if filetype is not None: cftype = filetype.ob_mpi
        cdef MPI_Info cinfo = arg_Info(info)
        with nogil: CHKERR( MPI_File_set_view(
            self.ob_mpi, disp, cetype, cftype, cdatarep, cinfo) )

    def Get_view(self):
        """
        Return the file view
        """
        cdef MPI_Offset disp = 0
        cdef Datatype etype = <Datatype>Datatype.__new__(Datatype)
        cdef Datatype ftype = <Datatype>Datatype.__new__(Datatype)
        cdef char cdatarep[MPI_MAX_DATAREP_STRING+1]
        with nogil: CHKERR( MPI_File_get_view(
            self.ob_mpi, &disp, &etype.ob_mpi, &ftype.ob_mpi, cdatarep) )
        fix_fileview_Datatype(etype); fix_fileview_Datatype(ftype)
        cdatarep[MPI_MAX_DATAREP_STRING] = 0 # just in case
        cdef object datarep = mpistr(cdatarep)
        return (disp, etype, ftype, datarep)

    # [9.4] Data Access
    # -----------------

    # [9.4.2] Data Access with Explicit Offsets
    # -----------------------------------------

    def Read_at(self, Offset offset, buf, Status status=None):
        """
        Read using explicit offset
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_at(
            self.ob_mpi, offset, m.buf, m.count, m.dtype, statusp) )

    def Read_at_all(self, Offset offset, buf, Status status=None):
        """
        Collective read using explicit offset
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_at_all(
            self.ob_mpi, offset, m.buf, m.count, m.dtype, statusp) )

    def Write_at(self, Offset offset, buf, Status status=None):
        """
        Write using explicit offset
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_at(
            self.ob_mpi, offset, m.buf, m.count, m.dtype, statusp) )

    def Write_at_all(self, Offset offset, buf, Status status=None):
        """
        Collective write using explicit offset
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_at_all(
            self.ob_mpi, offset, m.buf, m.count, m.dtype, statusp) )

    def Iread_at(self, Offset offset, buf):
        """
        Nonblocking read using explicit offset
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef Request request = <Request>Request.__new__(Request)
        with nogil: CHKERR( MPI_File_iread_at(
            self.ob_mpi, offset, m.buf, m.count, m.dtype, &request.ob_mpi) )
        request.ob_buf = m
        return request

    def Iwrite_at(self, Offset offset, buf):
        """
        Nonblocking write using explicit offset
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef Request request = <Request>Request.__new__(Request)
        with nogil: CHKERR( MPI_File_iwrite_at(
            self.ob_mpi, offset, m.buf, m.count, m.dtype, &request.ob_mpi) )
        request.ob_buf = m
        return request

    # [9.4.3] Data Access with Individual File Pointers
    # -------------------------------------------------

    def Read(self, buf, Status status=None):
        """
        Read using individual file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Read_all(self, buf, Status status=None):
        """
        Collective read using individual file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_all(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Write(self, buf, Status status=None):
        """
        Write using individual file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Write_all(self, buf, Status status=None):
        """
        Collective write using individual file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_all(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Iread(self, buf):
        """
        Nonblocking read using individual file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef Request request = <Request>Request.__new__(Request)
        with nogil: CHKERR( MPI_File_iread(
            self.ob_mpi, m.buf, m.count, m.dtype, &request.ob_mpi) )
        request.ob_buf = m
        return request

    def Iwrite(self, buf):
        """
        Nonblocking write using individual file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef Request request = <Request>Request.__new__(Request)
        with nogil: CHKERR( MPI_File_iwrite(
            self.ob_mpi, m.buf, m.count, m.dtype, &request.ob_mpi) )
        request.ob_buf = m
        return request

    def Seek(self, Offset offset, int whence=SEEK_SET):
        """
        Update the individual file pointer
        """
        with nogil: CHKERR( MPI_File_seek(self.ob_mpi, offset, whence) )

    def Get_position(self):
        """
        Return the current position of the individual file pointer
        in etype units relative to the current view
        """
        cdef MPI_Offset offset = 0
        with nogil: CHKERR( MPI_File_get_position(self.ob_mpi, &offset) )
        return offset

    def Get_byte_offset(self, Offset offset):
        """
        Returns the absolute byte position in the file corresponding
        to 'offset' etypes relative to the current view
        """
        cdef MPI_Offset disp = 0
        with nogil: CHKERR( MPI_File_get_byte_offset(
            self.ob_mpi, offset, &disp) )
        return disp

    # [9.4.4] Data Access with Shared File Pointers
    # ---------------------------------------------

    def Read_shared(self, buf, Status status=None):
        """
        Read using shared file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_shared(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Write_shared(self, buf, Status status=None):
        """
        Write using shared file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_shared(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Iread_shared(self, buf):
        """
        Nonblocking read using shared file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef Request request = <Request>Request.__new__(Request)
        with nogil: CHKERR( MPI_File_iread_shared(
            self.ob_mpi, m.buf, m.count, m.dtype, &request.ob_mpi) )
        request.ob_buf = m
        return request

    def Iwrite_shared(self, buf):
        """
        Nonblocking write using shared file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef Request request = <Request>Request.__new__(Request)
        with nogil: CHKERR( MPI_File_iwrite_shared(
            self.ob_mpi, m.buf, m.count, m.dtype, &request.ob_mpi) )
        request.ob_buf = m
        return request

    def Read_ordered(self, buf, Status status=None):
        """
        Collective read using shared file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_ordered(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Write_ordered(self, buf, Status status=None):
        """
        Collective write using shared file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_ordered(
            self.ob_mpi, m.buf, m.count, m.dtype, statusp) )

    def Seek_shared(self, Offset offset, int whence=SEEK_SET):
        """
        Update the shared file pointer
        """
        with nogil: CHKERR( MPI_File_seek_shared(self.ob_mpi, offset, whence) )

    def Get_position_shared(self):
        """
        Return the current position of the shared file pointer
        in etype units relative to the current view
        """
        cdef MPI_Offset offset = 0
        with nogil: CHKERR( MPI_File_get_position_shared(self.ob_mpi, &offset) )
        return offset

    # [9.4.5] Split Collective Data Access Routines
    # ---------------------------------------------

    # explicit offset

    def Read_at_all_begin(self, Offset offset, buf):
        """
        Start a split collective read using explict offset
        """
        cdef _p_msg_io m = message_io_read(buf)
        with nogil: CHKERR( MPI_File_read_at_all_begin(
            self.ob_mpi, offset, m.buf, m.count, m.dtype) )

    def Read_at_all_end(self, buf, Status status=None):
        """
        Complete a split collective read using explict offset
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_at_all_end(
            self.ob_mpi, m.buf, statusp) )

    def Write_at_all_begin(self, Offset offset, buf):
        """
        Start a split collective write using explict offset
        """
        cdef _p_msg_io m = message_io_write(buf)
        with nogil: CHKERR( MPI_File_write_at_all_begin(
            self.ob_mpi, offset, m.buf, m.count, m.dtype) )

    def Write_at_all_end(self, buf, Status status=None):
        """
        Complete a split collective write using explict offset
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_at_all_end(
            self.ob_mpi, m.buf, statusp) )

    # individual file pointer

    def Read_all_begin(self, buf):
        """
        Start a split collective read
        using individual file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        with nogil: CHKERR( MPI_File_read_all_begin(
            self.ob_mpi, m.buf, m.count, m.dtype) )

    def Read_all_end(self, buf, Status status=None):
        """
        Complete a split collective read
        using individual file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_all_end(
            self.ob_mpi, m.buf, statusp) )

    def Write_all_begin(self, buf):
        """
        Start a split collective write
        using individual file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        with nogil: CHKERR( MPI_File_write_all_begin(
            self.ob_mpi, m.buf, m.count, m.dtype) )

    def Write_all_end(self, buf, Status status=None):
        """
        Complete a split collective write
        using individual file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_all_end(
            self.ob_mpi, m.buf, statusp) )

    # shared file pointer

    def Read_ordered_begin(self, buf):
        """
        Start a split collective read
        using shared file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        with nogil: CHKERR( MPI_File_read_ordered_begin(
            self.ob_mpi, m.buf, m.count, m.dtype) )

    def Read_ordered_end(self, buf, Status status=None):
        """
        Complete a split collective read
        using shared file pointer
        """
        cdef _p_msg_io m = message_io_read(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_read_ordered_end(
            self.ob_mpi, m.buf, statusp) )

    def Write_ordered_begin(self, buf):
        """
        Start a split collective write using
        shared file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        with nogil: CHKERR( MPI_File_write_ordered_begin(
            self.ob_mpi, m.buf, m.count, m.dtype) )

    def Write_ordered_end(self, buf, Status status=None):
        """
        Complete a split collective write
        using shared file pointer
        """
        cdef _p_msg_io m = message_io_write(buf)
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_File_write_ordered_end(
            self.ob_mpi, m.buf, statusp) )

    # [9.5] File Interoperability
    # ---------------------------

    # [9.5.1] Datatypes for File Interoperability
    # -------------------------------------------

    def Get_type_extent(self, Datatype datatype not None):
        """
        Return the extent of datatype in the file
        """
        cdef MPI_Aint extent = 0
        with nogil: CHKERR( MPI_File_get_type_extent(
            self.ob_mpi, datatype.ob_mpi, &extent) )
        return extent

    # [9.6] Consistency and Semantics
    # -------------------------------

    # [9.6.1] File Consistency
    # ------------------------

    def Set_atomicity(self, bint flag):
        """
        Set the atomicity mode
        """
        with nogil: CHKERR( MPI_File_set_atomicity(self.ob_mpi, flag) )

    def Get_atomicity(self):
        """
        Return the atomicity mode
        """
        cdef int flag = 0
        with nogil: CHKERR( MPI_File_get_atomicity(self.ob_mpi, &flag) )
        return <bint>flag

    property atomicity:
        """atomicity"""
        def __get__(self):
            return self.Get_atomicity()
        def __set__(self, value):
            self.Set_atomicity(value)

    def Sync(self):
        """
        Causes all previous writes to be
        transferred to the storage device
        """
        with nogil: CHKERR( MPI_File_sync(self.ob_mpi) )

    # [9.7] I/O Error Handling
    # ------------------------

    def Get_errhandler(self):
        """
        Get the error handler for a file
        """
        cdef Errhandler errhandler = <Errhandler>Errhandler.__new__(Errhandler)
        CHKERR( MPI_File_get_errhandler(self.ob_mpi, &errhandler.ob_mpi) )
        return errhandler

    def Set_errhandler(self, Errhandler errhandler not None):
        """
        Set the error handler for a file
        """
        CHKERR( MPI_File_set_errhandler(self.ob_mpi, errhandler.ob_mpi) )

    def Call_errhandler(self, int errorcode):
        """
        Call the error handler installed on a file
        """
        CHKERR( MPI_File_call_errhandler(self.ob_mpi, errorcode) )

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_File_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef File file = <File>cls()
        file.ob_mpi = MPI_File_f2c(arg)
        return file



cdef File __FILE_NULL__ = new_File(MPI_FILE_NULL)


# Predefined file handles
# -----------------------

FILE_NULL = __FILE_NULL__  #: Null file handle
