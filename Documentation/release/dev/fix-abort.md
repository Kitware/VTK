# Change abort functionality in AMR, Core, and Extraction filters

Filter will now call `CheckAbort` once every 10% of data or every
thousand entries (whichever is smaller).
