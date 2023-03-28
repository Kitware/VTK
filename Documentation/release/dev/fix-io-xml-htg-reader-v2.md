## Fix IO XML HyperTreeGrid Reader v2

Fix a reading fault on vector fields in IO XML HyperTreeGrid Reader, version 2.

The ReadArrayValues ​​method reads the values ​​of an array based on the offsets on the values, with the caller taking into account the number of components specific to each array (which was not done here, hence the bug).

Instead of modifying the call to this method, a new ReadArrayTuples method will be called. It automatically takes into account the number of components in the array in order to be called elsewhere (pooling).
