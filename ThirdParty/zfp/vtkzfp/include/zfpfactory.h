#ifndef ZFP_FACTORY_H
#define ZFP_FACTORY_H

// (assumes zfparray.h already included)

zfp::array* zfp::array::construct(const zfp::array::header& header, const uchar* buffer, size_t buffer_size_bytes)
{
  // gather array metadata via C API, then construct with metadata
  uint dims = 0;
  zfp_type type = zfp_type_none;
  double rate = 0;
  uint n[4] = {0};

  // read once (will throw if reads a noncompatible header)
  zfp::array::read_header_contents(header, buffer_size_bytes, dims, type, rate, n);

  // construct once (passing zfp::array::header will read it again)
  zfp::array* arr = 0;
  std::string err_msg = "";
  switch (dims) {
    case 3:
#ifdef ZFP_ARRAY3_H
      switch (type) {
        case zfp_type_double:
          arr = new zfp::array3d(n[0], n[1], n[2], rate);
          break;

        case zfp_type_float:
          arr = new zfp::array3f(n[0], n[1], n[2], rate);
          break;

        default:
          /* NOTREACHED */
          err_msg = "Unexpected ZFP type.";
          break;
      }
#else
      err_msg = "Header files for 3 dimensional ZFP compressed arrays were not included.";
#endif
      break;

    case 2:
#ifdef ZFP_ARRAY2_H
      switch (type) {
        case zfp_type_double:
          arr = new zfp::array2d(n[0], n[1], rate);
          break;

        case zfp_type_float:
          arr = new zfp::array2f(n[0], n[1], rate);
          break;

        default:
          /* NOTREACHED */
          err_msg = "Unexpected ZFP type.";
          break;
      }
#else
      err_msg = "Header files for 2 dimensional ZFP compressed arrays were not included.";
#endif
      break;

    case 1:
#ifdef ZFP_ARRAY1_H
      switch (type) {
        case zfp_type_double:
          arr = new zfp::array1d(n[0], rate);
          break;

        case zfp_type_float:
          arr = new zfp::array1f(n[0], rate);
          break;

        default:
          /* NOTREACHED */
          err_msg = "Unexpected ZFP type.";
          break;
      }
#else
      err_msg = "Header files for 1 dimensional ZFP compressed arrays were not included.";
#endif
      break;

    default:
      err_msg = "ZFP compressed arrays do not yet support dimensionalities beyond 1, 2, and 3.";
      break;
  }

  if (!err_msg.empty())
    throw zfp::array::header::exception(err_msg);

  if (buffer)
    memcpy(arr->compressed_data(), buffer, arr->compressed_size());

  return arr;
}

#endif
