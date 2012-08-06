/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id */ 
/*  Date : $Date$ */
/*  Version : $Revision$  */
/*                                                                 */
/*  Author:Kenji Takizawa (Team for Advanced Flow Simulation and Modeling) */
/*                                                                 */
/*     Copyright @ 2008 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include "XdmfValuesBinary.h"
#include "XdmfDataStructure.h"
#include "XdmfArray.h"
//#include "XdmfHDF.h"
#include "XdmfDOM.h"



#include <exception>
#include <cassert>
#ifdef XDMF_USE_GZIP
#include "gzstream.h"
#endif

#ifdef XDMF_USE_BZIP2
#include "bz2stream.h"
#endif




//#include <sys/stat.h>
//#include <cassert>
//Internal Classes
template<size_t T>
struct ByteSwaper {
    static inline void swap(void*p){}
    static inline void swap(void*p,XdmfInt64 length){
        char* data = static_cast<char*>(p);
        for(XdmfInt64 i=0;i<length;++i, data+=T){
            ByteSwaper<T>::swap(data);
        }
    }
};

template<>
void ByteSwaper<2>::swap(void*p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[1]; data[1] = one_byte;
};
template<>
void ByteSwaper<4>::swap(void*p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
    one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
};
template<>
void ByteSwaper<8>::swap(void*p){
    char one_byte;
    char* data = static_cast<char*>(p);
    one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
    one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
    one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
    one_byte = data[3]; data[3] = data[4]; data[4] = one_byte;
};
void XdmfValuesBinary::byteSwap(XdmfArray * RetArray){
    if(needByteSwap()){
        switch(RetArray->GetElementSize()){
        case 1:
            break;
        case 2:
            ByteSwaper<2>::swap(RetArray->GetDataPointer(),RetArray->GetNumberOfElements());
            break;
        case 4:
            ByteSwaper<4>::swap(RetArray->GetDataPointer(),RetArray->GetNumberOfElements());
            break;
        case 8:
            ByteSwaper<8>::swap(RetArray->GetDataPointer(),RetArray->GetNumberOfElements());
            break;
        default:
            break;
        }
    }
}
class HyperSlabReader:public XdmfObject{//
    XdmfInt64 ncontiguous;//byte
    XdmfInt64 start[XDMF_MAX_DIMENSION];//byte
    XdmfInt64 stride[XDMF_MAX_DIMENSION];//byte
    XdmfInt64 last[XDMF_MAX_DIMENSION];//byte
    XdmfInt64 count[XDMF_MAX_DIMENSION];//size
    XdmfInt64 rank;

    void toTotal(const XdmfInt64 * dims, XdmfInt32 byte, XdmfInt64 * data){
        data[this->rank-1] *= byte;
        for(XdmfInt32 i = 1; i< this->rank; ++i){
            for(XdmfInt32 j=i; j<this->rank; ++j){
                data[i-1] *= dims[j];
            }
            data[i-1] *= byte;
        }
    }
    void read(XdmfInt32 k, char *& pointer, istream &is){
        is.seekg(this->start[k], std::ios::cur);
        //XdmfDebug("Skip: " << this->start[k]<<", "<<k );
        if(k==rank-1){
            XdmfDebug("Read: " << ncontiguous);
            is.read(pointer, ncontiguous);
            pointer += ncontiguous;
            for(XdmfInt64 i=1,l=this->count[k];i<l;++i){
                //XdmfDebug("Skip: " << this->stride[k] <<", " << k );
                is.seekg(this->stride[k], std::ios::cur);
                is.read(pointer, ncontiguous);
                //XdmfDebug("Read: " << ncontiguous);
                pointer += ncontiguous;
            }
        }else{
            read(k+1,pointer,is);
            for(XdmfInt64 i=1,l=this->count[k];i<l;++i){
                is.seekg(this->stride[k], std::ios::cur);
                //XdmfDebug("Skip: " << this->stride[k] << ", "<< k);
                read(k+1,pointer,is);
            }
        }
        //XdmfDebug("Skip: " << this->last[k] << ", " << k);
        is.seekg(this->last[k], std::ios::cur);
    }

public:
    HyperSlabReader(XdmfInt32 rank, XdmfInt32 byte, const XdmfInt64 * dims, const XdmfInt64 * start, const XdmfInt64 * lstride, const XdmfInt64 * lcount){
        assert(rank>0 && rank<XDMF_MAX_DIMENSION);
        this->rank = rank;
        XdmfInt64 d[XDMF_MAX_DIMENSION];
        for(XdmfInt32 i =0;i<rank;++i){
            this->start[i] = start[i];
            this->stride[i] = lstride[i]-1;
            this->count[i] = lcount[i];
            d[i] = dims[i];
        }
        //reduce rank
        for(XdmfInt32 i =rank-1;i>0;--i){
            if(this->start[i]==0 && this->stride[i]==0 && this->count[i]==dims[i]){
                --this->rank;
            }else{
                break;
            }
        }
        if(this->rank != rank){
            XdmfDebug("Reduce Rank: " << rank << " to " << this->rank);
            for(XdmfInt32 i = this->rank;i<rank;++i){
                byte *= lcount[i];
            }
        }
        for(XdmfInt32 i =0;i<this->rank;++i){
            this->last[i] = d[i] - (this->start[i] + (this->stride[i]+1)*(this->count[i]-1) + 1);
        }
        toTotal(d,byte, this->start);
        toTotal(d,byte, this->stride);
        toTotal(d,byte, this->last);

        ncontiguous=byte;
        if(this->stride[this->rank-1]==0){
            ncontiguous *= this->count[this->rank-1];
            this->count[this->rank-1] = 1;
        }
        XdmfDebug("Contiguous byte: " << ncontiguous);
    }
    ~HyperSlabReader(){
    }
    void read(char * pointer, istream &is){
        read(static_cast<XdmfInt32>(0),pointer,is);
    }
};


size_t XdmfValuesBinary::getSeek(){
    if(this->Seek==NULL)return 0;
    return static_cast<size_t>(atoi(this->Seek));
}


enum XdmfValuesBinary::CompressionType XdmfValuesBinary::getCompressionType(){
    if(this->Compression==NULL||XDMF_WORD_CMP(Compression, "Raw")){
        return Raw;
    }
    if(XDMF_WORD_CMP(Compression, "Zlib")){
        return Zlib;
    }
    if(XDMF_WORD_CMP(Compression, "BZip2")){
        return BZip2;
    }
    return Raw;
}
XdmfValuesBinary::XdmfValuesBinary() {
    this->Endian = NULL;
    this->Seek = NULL;
    this->Compression = NULL;
    this->SetFormat(XDMF_FORMAT_BINARY);
}
XdmfValuesBinary::~XdmfValuesBinary() {
}

XdmfArray *
XdmfValuesBinary::Read(XdmfArray *anArray){
    if(!this->DataDesc){
        XdmfErrorMessage("DataDesc has not been set");
        return(NULL);
    }
    XdmfArray   *RetArray = anArray;
    ostrstream FullFileName;
    // Allocate anArray if Necessary
    if(!RetArray){
        RetArray = new XdmfArray();
        RetArray->CopyType(this->DataDesc);
        RetArray->CopyShape(this->DataDesc);
        RetArray->CopySelection(this->DataDesc);
        RetArray->Allocate();
    }
    XdmfDebug("Accessing Binary CDATA");
    {
        XdmfConstString Value = this->Get("Endian");
        if(Value){
            this->SetEndian(Value);
        }else{
            this->Endian = NULL;
        }
    }
    {
        XdmfConstString Value = this->Get("Seek");
        if(Value){
            this->SetSeek(Value);
        }else{
            this->Seek = NULL;
        }
    }
    {
        XdmfConstString Value = this->Get("Compression");
        if(Value){
            this->SetCompression(Value);
        }else{
            this->Compression = NULL;
        }
    }

    XdmfString  DataSetName = 0;
    XDMF_STRING_DUPLICATE(DataSetName, this->Get("CDATA"));
    XDMF_WORD_TRIM(DataSetName);
    XdmfInt64 dims[XDMF_MAX_DIMENSION];
    XdmfInt32 rank = this->DataDesc->GetShape(dims);
    XdmfInt64 total = 1;
    for(XdmfInt32 i=0;i<rank;++i){
        total *= dims[i];
    }
    XdmfDebug("Data Size : " << total);
    XdmfDebug("Size[Byte]: " << RetArray->GetCoreLength());
    XdmfDebug("     Byte   " << RetArray->GetElementSize());
    //check
    //    struct stat buf;
    //    stat(DataSetName, &buf);
    //    assert(buf.st_size == RetArray->GetCoreLength());

    //ifstream fs(DataSetName,std::ios::binary);
    if( RetArray->GetDataPointer() == NULL ){
        XdmfErrorMessage("Memory Object Array has no data storage");
        return( NULL );
    }
    istream * fs = NULL;
    if( ( strlen( this->DOM->GetWorkingDirectory() ) > 0 ) && 
      ( DataSetName[0] != '/' ) ){
        FullFileName << this->DOM->GetWorkingDirectory() << "/";
    }
    FullFileName << DataSetName << ends;
    char * path = FullFileName.rdbuf()->str();
    XdmfDebug("Opening Binary Data for Reading : " << FullFileName);


    //char * path = new char [ strlen(this->DOM->GetWorkingDirectory())+strlen(DataSetName) + 1 ];
    //strcpy(path, this->DOM->GetWorkingDirectory());
    //strcpy(path+strlen(this->DOM->GetWorkingDirectory()), DataSetName);
    try{
        size_t seek = this->getSeek();
        switch(getCompressionType()){
        case Zlib:
            XdmfDebug("Compression: Zlib");
            #ifdef XDMF_USE_GZIP
            fs = new igzstream(path, std::ios::binary|std::ios::in);
            if(seek!=0){
                XdmfDebug("Seek has not supported with Zlib.");
            }
            break;
            #else
                        XdmfDebug("GZip Lib is needed.");
            #endif
        case BZip2:
            XdmfDebug("Compression: Bzip2");
#ifdef XDMF_USE_BZIP2
            fs = new ibz2stream(path);//, std::ios::binary|std::ios::in);
            if(seek!=0){
                XdmfDebug("Seek has not supported with Bzip2.");
            }
            break;
#else
            XdmfDebug("BZIP2 LIBRARY IS NEEDED.");
#endif
        default:
            fs = new ifstream(path, std::ios::binary);
            fs->seekg(seek);
            XdmfDebug("Seek: " << seek);
            break;
        }
        fs->exceptions( ios::failbit | ios::badbit );
        if(!fs->good()){
            XdmfErrorMessage("Can't Open File " << DataSetName);
            //return(NULL);
        }
        if( this->DataDesc->GetSelectionType() == XDMF_HYPERSLAB ){
            XdmfDebug("Hyperslab data");
            XdmfInt32  Rank;
            XdmfInt64  Start[ XDMF_MAX_DIMENSION ];
            XdmfInt64  Stride[ XDMF_MAX_DIMENSION ];
            XdmfInt64  Count[ XDMF_MAX_DIMENSION ];
            Rank = this->DataDesc->GetHyperSlab( Start, Stride, Count );
            HyperSlabReader wrapper(Rank,RetArray->GetElementSize(),dims,Start,Stride, Count);
            wrapper.read(reinterpret_cast<char*>(RetArray->GetDataPointer()), *fs);
        }else{
            XdmfDebug("Regular data");
            fs->read(reinterpret_cast<char*>(RetArray->GetDataPointer()), RetArray->GetCoreLength());
        }
    } catch( std::exception& e){
        XdmfErrorMessage(e.what());
        delete fs;
        //delete path;
        return( NULL );
    }

    //fs->close();?
    delete fs;
    //delete path;
    byteSwap(RetArray);
    return RetArray;
}


XdmfInt32
XdmfValuesBinary::Write(XdmfArray *anArray, XdmfConstString aHeavyDataSetName){
    if(!aHeavyDataSetName) aHeavyDataSetName = this->GetHeavyDataSetName();
    if(anArray->GetHeavyDataSetName()){
        aHeavyDataSetName = (XdmfConstString)anArray->GetHeavyDataSetName();
    }else{
        return(XDMF_FAIL);
    }
    XdmfDebug("Writing Values to " << aHeavyDataSetName);
    if(!this->DataDesc ){
        XdmfErrorMessage("DataDesc has not been set");
        return(XDMF_FAIL);
    }
    if(!anArray){
        XdmfErrorMessage("Array to Write is NULL");
        return(XDMF_FAIL);
    }
    if( anArray->GetDataPointer() == NULL ){
        XdmfErrorMessage("Memory Object Array has no data storage");
        return(XDMF_FAIL);
    }
    char* hds;
    XDMF_STRING_DUPLICATE(hds, aHeavyDataSetName);
    XDMF_WORD_TRIM( hds );
    this->Set("CDATA", hds);
    byteSwap(anArray);
    ostream * fs = NULL;
    char * path = new char [ strlen(this->DOM->GetWorkingDirectory())+strlen(aHeavyDataSetName) + 1 ];
    strcpy(path, this->DOM->GetWorkingDirectory());
    strcpy(path+strlen(this->DOM->GetWorkingDirectory()), aHeavyDataSetName);
    try{
        //ofstream fs(aHeavyDataSetName,std::ios::binary);
        switch(getCompressionType()){
        case Zlib:
            XdmfDebug("Compression: ZLIB");
            #ifdef XDMF_USE_GZIP
            // fs = gzip(fs);
            fs = new ogzstream(path, std::ios::binary|std::ios::out);
            break;
            #else
                            XdmfDebug("GZIP LIBRARY IS NEEDED.");
            #endif
        case BZip2:
            XdmfDebug("Compression: BZIP2");
#ifdef XDMF_USE_BZIP2
            fs = new obz2stream(path);//, std::ios::binary|std::ios::out);
            break;
#else
            XdmfDebug("BZIP2 LIBRARY IS NEEDED.");
#endif
        default:
            fs = new ofstream(path, std::ios::binary);
            //fs->seekg(seek);
            //XdmfDebug("Seek: " << seek);
            break;
        }
        fs->exceptions( ios::failbit | ios::badbit );
        if(!fs->good()){
            XdmfErrorMessage("Can't Open File " << aHeavyDataSetName);
        }
        fs->write(reinterpret_cast<char*>(anArray->GetDataPointer()), anArray->GetCoreLength());
    }catch( std::exception& ){
        //fs.close();
        byteSwap(anArray);
        delete fs;
        delete [] hds;
        delete [] path;
        return(XDMF_FAIL);
    }
    byteSwap(anArray);
    delete [] fs;
    delete [] hds;
    delete [] path;
    return(XDMF_SUCCESS);
}
#ifdef CMAKE_WORDS_BIGENDIAN
bool XdmfValuesBinary::needByteSwap(){
    return XDMF_WORD_CMP(Endian, "Little");
}
#else
bool XdmfValuesBinary::needByteSwap(){
    return XDMF_WORD_CMP(Endian, "Big");
}
#endif
// vim: expandtab sw=4 :
