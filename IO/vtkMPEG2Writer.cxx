/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPEG2Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPEG2Writer.h"

#include "vtkDataSetAttributes.h"
#include "vtkDirectory.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkErrorCode.h"

#include <vtkstd/map>
#include <vtkstd/string>

#include "mpeg2enc_config.h"
#include "mpeg2enc_global.h"

/* private prototypes */

//---------------------------------------------------------------------------
class vtkMPEG2WriterInternal 
{
public:
  vtkMPEG2WriterInternal();
  ~vtkMPEG2WriterInternal();

  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkImageData> > StringToImageMap;

  int StoreImage(const char* name, vtkImageData* id);
  unsigned char* GetImagePtr(const char* name);
  int RemoveImage(const char* name);

  int Dim[2];

  void ReadParmFile( );
  void Init();
  void ReadQuantMat();

  MPEG2_structure* GetMPEG2Structure() { return this->Structure; }

private:
  StringToImageMap ImagesMap;
  MPEG2_structure* Structure;
};

//---------------------------------------------------------------------------
extern "C"
{
  void vtkMPEG2WriterReportError( const char *text )
  {
    vtkGenericWarningMacro(<< text);
  }

  unsigned char* vtkMPEG2WriterInternalGetImagePtr(const char* fname, 
                                                   void *mpeg2_writer_internal)
  {
    return (static_cast<vtkMPEG2WriterInternal*>(mpeg2_writer_internal))->GetImagePtr(fname);
  }
}

//---------------------------------------------------------------------------
vtkMPEG2WriterInternal::vtkMPEG2WriterInternal()
{
  this->Dim[0] = 0;
  this->Dim[1] = 0;

  this->Structure = new MPEG2_structure;
}

//---------------------------------------------------------------------------
vtkMPEG2WriterInternal::~vtkMPEG2WriterInternal()
{
  delete this->Structure;
  this->Structure = 0;
}

//---------------------------------------------------------------------------
int vtkMPEG2WriterInternal::StoreImage(const char* name, vtkImageData* iid)
{
  if ( !name )
    {
    return 0;
    }

  // We need to flip the image vertically

  vtkImageData* id = vtkImageData::New();
  id->CopyStructure(iid);
  id->SetNumberOfScalarComponents(iid->GetNumberOfScalarComponents());

  vtkInformation *pipelineInfo = id->GetPipelineInformation();
  if (pipelineInfo)
    {
    vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(
      pipelineInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
      vtkDataSetAttributes::SCALARS);
    if (scalarInfo)
      {
      scalarInfo->Set(vtkDataObject::FIELD_ARRAY_TYPE(), iid->GetScalarType());
      }
    }
  
  int dims[3];
  id->GetDimensions(dims);

  int row_length = dims[0] * id->GetNumberOfScalarComponents();

  const unsigned char *src_ptr = 
    (const unsigned char *)iid->GetScalarPointer();
  src_ptr += iid->GetNumberOfPoints() * iid->GetNumberOfScalarComponents() 
    - row_length;

  unsigned char *dest_ptr = (unsigned char *)id->GetScalarPointer();

  // We assume there is only one slice

  for (int j = 0; j < dims[1]; j++)
    {
    memcpy(dest_ptr, src_ptr, row_length);
    dest_ptr += row_length;
    src_ptr -= row_length;
    }

  this->ImagesMap[name] = id;
  id->Delete();

  return 1;
}

//---------------------------------------------------------------------------
unsigned char* vtkMPEG2WriterInternal::GetImagePtr(const char* fname)
{
  if ( !fname )
    {
    return 0;
    }
  vtkMPEG2WriterInternal::StringToImageMap::iterator it
    = this->ImagesMap.find(fname);
  if ( it == this->ImagesMap.end() )
    {
    return 0;
    }
  vtkImageData* id = it->second.GetPointer();
  return static_cast<unsigned char*>(id->GetScalarPointer());
}

//---------------------------------------------------------------------------
int vtkMPEG2WriterInternal::RemoveImage(const char* fname)
{
  if ( !fname )
    {
    return 0;
    }
  vtkMPEG2WriterInternal::StringToImageMap::iterator it
    = this->ImagesMap.find(fname);
  if ( it == this->ImagesMap.end() )
    {
    return 0;
    }
  this->ImagesMap.erase(it);
  return 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMPEG2Writer);

//---------------------------------------------------------------------------
vtkMPEG2Writer::vtkMPEG2Writer()
{
  this->Internals = 0;
  this->Time = 0;
  this->ActualWrittenTime = 0;
}

//---------------------------------------------------------------------------
vtkMPEG2Writer::~vtkMPEG2Writer()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::Start()
{
  // Error checking
  this->Error = 1;
  
  if ( this->Internals )
    {
    vtkErrorMacro("Movie already started");
    this->SetErrorCode(vtkGenericMovieWriter::InitError);
    return;
    }
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Write:Please specify an input!");
    this->SetErrorCode(vtkGenericMovieWriter::NoInputError);
    return;
    }
  if (!this->FileName)
    {
    vtkErrorMacro(<<"Write:Please specify a FileName");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
    }

  this->Internals = new vtkMPEG2WriterInternal;

  this->Error = 0;
  this->Time = 0;
  this->ActualWrittenTime = 0;

  this->Initialized = 0;
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::Write()
{
  if ( !this->Internals )
    {
    vtkErrorMacro("Movie not started");
    this->Error = 1;
    this->SetErrorCode(vtkGenericMovieWriter::InitError);
    return;
    }

  // get the data
  this->GetInput()->UpdateInformation();
  int *wExtent = this->GetInput()->GetWholeExtent();
  this->GetInput()->SetUpdateExtent(wExtent);
  this->GetInput()->Update();

  int dim[4];
  this->GetInput()->GetDimensions(dim);
  if ( this->Internals->Dim[0] == 0 && this->Internals->Dim[1] == 0 )
    {
    this->Internals->Dim[0] = dim[0];
    this->Internals->Dim[1] = dim[1];
    }

  if ( this->Internals->Dim[0] != dim[0] || this->Internals->Dim[1] != dim[1] )
    {
    vtkErrorMacro("Image not of the same size");
    this->SetErrorCode(vtkGenericMovieWriter::ChangedResolutionError);
    return;
    }

  if ( !this->Initialized )
    {
    this->Initialize();
    }

  
  MPEG2_structure* str = this->Internals->GetMPEG2Structure();
  char buffer[1024];
  sprintf(buffer, str->tplorg, this->Time + str->frame0);
  this->Internals->StoreImage(buffer, this->GetInput());
                 
  int last = MPEG2_putseq_one(this->ActualWrittenTime, this->Time,str);
  if ( last >= 0 )
    {
    sprintf(buffer, str->tplorg, last + str->frame0);
    this->Internals->RemoveImage(buffer);
    this->ActualWrittenTime ++;
    }
  this->Time++;
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::Initialize()
{
  MPEG2_structure* str = this->Internals->GetMPEG2Structure();
  str->quiet = 1;

  str->report_error = vtkMPEG2WriterReportError;
  str->get_image_ptr = vtkMPEG2WriterInternalGetImagePtr;
  str->mpeg2_writer_internal = this->Internals;

  /* read parameter file */
  this->Internals->ReadParmFile();


  /* read quantization matrices */
  this->Internals->ReadQuantMat();

  /* open output file */
  if (!(str->outfile=fopen(this->FileName,"wb")))
    {
    sprintf(str->errortext,"Couldn't create output file %s",this->FileName);
    (*(str->report_error))(str->errortext);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    }

  this->Internals->Init();

  MPEG2_rc_init_seq(str); /* initialize rate control */

  /* sequence header, sequence extension and sequence display extension */
  MPEG2_putseqhdr(str);
  if (!str->mpeg1)
    {
    MPEG2_putseqext(str);
    MPEG2_putseqdispext(str);
    }

  /* optionally output some text data (description, copyright or whatever) */
  if (strlen(str->id_string) > 1)
    MPEG2_putuserdata(str->id_string,str);


  this->Initialized = 1;
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::End()
{
  MPEG2_structure* str = this->Internals->GetMPEG2Structure();
  int last;
  while ( (last = MPEG2_putseq_one(this->ActualWrittenTime, this->Time-1,str)) >= 0 )
    {
    char buffer[1024];
    sprintf(buffer, str->tplorg, last + str->frame0);
    this->Internals->RemoveImage(buffer);
    this->ActualWrittenTime ++;
    }

  MPEG2_putseqend(str);

  fclose(str->outfile);
  if ( str->statfile )
    {
    fclose(str->statfile);
    }

  delete this->Internals;
  this->Internals = 0;
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkMPEG2WriterInternal::Init()
{
  int i, size;
  static int block_count_tab[3] = {6,8,12};

  MPEG2_initbits();
  MPEG2_init_fdct();
  MPEG2_init_idct();

  /* round picture dimensions to nearest multiple of 16 or 32 */
  this->Structure->mb_width = (this->Structure->horizontal_size+15)/16;
  this->Structure->mb_height = this->Structure->prog_seq ? (this->Structure->vertical_size+15)/16 : 2*((this->Structure->vertical_size+31)/32);
  this->Structure->mb_height2 = this->Structure->fieldpic ? this->Structure->mb_height>>1 : this->Structure->mb_height; /* for field pictures */
  this->Structure->width = 16*this->Structure->mb_width;
  this->Structure->height = 16*this->Structure->mb_height;

  this->Structure->chrom_width = (this->Structure->chroma_format==CHROMA444) ? this->Structure->width : this->Structure->width>>1;
  this->Structure->chrom_height = (this->Structure->chroma_format!=CHROMA420) ? this->Structure->height : this->Structure->height>>1;

  this->Structure->height2 = this->Structure->fieldpic ? this->Structure->height>>1 : this->Structure->height;
  this->Structure->width2 = this->Structure->fieldpic ? this->Structure->width<<1 : this->Structure->width;
  this->Structure->chrom_width2 = this->Structure->fieldpic ? this->Structure->chrom_width<<1 : this->Structure->chrom_width;

  this->Structure->block_count = block_count_tab[this->Structure->chroma_format-1];

  /* clip table */
  if (!(this->Structure->clp = (unsigned char *)malloc(1024)))
    {
    (*(this->Structure->report_error))("malloc failed\n");
    }
  this->Structure->clp+= 384;

  for (i=-384; i<640; i++)
    {
    this->Structure->clp[i] = (i<0) ? 0 : ((i>255) ? 255 : i);
    }

  for (i=0; i<3; i++)
    {
    size = (i==0) ? this->Structure->width*this->Structure->height : 
      this->Structure->chrom_width*this->Structure->chrom_height;

    if (!(this->Structure->newrefframe[i] = (unsigned char *)malloc(size)))
      (*(this->Structure->report_error))("malloc failed\n");
    if (!(this->Structure->oldrefframe[i] = (unsigned char *)malloc(size)))
      (*(this->Structure->report_error))("malloc failed\n");
    if (!(this->Structure->auxframe[i] = (unsigned char *)malloc(size)))
      (*(this->Structure->report_error))("malloc failed\n");
    if (!(this->Structure->neworgframe[i] = (unsigned char *)malloc(size)))
      (*(this->Structure->report_error))("malloc failed\n");
    if (!(this->Structure->oldorgframe[i] = (unsigned char *)malloc(size)))
      (*(this->Structure->report_error))("malloc failed\n");
    if (!(this->Structure->auxorgframe[i] = (unsigned char *)malloc(size)))
      (*(this->Structure->report_error))("malloc failed\n");
    if (!(this->Structure->predframe[i] = (unsigned char *)malloc(size)))
      (*(this->Structure->report_error))("malloc failed\n");
    }

  this->Structure->mbinfo = (struct mbinfo *)malloc(this->Structure->mb_width*this->Structure->mb_height2*sizeof(struct mbinfo));

  if (!this->Structure->mbinfo)
    (*(this->Structure->report_error))("malloc failed\n");

  this->Structure->blocks =
    (short (*)[64])malloc(this->Structure->mb_width*this->Structure->mb_height2*this->Structure->block_count*sizeof(short [64]));

  if (!this->Structure->blocks)
    (*(this->Structure->report_error))("malloc failed\n");

  /* open statistics output file */
  if (this->Structure->statname[0]=='-')
    this->Structure->statfile = 0;
  else if (!(this->Structure->statfile = fopen(this->Structure->statname,"w")))
    {
    sprintf(this->Structure->errortext,"Couldn't create statistics output file %s",this->Structure->statname);
    (*(this->Structure->report_error))(this->Structure->errortext);
    }
}

void vtkMPEG2WriterInternal::ReadParmFile( )
{
  int i;
  int h,m,s,f;
  //FILE *fd;
  //char line[256];
  static double ratetab[8]=
    {24000.0/1001.0,24.0,25.0,30000.0/1001.0,30.0,50.0,60000.0/1001.0,60.0};

  /*
  if (!(fd = fopen(fname,"r")))
  {
    sprintf(this->Structure->errortext,"Couldn't open parameter file %s",fname);
    (*(this->Structure->report_error))(this->Structure->errortext);
  }

  fgets(this->Structure->id_string,254,fd);
  fgets(line,254,fd); sscanf(line,"%s",this->Structure->tplorg);
  fgets(line,254,fd); sscanf(line,"%s",this->Structure->tplref);
  fgets(line,254,fd); sscanf(line,"%s",this->Structure->iqname);
  fgets(line,254,fd); sscanf(line,"%s",this->Structure->niqname);
  fgets(line,254,fd); sscanf(line,"%s",this->Structure->statname);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->inputtype);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->nframes);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->frame0);
  fgets(line,254,fd); sscanf(line,"%d:%d:%d:%d",&h,&m,&s,&f);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->N_val);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->M_val);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->mpeg1);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->fieldpic);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->horizontal_size);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->vertical_size);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->aspectratio);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->frame_rate_code);
  fgets(line,254,fd); sscanf(line,"%lf",&this->Structure->bit_rate);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->vbv_buffer_size);   
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->low_delay);     
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->constrparms);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->profile);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->level);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->prog_seq);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->chroma_format);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->video_format);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->color_primaries);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->transfer_characteristics);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->matrix_coefficients);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->display_horizontal_size);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->display_vertical_size);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->dc_prec);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->topfirst);
  fgets(line,254,fd); sscanf(line,"%d %d %d",
    this->Structure->frame_pred_dct_tab,this->Structure->frame_pred_dct_tab+1,this->Structure->frame_pred_dct_tab+2);
  
  fgets(line,254,fd); sscanf(line,"%d %d %d",
    this->Structure->conceal_tab,this->Structure->conceal_tab+1,this->Structure->conceal_tab+2);
  
  fgets(line,254,fd); sscanf(line,"%d %d %d",
    this->Structure->qscale_tab,this->Structure->qscale_tab+1,this->Structure->qscale_tab+2);

  fgets(line,254,fd); sscanf(line,"%d %d %d",
    this->Structure->intravlc_tab,this->Structure->intravlc_tab+1,this->Structure->intravlc_tab+2);
  fgets(line,254,fd); sscanf(line,"%d %d %d",
    this->Structure->altscan_tab,this->Structure->altscan_tab+1,this->Structure->altscan_tab+2);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->repeatfirst);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->prog_frame);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->P_val);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->reaction);
  fgets(line,254,fd); sscanf(line,"%lf",&this->Structure->avg_act);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->Xi);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->Xp);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->Xb);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->d0i);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->d0p);
  fgets(line,254,fd); sscanf(line,"%d",&this->Structure->d0b);
  */

  strcpy(this->Structure->id_string, "vtkMPEG2Writer");
  strcpy(this->Structure->tplorg, "%d");
  strcpy(this->Structure->tplref, "-");
  strcpy(this->Structure->iqname, "-");
  strcpy(this->Structure->niqname, "-");
  strcpy(this->Structure->statname, "-");
  this->Structure->inputtype = T_MEMPPM;
  this->Structure->nframes = 1000000;
  this->Structure->frame0 = 0;
  this->Structure->N_val = 21;
  this->Structure->M_val = 3;
  this->Structure->mpeg1 = 0;
  this->Structure->fieldpic = 0;
  this->Structure->horizontal_size = this->Dim[0];
  this->Structure->vertical_size = this->Dim[1];
  this->Structure->aspectratio = 1;
  this->Structure->frame_rate_code = 5;
  this->Structure->bit_rate = 5000000.0;
  this->Structure->vbv_buffer_size = 112;
  this->Structure->low_delay = 0;
  this->Structure->constrparms = 0;
  this->Structure->profile = 4;
  this->Structure->level = 4;
  this->Structure->prog_seq = 1;
  this->Structure->chroma_format = 1;
  this->Structure->video_format = 0;
  this->Structure->color_primaries = 5;
  this->Structure->transfer_characteristics = 5;
  this->Structure->matrix_coefficients = 4;
  this->Structure->display_horizontal_size = this->Dim[0];
  this->Structure->display_vertical_size = this->Dim[1];
  this->Structure->dc_prec = 2;
  this->Structure->topfirst = 1;
  this->Structure->frame_pred_dct_tab[0] = this->Structure->frame_pred_dct_tab[1] = this->Structure->frame_pred_dct_tab[2] = 0;
  this->Structure->conceal_tab[0] = this->Structure->conceal_tab[1] = this->Structure->conceal_tab[2] = 0;
  this->Structure->qscale_tab[0] = this->Structure->qscale_tab[1] = this->Structure->qscale_tab[2] = 1;
  this->Structure->intravlc_tab[0] = 1;
  this->Structure->intravlc_tab[1] = this->Structure->intravlc_tab[2] = 1;
  this->Structure->altscan_tab[0] = this->Structure->altscan_tab[1] = this->Structure->altscan_tab[2] = 0;
  this->Structure->repeatfirst = 0;
  this->Structure->prog_frame = 0;
  this->Structure->P_val = 0;
  this->Structure->reaction = 0;
  this->Structure->avg_act = 0;
  this->Structure->Xi = 0;
  this->Structure->Xp = 0;
  this->Structure->Xb = 0;
  this->Structure->d0i = 0;
  this->Structure->d0p = 0;
  this->Structure->d0b = 0;

  if (this->Structure->N_val<1)
    (*(this->Structure->report_error))("N must be positive");
  if (this->Structure->M_val<1)
    (*(this->Structure->report_error))("M must be positive");
  if (this->Structure->N_val%this->Structure->M_val != 0)
    (*(this->Structure->report_error))("N must be an integer multiple of M");

  this->Structure->motion_data = (struct motion_data *)malloc(this->Structure->M_val*sizeof(struct motion_data));
  if (!this->Structure->motion_data)
    (*(this->Structure->report_error))("malloc failed\n");

  this->Structure->motion_data[0].forw_hor_f_code = 2;
  this->Structure->motion_data[0].forw_vert_f_code = 2;
  this->Structure->motion_data[0].sxf = 11;
  this->Structure->motion_data[0].syf = 11;

  this->Structure->motion_data[1].forw_hor_f_code = 1;
  this->Structure->motion_data[1].forw_vert_f_code = 1;
  this->Structure->motion_data[1].sxf = 3;
  this->Structure->motion_data[1].syf = 3;
  
  this->Structure->motion_data[1].back_hor_f_code = 1;
  this->Structure->motion_data[1].back_vert_f_code = 1;
  this->Structure->motion_data[1].sxb = 7;
  this->Structure->motion_data[1].syb = 7;

  this->Structure->motion_data[2].forw_hor_f_code = 1;
  this->Structure->motion_data[2].forw_vert_f_code = 1;
  this->Structure->motion_data[2].sxf = 7;
  this->Structure->motion_data[2].syf = 7;
  
  this->Structure->motion_data[2].back_hor_f_code = 1;
  this->Structure->motion_data[2].back_vert_f_code = 1;
  this->Structure->motion_data[2].sxb = 3;
  this->Structure->motion_data[2].syb = 3;

  /*
  for (i=0; i<this->Structure->M_val; i++)
  {
    fgets(line,254,fd);
    sscanf(line,"%d %d %d %d",
      &this->Structure->motion_data[i].forw_hor_f_code, &this->Structure->motion_data[i].forw_vert_f_code,
      &this->Structure->motion_data[i].sxf, &this->Structure->motion_data[i].syf);

    if (i!=0)
    {
      fgets(line,254,fd);
      sscanf(line,"%d %d %d %d",
        &this->Structure->motion_data[i].back_hor_f_code, &this->Structure->motion_data[i].back_vert_f_code,
        &this->Structure->motion_data[i].sxb, &this->Structure->motion_data[i].syb);
    }
  }

  fclose(fd);
  */

  /* make flags boolean (x!=0 -> x=1) */
  this->Structure->mpeg1 = !!this->Structure->mpeg1;
  this->Structure->fieldpic = !!this->Structure->fieldpic;
  this->Structure->low_delay = !!this->Structure->low_delay;
  this->Structure->constrparms = !!this->Structure->constrparms;
  this->Structure->prog_seq = !!this->Structure->prog_seq;
  this->Structure->topfirst = !!this->Structure->topfirst;

  for (i=0; i<3; i++)
  {
    this->Structure->frame_pred_dct_tab[i] = !!this->Structure->frame_pred_dct_tab[i];
    this->Structure->conceal_tab[i] = !!this->Structure->conceal_tab[i];
    this->Structure->qscale_tab[i] = !!this->Structure->qscale_tab[i];
    this->Structure->intravlc_tab[i] = !!this->Structure->intravlc_tab[i];
    this->Structure->altscan_tab[i] = !!this->Structure->altscan_tab[i];
  }
  this->Structure->repeatfirst = !!this->Structure->repeatfirst;
  this->Structure->prog_frame = !!this->Structure->prog_frame;

  /* make sure MPEG specific parameters are valid */
  MPEG2_range_checks(this->Structure);

  this->Structure->frame_rate = ratetab[this->Structure->frame_rate_code-1];

  /* timecode -> frame number */
  h = m = s = f = 0;
  this->Structure->tc0 = h;
  this->Structure->tc0 = 60*this->Structure->tc0 + m;
  this->Structure->tc0 = 60*this->Structure->tc0 + s;
  this->Structure->tc0 = (int)(this->Structure->frame_rate+0.5)*this->Structure->tc0 + f;

  if (!this->Structure->mpeg1)
  {
    MPEG2_profile_and_level_checks(this->Structure);
  }
  else
  {
    /* MPEG-1 */
    if (this->Structure->constrparms)
    {
      if (this->Structure->horizontal_size>768
          || this->Structure->vertical_size>576
          || ((this->Structure->horizontal_size+15)/16)*((this->Structure->vertical_size+15)/16)>396
          || ((this->Structure->horizontal_size+15)/16)*((this->Structure->vertical_size+15)/16)*this->Structure->frame_rate>396*25.0
          || this->Structure->frame_rate>30.0)
      {
        if (!this->Structure->quiet)
          fprintf(stderr,"Warning: setting constrained_parameters_flag = 0\n");
        this->Structure->constrparms = 0;
      }
    }

    if (this->Structure->constrparms)
    {
      for (i=0; i<this->Structure->M_val; i++)
      {
        if (this->Structure->motion_data[i].forw_hor_f_code>4)
        {
          if (!this->Structure->quiet)
            fprintf(stderr,"Warning: setting constrained_parameters_flag = 0\n");
          this->Structure->constrparms = 0;
          break;
        }

        if (this->Structure->motion_data[i].forw_vert_f_code>4)
        {
          if (!this->Structure->quiet)
            fprintf(stderr,"Warning: setting constrained_parameters_flag = 0\n");
          this->Structure->constrparms = 0;
          break;
        }

        if (i!=0)
        {
          if (this->Structure->motion_data[i].back_hor_f_code>4)
          {
            if (!this->Structure->quiet)
              fprintf(stderr,"Warning: setting constrained_parameters_flag = 0\n");
            this->Structure->constrparms = 0;
            break;
          }

          if (this->Structure->motion_data[i].back_vert_f_code>4)
          {
            if (!this->Structure->quiet)
              fprintf(stderr,"Warning: setting constrained_parameters_flag = 0\n");
            this->Structure->constrparms = 0;
            break;
          }
        }
      }
    }
  }

  /* relational checks */

  if (this->Structure->mpeg1)
  {
    if (!this->Structure->prog_seq)
    {
      if (!this->Structure->quiet)
        fprintf(stderr,"Warning: setting progressive_sequence = 1\n");
      this->Structure->prog_seq = 1;
    }

    if (this->Structure->chroma_format!=CHROMA420)
    {
      if (!this->Structure->quiet)
        fprintf(stderr,"Warning: setting this->Structure->chroma_format = 1 (4:2:0)\n");
      this->Structure->chroma_format = CHROMA420;
    }

    if (this->Structure->dc_prec!=0)
    {
      if (!this->Structure->quiet)
        fprintf(stderr,"Warning: setting intra_dc_precision = 0\n");
      this->Structure->dc_prec = 0;
    }

    for (i=0; i<3; i++)
      if (this->Structure->qscale_tab[i])
      {
        if (!this->Structure->quiet)
          fprintf(stderr,"Warning: setting this->Structure->qscale_tab[%d] = 0\n",i);
        this->Structure->qscale_tab[i] = 0;
      }

    for (i=0; i<3; i++)
      if (this->Structure->intravlc_tab[i])
      {
        if (!this->Structure->quiet)
          fprintf(stderr,"Warning: setting this->Structure->intravlc_tab[%d] = 0\n",i);
        this->Structure->intravlc_tab[i] = 0;
      }

    for (i=0; i<3; i++)
      if (this->Structure->altscan_tab[i])
      {
        if (!this->Structure->quiet)
          fprintf(stderr,"Warning: setting this->Structure->altscan_tab[%d] = 0\n",i);
        this->Structure->altscan_tab[i] = 0;
      }
  }

  if (!this->Structure->mpeg1 && this->Structure->constrparms)
  {
    if (!this->Structure->quiet)
      fprintf(stderr,"Warning: setting constrained_parameters_flag = 0\n");
    this->Structure->constrparms = 0;
  }

  if (this->Structure->prog_seq && !this->Structure->prog_frame)
  {
    if (!this->Structure->quiet)
      fprintf(stderr,"Warning: setting progressive_frame = 1\n");
    this->Structure->prog_frame = 1;
  }

  if (this->Structure->prog_frame && this->Structure->fieldpic)
  {
    if (!this->Structure->quiet)
      fprintf(stderr,"Warning: setting field_pictures = 0\n");
    this->Structure->fieldpic = 0;
  }

  if (!this->Structure->prog_frame && this->Structure->repeatfirst)
  {
    if (!this->Structure->quiet)
      fprintf(stderr,"Warning: setting repeat_first_field = 0\n");
    this->Structure->repeatfirst = 0;
  }

  if (this->Structure->prog_frame)
  {
    for (i=0; i<3; i++)
      if (!this->Structure->frame_pred_dct_tab[i])
      {
        if (!this->Structure->quiet)
          fprintf(stderr,"Warning: setting frame_pred_frame_dct[%d] = 1\n",i);
        this->Structure->frame_pred_dct_tab[i] = 1;
      }
  }

  if (this->Structure->prog_seq && !this->Structure->repeatfirst && this->Structure->topfirst)
  {
    if (!this->Structure->quiet)
      fprintf(stderr,"Warning: setting top_field_first = 0\n");
    this->Structure->topfirst = 0;
  }

  /* search windows */
  for (i=0; i<this->Structure->M_val; i++)
  {
    if (this->Structure->motion_data[i].sxf > (4<<this->Structure->motion_data[i].forw_hor_f_code)-1)
    {
      if (!this->Structure->quiet)
        fprintf(stderr,
          "Warning: reducing forward horizontal search this->Structure->width to %d\n",
          (4<<this->Structure->motion_data[i].forw_hor_f_code)-1);
      this->Structure->motion_data[i].sxf = (4<<this->Structure->motion_data[i].forw_hor_f_code)-1;
    }

    if (this->Structure->motion_data[i].syf > (4<<this->Structure->motion_data[i].forw_vert_f_code)-1)
    {
      if (!this->Structure->quiet)
        fprintf(stderr,
          "Warning: reducing forward vertical search this->Structure->width to %d\n",
          (4<<this->Structure->motion_data[i].forw_vert_f_code)-1);
      this->Structure->motion_data[i].syf = (4<<this->Structure->motion_data[i].forw_vert_f_code)-1;
    }

    if (i!=0)
    {
      if (this->Structure->motion_data[i].sxb > (4<<this->Structure->motion_data[i].back_hor_f_code)-1)
      {
        if (!this->Structure->quiet)
          fprintf(stderr,
            "Warning: reducing backward horizontal search this->Structure->width to %d\n",
            (4<<this->Structure->motion_data[i].back_hor_f_code)-1);
        this->Structure->motion_data[i].sxb = (4<<this->Structure->motion_data[i].back_hor_f_code)-1;
      }

      if (this->Structure->motion_data[i].syb > (4<<this->Structure->motion_data[i].back_vert_f_code)-1)
      {
        if (!this->Structure->quiet)
          fprintf(stderr,
            "Warning: reducing backward vertical search this->Structure->width to %d\n",
            (4<<this->Structure->motion_data[i].back_vert_f_code)-1);
        this->Structure->motion_data[i].syb = (4<<this->Structure->motion_data[i].back_vert_f_code)-1;
      }
    }
  }

}

void vtkMPEG2WriterInternal::ReadQuantMat()
{
  int i,v;
  FILE *fd;

  if (this->Structure->iqname[0]=='-')
    {
    /* use default intra matrix */
    this->Structure->load_iquant = 0;
    for (i=0; i<64; i++)
      this->Structure->intra_q[i] = MPEG2_default_intra_quantizer_matrix[i];
    }
  else
    {
    /* read customized intra matrix */
    this->Structure->load_iquant = 1;
    if (!(fd = fopen(this->Structure->iqname,"r")))
      {
      sprintf(this->Structure->errortext,"Couldn't open quant matrix file %s",this->Structure->iqname);
      (*(this->Structure->report_error))(this->Structure->errortext);
      }

    for (i=0; i<64; i++)
      {
      fscanf(fd,"%d",&v);
      if (v<1 || v>255)
        (*(this->Structure->report_error))("invalid value in quant matrix");
      this->Structure->intra_q[i] = v;
      }

    fclose(fd);
    }

  if (this->Structure->niqname[0]=='-')
    {
    /* use default non-intra matrix */
    this->Structure->load_niquant = 0;
    for (i=0; i<64; i++)
      this->Structure->inter_q[i] = 16;
    }
  else
    {
    /* read customized non-intra matrix */
    this->Structure->load_niquant = 1;
    if (!(fd = fopen(this->Structure->niqname,"r")))
      {
      sprintf(this->Structure->errortext,"Couldn't open quant matrix file %s",this->Structure->niqname);
      (*(this->Structure->report_error))(this->Structure->errortext);
      }

    for (i=0; i<64; i++)
      {
      fscanf(fd,"%d",&v);
      if (v<1 || v>255)
        (*(this->Structure->report_error))("invalid value in quant matrix");
      this->Structure->inter_q[i] = v;
      }

    fclose(fd);
    }
}
