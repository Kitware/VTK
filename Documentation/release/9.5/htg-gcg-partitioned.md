# Support Partitioned Input in HyperTreeGridGhostCellsGenerator

The HTG GhostCellsGenerator now natively supports Partitioned inputs, using the first non null partition found as the HTG to process. Before, partitions were processed one by one, causing issues on multi-partition data.
