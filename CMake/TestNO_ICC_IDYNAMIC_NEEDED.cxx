#ifdef __INTEL_COMPILER
        //If -i_dynamic is required (i.e. icc v7.1 on Redhat 9 or similar glibc version),
        //this simple program will fail to compile.
        #include <iostream>
        int main(int argc, char * argv[]) { return 1; }
#else //__INTEL_COMPILER
        // If not the INTEL compiler, just fall though to simplest program
        int main(int argc, char * argv[]) { return 1; }
#endif //__INTEL_COMPILER
