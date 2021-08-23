/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: Public, shared definitions for Mirror VFD & remote Writer.
 */

#ifndef H5FDmirror_priv_H
#define H5FDmirror_priv_H

#ifdef H5_HAVE_MIRROR_VFD

#ifdef __cplusplus
extern "C" {
#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
 * IPC - Mirror VFD and Remote Worker application.
 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
 */

/* The maximum allowed size for a receiving buffer when accepting bytes to
 * write. Writes larger than this size are performed by multiple accept-write
 * steps by the Writer. */
#define H5FD_MIRROR_DATA_BUFFER_MAX H5_GB /* 1 Gigabyte */

#define H5FD_MIRROR_XMIT_CURR_VERSION 1
#define H5FD_MIRROR_XMIT_MAGIC        0x87F8005B

#define H5FD_MIRROR_OP_OPEN     1
#define H5FD_MIRROR_OP_CLOSE    2
#define H5FD_MIRROR_OP_WRITE    3
#define H5FD_MIRROR_OP_TRUNCATE 4
#define H5FD_MIRROR_OP_REPLY    5
#define H5FD_MIRROR_OP_SET_EOA  6
#define H5FD_MIRROR_OP_LOCK     7
#define H5FD_MIRROR_OP_UNLOCK   8

#define H5FD_MIRROR_STATUS_OK          0
#define H5FD_MIRROR_STATUS_ERROR       1
#define H5FD_MIRROR_STATUS_MESSAGE_MAX 256 /* Dedicated error message size */

/* Maximum length of a path/filename string, including the NULL-terminator.
 * Must not be smaller than H5FD_SPLITTER_PATH_MAX. */
#define H5FD_MIRROR_XMIT_FILEPATH_MAX 4097

/* Define the exact sizes of the various xmit blobs as sent over the wire.
 * This is used to minimize the number of bytes transmitted as well as to
 * sanity-check received bytes.
 * Any modifications to the xmit structures and/or the encode/decode functions
 * must be reflected here.
 * */
#define H5FD_MIRROR_XMIT_HEADER_SIZE 14
#define H5FD_MIRROR_XMIT_EOA_SIZE    (H5FD_MIRROR_XMIT_HEADER_SIZE + 9)
#define H5FD_MIRROR_XMIT_LOCK_SIZE   (H5FD_MIRROR_XMIT_HEADER_SIZE + 8)
#define H5FD_MIRROR_XMIT_OPEN_SIZE   (H5FD_MIRROR_XMIT_HEADER_SIZE + 20 + H5FD_MIRROR_XMIT_FILEPATH_MAX)
#define H5FD_MIRROR_XMIT_REPLY_SIZE  (H5FD_MIRROR_XMIT_HEADER_SIZE + 4 + H5FD_MIRROR_STATUS_MESSAGE_MAX)
#define H5FD_MIRROR_XMIT_WRITE_SIZE  (H5FD_MIRROR_XMIT_HEADER_SIZE + 17)

/* Maximum length of any xmit. */
#define H5FD_MIRROR_XMIT_BUFFER_MAX                                                                          \
    MAX2(MAX3(H5FD_MIRROR_XMIT_HEADER_SIZE, H5FD_MIRROR_XMIT_EOA_SIZE, H5FD_MIRROR_XMIT_LOCK_SIZE),          \
         MAX3(H5FD_MIRROR_XMIT_OPEN_SIZE, H5FD_MIRROR_XMIT_REPLY_SIZE, H5FD_MIRROR_XMIT_WRITE_SIZE))

/* ---------------------------------------------------------------------------
 * Structure:   H5FD_mirror_xmit_t
 *
 * Common structure 'header' for all mirror VFD/worker IPC.
 * Must be the first component of a derived operation xmit structure,
 * such as file-open or write command.
 *
 * `magic` (uint32_t)
 *      A "unique" number identifying the structure and endianness of
 *      transmitting maching.
 *      Must be set to H5FD_MIRROR_XMIT_MAGIC native to the VFD "sender".
 *
 * `version` (uint8_t)
 *      Number used to identify the structure membership.
 *      Allows sane modifications to this structure in the future.
 *      Must be set to H5FD_MIRROR_XMIT_CURR_VERSION.
 *
 * `session_token` (uint32_t)
 *      A "unique" number identifying the session between VFD sender and
 *      remote receiver/worker/writer. Exists to help sanity-check.
 *
 * `xmit_count` (uint32_t)
 *      Which transmission this is since the session began.
 *      Used to sanity-check transmission errors.
 *      First xmit (file-open) must be 0.
 *
 * `op` (uint8_t)
 *      Number identifying which operation to perform.
 *      Corresponds with the extended structure outside of this xmit header.
 *      Possible values are all defined H5FD_MIRROR_OP_* constants.
 *
 * ---------------------------------------------------------------------------
 */
typedef struct H5FD_mirror_xmit_t {
    uint32_t magic;
    uint8_t  version;
    uint32_t session_token;
    uint32_t xmit_count;
    uint8_t  op;
} H5FD_mirror_xmit_t;

/* ---------------------------------------------------------------------------
 * Structure:   H5FD_mirror_xmit_eoa_t
 *
 * Structure containing eoa-set information from VFD sender.
 *
 * `pub` (H5FD_mirror_xmit_t)
 *      Common transmission header, containing session information.
 *      Must be first.
 *
 * `type` (uint8_t)
 *      System-independent alias for H5F[D]_mem_t.
 *      Specifies datatype to be written.
 *
 * `eoa_addr` (uint64_t)
 *      New address for eoa.
 *      (Natively 'haddr_t', always a 64-bit field)
 *
 * ---------------------------------------------------------------------------
 */
typedef struct H5FD_mirror_xmit_eoa_t {
    H5FD_mirror_xmit_t pub;
    uint8_t            type;
    uint64_t           eoa_addr;
} H5FD_mirror_xmit_eoa_t;

/* ---------------------------------------------------------------------------
 * Structure:   H5FD_mirror_xmit_lock_t
 *
 * Structure containing eoa-set information from VFD sender.
 *
 * `pub` (H5FD_mirror_xmit_t)
 *      Common transmission header, containing session information.
 *      Must be first.
 *
 * `rw` (uint64_t)
 *      The Read/Write mode flag passed into H5FDlock().
 *      (Natively `hbool_t`, an 'int') TODO: native int may be 64-bit?
 *
 * ---------------------------------------------------------------------------
 */
typedef struct H5FD_mirror_xmit_lock_t {
    H5FD_mirror_xmit_t pub;
    uint64_t           rw;
} H5FD_mirror_xmit_lock_t;

/* ---------------------------------------------------------------------------
 * Structure:   H5FD_mirror_xmit_open_t
 *
 * Structure containing file-open information from the VFD sender.
 *
 * `pub` (H5FD_mirror_xmit_t)
 *      Common transmission header, containing session information.
 *      Must be first.
 *
 * `flags` (uint32_t)
 *      VFL-layer file-open flags passed directly to H5FDopen().
 *      (Natively 'unsigned [int]') TODO: native int may be 64-bit?
 *
 * `maxaddr` (uint64_t)
 *      VFL-layer maximum allowed address space for the file to open passed
 *      directly to H5FDopen().
 *      (Natively 'haddr_t', always a 64-bit field)
 *
 * `size_t_blob` (uint64_t)
 *      A number indicating how large a size_t is on the sending system.
 *      Must be set to (uint64_t)((size_t)(-1))
 *          (maximum possible value of size_t, cast to uint64_t).
 *      The receiving system inspects this value -- if the local (remote)
 *      size_t is smaller than that of the Sender, issues a warning.
 *      Not an error, as:
 *      1. It is assumed that underlying file systems/drivers have become
 *         smart enough to handle file sizes that otherwise might be
 *         constrained.
 *      2. The Mirror Writer ingests bytes to write multiple 'slices' if the
 *         size is greater than H5FD_MIRROR_DATA_BUFFER_MAX, regardless of
 *         any size_t storage size disparity.
 *
 * `filename` (char[])
 *      String giving the filename and path of file to open.
 *
 * ---------------------------------------------------------------------------
 */
typedef struct H5FD_mirror_xmit_open_t {
    H5FD_mirror_xmit_t pub;
    uint32_t           flags;
    uint64_t           maxaddr;
    uint64_t           size_t_blob;
    char               filename[H5FD_MIRROR_XMIT_FILEPATH_MAX];
} H5FD_mirror_xmit_open_t;

/* ---------------------------------------------------------------------------
 * Structure: H5FD_mirror_xmit_reply_t
 *
 * Structure used by the remote receiver/worker/writer to respond to
 * a command from the VFD sender.
 *
 * `pub` (H5FD_mirror_xmit_t)
 *      Common transmission header, containing session information.
 *      Must be first.
 *
 * `status` (uint32_t)
 *      Number indicating whether the command was successful or if an
 *      occured.
 *      Allowed values are H5FD_MIRROR_STATUS_OK and
 *      H5FD_MIRROR_STATUS_ERROR.
 *
 * `message` (char[])
 *      Error message. Populated if and only if there was a problem.
 *      It is possible that a message may reach the end of the alloted
 *      space without a NULL terminator -- the onus is on the programmer to
 *      handle this situation.
 *
 * ---------------------------------------------------------------------------
 */
typedef struct H5FD_mirror_xmit_reply_t {
    H5FD_mirror_xmit_t pub;
    uint32_t           status;
    char               message[H5FD_MIRROR_STATUS_MESSAGE_MAX];
} H5FD_mirror_xmit_reply_t;

/* ---------------------------------------------------------------------------
 * Structure:   H5FD_mirror_xmit_write_t
 *
 * Structure containing data-write information from VFD sender.
 *
 * The data to be written is transmitted in subsequent, packets
 * and may be broken up into more than one transmission buffer.
 * The VFD sender and remote receiver/worker/writer must coordinate
 * the receipt of data.
 *
 * `pub` (H5FD_mirror_xmit_t)
 *      Common transmission header, containing session information.
 *      Must be first.
 *
 * `type` (uint8_t)
 *      Specifies datatype to be written.
 *      (Natively 'H5FD_mem_t', an enumerated type in H5Fpublic.h)
 *
 * `offset` (uint64_t)
 *      Start location of write in file.
 *      (Natively 'haddr_t', always a 64-bit field)
 *
 * `size` (uint64_t)
 *      Size of the data to be written, in bytes.
 *      (Natively 'size_t', accommodate the largest possible as 64-bits)
 *
 * ---------------------------------------------------------------------------
 */
typedef struct H5FD_mirror_xmit_write_t {
    H5FD_mirror_xmit_t pub;
    uint8_t            type;
    uint64_t           offset;
    uint64_t           size;
} H5FD_mirror_xmit_write_t;

/* Encode/decode routines are required to "pack" the xmit data into a known
 * byte format for transmission over the wire.
 *
 * All component numbers must be stored in "network" word order (Big-Endian).
 *
 * All components must be packed in the order given in the structure definition.
 *
 * All components must be packed with zero padding between.
 */

H5_DLL size_t H5FD__mirror_xmit_decode_uint16(uint16_t *out, const unsigned char *buf);
H5_DLL size_t H5FD__mirror_xmit_decode_uint32(uint32_t *out, const unsigned char *buf);
H5_DLL size_t H5FD__mirror_xmit_decode_uint64(uint64_t *out, const unsigned char *buf);
H5_DLL size_t H5FD__mirror_xmit_decode_uint8(uint8_t *out, const unsigned char *buf);
H5_DLL size_t H5FD__mirror_xmit_encode_uint16(unsigned char *dest, uint16_t v);
H5_DLL size_t H5FD__mirror_xmit_encode_uint32(unsigned char *dest, uint32_t v);
H5_DLL size_t H5FD__mirror_xmit_encode_uint64(unsigned char *dest, uint64_t v);
H5_DLL size_t H5FD__mirror_xmit_encode_uint8(unsigned char *dest, uint8_t v);

H5_DLL size_t H5FD_mirror_xmit_decode_header(H5FD_mirror_xmit_t *out, const unsigned char *buf);
H5_DLL size_t H5FD_mirror_xmit_decode_lock(H5FD_mirror_xmit_lock_t *out, const unsigned char *buf);
H5_DLL size_t H5FD_mirror_xmit_decode_open(H5FD_mirror_xmit_open_t *out, const unsigned char *buf);
H5_DLL size_t H5FD_mirror_xmit_decode_reply(H5FD_mirror_xmit_reply_t *out, const unsigned char *buf);
H5_DLL size_t H5FD_mirror_xmit_decode_set_eoa(H5FD_mirror_xmit_eoa_t *out, const unsigned char *buf);
H5_DLL size_t H5FD_mirror_xmit_decode_write(H5FD_mirror_xmit_write_t *out, const unsigned char *buf);

H5_DLL size_t H5FD_mirror_xmit_encode_header(unsigned char *dest, const H5FD_mirror_xmit_t *x);
H5_DLL size_t H5FD_mirror_xmit_encode_lock(unsigned char *dest, const H5FD_mirror_xmit_lock_t *x);
H5_DLL size_t H5FD_mirror_xmit_encode_open(unsigned char *dest, const H5FD_mirror_xmit_open_t *x);
H5_DLL size_t H5FD_mirror_xmit_encode_reply(unsigned char *dest, const H5FD_mirror_xmit_reply_t *x);
H5_DLL size_t H5FD_mirror_xmit_encode_set_eoa(unsigned char *dest, const H5FD_mirror_xmit_eoa_t *x);
H5_DLL size_t H5FD_mirror_xmit_encode_write(unsigned char *dest, const H5FD_mirror_xmit_write_t *x);

H5_DLL hbool_t H5FD_mirror_xmit_is_close(const H5FD_mirror_xmit_t *xmit);
H5_DLL hbool_t H5FD_mirror_xmit_is_lock(const H5FD_mirror_xmit_lock_t *xmit);
H5_DLL hbool_t H5FD_mirror_xmit_is_open(const H5FD_mirror_xmit_open_t *xmit);
H5_DLL hbool_t H5FD_mirror_xmit_is_reply(const H5FD_mirror_xmit_reply_t *xmit);
H5_DLL hbool_t H5FD_mirror_xmit_is_set_eoa(const H5FD_mirror_xmit_eoa_t *xmit);
H5_DLL hbool_t H5FD_mirror_xmit_is_write(const H5FD_mirror_xmit_write_t *xmit);
H5_DLL hbool_t H5FD_mirror_xmit_is_xmit(const H5FD_mirror_xmit_t *xmit);

#ifdef __cplusplus
}
#endif

#endif /* H5_HAVE_MIRROR_VFD */

#endif /* H5FDmirror_priv_H */
