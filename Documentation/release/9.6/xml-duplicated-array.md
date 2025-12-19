## Only keep first array if duplicated names in xml readers

When reading a dataset with multiple point/cell/field/row data arrays with the same name, we now display a warning
to inform that only the first one is kept, all other will be discarded.

Before these changes, some readers would keep the first, others the last, and it would often result in an error
further in the reader's code.
