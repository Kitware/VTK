/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Packet Table wrapper classes
 *
 * Wraps the H5PT Packet Table C functions in C++ objects
 *
 * Nat Furrer and James Laird
 * February 2004
 */

/* High-level library internal header file */
#include "H5HLprivate2.h"

#include "H5PacketTable.h"

    /********************************/
    /* PacketTable superclass       */
    /********************************/

    /* "Open" Constructor
     * Opens an existing packet table, which can contain either fixed-length or
     * variable-length packets.
     */
    PacketTable::PacketTable(hid_t fileID, char* name)
    {
        table_id = H5PTopen( fileID, name);
    }

    /* Destructor
     * Cleans up the packet table
     */
    PacketTable::~PacketTable()
    {
        H5PTclose( table_id);
    }

    /* IsValid
     * Returns true if this packet table is valid, false otherwise.
     * Use this after the constructor to ensure HDF did not have
     * any trouble making or opening the packet table.
     */
    bool PacketTable::IsValid()
    {
        if (H5PTis_valid(table_id) == 0)
            return true;
        else
            return false;
    }

#ifdef VLPT_REMOVED
    /* IsVariableLength
     * Return 1 if this packet table is a Variable Length packet table,
     * return 0 if it is Fixed Length.  Returns -1 if the table is
     * invalid (not open).
     */
    int PacketTable::IsVariableLength()
    {
        return H5PTis_varlen(table_id);
    }
#endif /* VLPT_REMOVED */

    /* ResetIndex
     * Sets the index to point to the first packet in the packet table
     */
    void PacketTable::ResetIndex()
    {
        H5PTcreate_index(table_id);
    }

    /* SetIndex
     * Sets the index to point to the packet specified by index.
     * Returns 0 on success, negative on failure (if index is out of bounds)
     */
    int PacketTable::SetIndex(hsize_t index)
    {
        return H5PTset_index(table_id, index);
    }

    /* SetIndex
     * Sets the index to point to the packet specified by index.
     * Returns 0 on success, negative on failure (if index is out of bounds)
     */
    hsize_t PacketTable::GetIndex(int &error)
    {
        hsize_t index;

        error = H5PTget_index(table_id, &index);
        if(error < 0)
           return 0;
        else
           return index;
    }

    /* GetPacketCount
     * Returns the number of packets in the packet table.  Error
     * is set to 0 on success.  On failure, returns 0 and
     * error is set to negative.
     */
    hsize_t PacketTable::GetPacketCount(int& error)
    {
        hsize_t npackets;

        error = H5PTget_num_packets( table_id, (hsize_t *)&npackets);
        return npackets;
    }

    /********************************/
    /* Fixed-Length Packet Table    */
    /********************************/

    /* Constructor
     * Creates a packet table in which to store fixed length packets.
     * Takes the ID of the file the packet table will be created in, the name of
     * the packet table, the ID of the datatype of the set, and the size
     * of a memory chunk used in chunking.
     */
    FL_PacketTable::FL_PacketTable(hid_t fileID, char* name, hid_t dtypeID, hsize_t chunkSize, int compression)
    {
        table_id = H5PTcreate_fl ( fileID, name, dtypeID, chunkSize, compression);
    }

    /* "Open" Constructor
     * Opens an existing fixed-length packet table.
     * Fails if the packet table specified is variable-length.
     */
    FL_PacketTable::FL_PacketTable(hid_t fileID, char* name) : PacketTable(fileID, name)
    {
#ifdef VLPT_REMOVED
        if( H5PTis_varlen(table_id) != 0 )    // If this is not a fixed-length table
        {
            H5PTclose(table_id);
            table_id = -1;
        }
#endif /* VLPT_REMOVED */
    }

    /* AppendPacket
     * Adds a single packet to the packet table.  Takes a pointer
     * to the location of the data in memory.
     * Returns 0 on success, negative on failure
     */
    int FL_PacketTable::AppendPacket(void * data)
    {
        return H5PTappend(table_id, 1, data);
    }

    /* AppendPackets (multiple packets)
     * Adds multiple packets to the packet table.  Takes the number of packets
     * to be added and a pointer to their location in memory.
     * Returns 0 on success, -1 on failure.
     */
    int FL_PacketTable::AppendPackets(size_t numPackets, void * data)
    {
        return H5PTappend(table_id, numPackets, data);
    }

    /* GetPacket (indexed)
     * Gets a single packet from the packet table.  Takes the index
     * of the packet (with 0 being the first packet) and a pointer
     * to memory where the data should be stored.
     * Returns 0 on success, negative on failure
     */
    int FL_PacketTable::GetPacket(hsize_t index, void * data)
    {
        return H5PTread_packets(table_id, index, 1, data);
    }

    /* GetPackets (multiple packets)
     * Gets multiple packets at once, all packets between
     * startIndex and endIndex inclusive.  Also takes a pointer to
     * the memory where these packets should be stored.
     * Returns 0 on success, negative on failure.
     */
    int FL_PacketTable::GetPackets(hsize_t startIndex, hsize_t endIndex, void * data)
    {
        // Make sure the range of indexes is valid
        if (startIndex > endIndex)
            return -1;

        return  H5PTread_packets(table_id, startIndex, (size_t)(endIndex-startIndex+1), data);
    }

    /* GetNextPacket (single packet)
     * Gets the next packet in the packet table.  Takes a pointer to
     * memory where the packet should be stored.
     * Returns 0 on success, negative on failure.  Index
     * is not advanced to the next packet on failure.
     */
    int FL_PacketTable::GetNextPacket(void * data)
    {
        return H5PTget_next(table_id, 1, data);
    }

    /* GetNextPackets (multiple packets)
     * Gets the next numPackets packets in the packet table.  Takes a
     * pointer to memory where these packets should be stored.
     * Returns 0 on success, negative on failure.  Index
     * is not advanced on failure.
     */
    int FL_PacketTable::GetNextPackets(size_t numPackets, void * data)
    {
        return H5PTget_next(table_id, numPackets, data);
    }


#ifdef VLPT_REMOVED
    /********************************/
    /* Variable-Length Packet Table */
    /********************************/

    /* Constructor
     * Creates a packet table in which to store variable length packets.
     * Takes the ID of the file the packet table will be created in, the name of
     * the packet table, and the size of a memory chunk used in chunking.
     */
    VL_PacketTable::VL_PacketTable(hid_t fileID, char* name, hsize_t chunkSize)
    {
        table_id = H5PTcreate_vl ( fileID, name, chunkSize);
    }

    /* "Open" Constructor
     * Opens an existing variable-length packet table.
     * Fails if the packet table specified is fixed-length.
     */
    VL_PacketTable::VL_PacketTable(hid_t fileID, char* name) : PacketTable(fileID, name)
    {
        if( H5PTis_varlen(table_id) != 1 )    // If this is not a variable-length table
        {
            H5PTclose(table_id);
            table_id = -1;
        }
    }

    /* AppendPacket (variable-length)
     * Adds a single variable-length packet to the packet table.
     * Takes a pointer to the location of the data in memory and the length of the data
     * in bytes.
     * Returns 0 on success, negative on failure.
     */
       int VL_PacketTable::AppendPacket(void * data, size_t length)
    {
        hvl_t packet;

        packet.len = length;
        packet.p = data;

        return H5PTappend(table_id, 1, &packet);
    }

    /* AppendPackets (multiple packets)
     * Adds multiple variable-length packets to the packet table.  Takes the
     * number of
     * packets to be added and a pointer to an array of hvl_t structs in memory.
     * Returns 0 on success, negative on failure.
     */
    int VL_PacketTable::AppendPackets(size_t numPackets, hvl_t * data)
    {
        return H5PTappend(table_id, numPackets, data);
    }

    /* GetPacket (indexed)
     * Gets a single variable-length packet from the packet table.  Takes the
     * index of the packet (with 0 being the first packet) and a pointer
     * to a hvl_t struct in which to store the packet's size and location.
     * Returns 0 on success, negative on failure.
     */
    int VL_PacketTable::GetPacket(hsize_t index, hvl_t * data)
    {
        return H5PTread_packets(table_id, index, 1, data);
    }

    /* GetPackets (multiple packets)
     * Gets multiple variable-length packets at once, all packets between
     * startIndex and endIndex inclusive.  Takes a pointer to an array
     * of hvl_t structs in memory in which to store pointers to the packets.
     * Returns 0 on success, negative on failure.
     */
    int VL_PacketTable::GetPackets(hsize_t startIndex, hsize_t endIndex, hvl_t * data)
    {
        // Make sure the range of indexes is valid
        if (startIndex > endIndex)
            return -1;

        return  H5PTread_packets(table_id, startIndex, endIndex-startIndex+1, data);
    }

    /* GetNextPacket (single packet)
     * Gets the next packet in the packet table.  Takes a pointer to
     * an hvl_t struct where the packet should be stored.
     * Returns 0 on success, negative on failure.  Index
     * is not advanced to the next packet on failure.
     */
    int VL_PacketTable::GetNextPacket(hvl_t * data)
    {
        return H5PTget_next(table_id, 1, data);
    }

    /* GetNextPackets (multiple packets)
     * Gets the next numPackets packets in the packet table.  Takes a
     * pointer to an array of hvl_t structs where pointers to the packets
     * should be stored.
     * Returns 0 on success, negative on failure.  Index
     * is not advanced on failure.
     */
    int VL_PacketTable::GetNextPackets(size_t numPackets, hvl_t * data)
    {
        return H5PTget_next(table_id, numPackets, data);
    }

    /* FreeReadbuff
     * Frees the buffers created when variable-length packets are read.
     * Takes the number of hvl_t structs to be freed and a pointer to their
     * location in memory.
     * Returns 0 on success, negative on error.
     */
    int VL_PacketTable::FreeReadbuff(size_t numStructs, hvl_t * buffer)
    {
        return H5PTfree_vlen_readbuff( table_id, numStructs, buffer);
    }
#endif /* VLPT_REMOVED */
