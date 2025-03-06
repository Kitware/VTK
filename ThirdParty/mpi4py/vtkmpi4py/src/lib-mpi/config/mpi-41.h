#if defined(MPI_VERSION)
#if (MPI_VERSION > 4) || (MPI_VERSION == 4 && MPI_SUBVERSION >= 1)

#define PyMPI_HAVE_MPI_ERR_ERRHANDLER 1

#define PyMPI_HAVE_MPI_Remove_error_class 1
#define PyMPI_HAVE_MPI_Remove_error_code 1
#define PyMPI_HAVE_MPI_Remove_error_string 1

#define PyMPI_HAVE_MPI_Status_get_source 1
#define PyMPI_HAVE_MPI_Status_set_source 1
#define PyMPI_HAVE_MPI_Status_get_tag 1
#define PyMPI_HAVE_MPI_Status_set_tag 1
#define PyMPI_HAVE_MPI_Status_get_error 1
#define PyMPI_HAVE_MPI_Status_set_error 1

#define PyMPI_HAVE_MPI_COMBINER_VALUE_INDEX 1
#define PyMPI_HAVE_MPI_Type_get_value_index 1

#define PyMPI_HAVE_MPI_Request_get_status_any 1
#define PyMPI_HAVE_MPI_Request_get_status_all 1
#define PyMPI_HAVE_MPI_Request_get_status_some 1

#define PyMPI_HAVE_MPI_BUFFER_AUTOMATIC 1
#define PyMPI_HAVE_MPI_Buffer_flush 1
#define PyMPI_HAVE_MPI_Buffer_iflush 1
#define PyMPI_HAVE_MPI_Comm_attach_buffer 1
#define PyMPI_HAVE_MPI_Comm_detach_buffer 1
#define PyMPI_HAVE_MPI_Comm_flush_buffer 1
#define PyMPI_HAVE_MPI_Comm_iflush_buffer 1
#define PyMPI_HAVE_MPI_Session_attach_buffer 1
#define PyMPI_HAVE_MPI_Session_detach_buffer 1
#define PyMPI_HAVE_MPI_Session_flush_buffer  1
#define PyMPI_HAVE_MPI_Session_iflush_buffer 1
#define PyMPI_HAVE_MPI_Comm_attach_buffer_c 1
#define PyMPI_HAVE_MPI_Comm_detach_buffer_c 1
#define PyMPI_HAVE_MPI_Session_attach_buffer_c 1
#define PyMPI_HAVE_MPI_Session_detach_buffer_c 1

#define PyMPI_HAVE_MPI_COMM_TYPE_RESOURCE_GUIDED 1

#define PyMPI_HAVE_MPI_Get_hw_resource_info 1

#endif
#endif
