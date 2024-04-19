-- This file is hand generated.

-- Norway triangulated files
 
INSERT INTO other_transformation VALUES(
    'PROJ','NGO48_TO_ETRS89NO','NGO 1948 to ETRS89 (2)',
    'Transformation based on a triangulated irregular network',
    'PROJ','PROJString',
    '+proj=pipeline ' ||
        '+step +proj=axisswap +order=2,1 ' ||
        '+step +proj=tinshift +file=no_kv_ETRS89NO_NGO48_TIN.json +inv ' ||
        '+step +proj=axisswap +order=2,1',
    'EPSG','4273',
    'EPSG','4258',
    0.1,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);
INSERT INTO "usage" VALUES('PROJ','NGO48_TO_ETRS89NO_USAGE','other_transformation','PROJ','NGO48_TO_ETRS89NO','EPSG','1352','EPSG','1031');

-- Finland triangulated files

INSERT INTO other_transformation VALUES(
    'PROJ','YKJ_TO_ETRS35FIN','KKJ / Finland Uniform Coordinate System to ETRS35FIN',
    'Transformation based on a triangulated irregular network',
    'PROJ','PROJString','+proj=pipeline +step +proj=axisswap +order=2,1 +step +proj=tinshift +file=fi_nls_ykj_etrs35fin.json',
    'EPSG','2393','EPSG','3067',0.1,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0);
INSERT INTO "usage" VALUES('PROJ','YKJ_TO_ETRS35FIN_USAGE','other_transformation','PROJ','YKJ_TO_ETRS35FIN','EPSG','3333','EPSG','1024');

INSERT INTO "concatenated_operation" VALUES('PROJ','KKJ_TO_ETRS89','KKJ to ETRS89 (using PROJ:YKJ_TO_ETRS35FIN)','Transformation based on a triangulated irregular network','EPSG','4123','EPSG','4258',NULL,NULL,0);
INSERT INTO "concatenated_operation_step" VALUES('PROJ','KKJ_TO_ETRS89',1,'EPSG','18193');
INSERT INTO "concatenated_operation_step" VALUES('PROJ','KKJ_TO_ETRS89',2,'PROJ','YKJ_TO_ETRS35FIN');
INSERT INTO "concatenated_operation_step" VALUES('PROJ','KKJ_TO_ETRS89',3,'EPSG','16065');
INSERT INTO "usage" VALUES(
    'PROJ',
    'KKJ_TO_ETRS89_USAGE',
    'concatenated_operation',
    'PROJ',
    'KKJ_TO_ETRS89',
    'EPSG','3333', -- extent
    'EPSG','1024'  -- unknown
);

INSERT INTO other_transformation VALUES(
    'PROJ','N43_TO_N60','N43 height to N60 height',
    'Transformation based on a triangulated irregular network',
    'PROJ','PROJString','+proj=pipeline +step +proj=tmerc +lat_0=0 +lon_0=27 +k=1 +x_0=3500000 +y_0=0 +ellps=intl +step +proj=tinshift +file=fi_nls_n43_n60.json +step +inv +proj=tmerc +lat_0=0 +lon_0=27 +k=1 +x_0=3500000 +y_0=0 +ellps=intl',
    'EPSG','8675','EPSG','5717',0.01,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,'EPSG','4123',NULL,0);
INSERT INTO "usage" VALUES('PROJ','N43_TO_N60_USAGE','other_transformation','PROJ','N43_TO_N60','EPSG','4522','EPSG','1024');


INSERT INTO other_transformation VALUES(
    'PROJ','N60_TO_N2000','N60 height to N2000 height',
    'Transformation based on a triangulated irregular network',
    'PROJ','PROJString','+proj=pipeline +step +proj=tmerc +lat_0=0 +lon_0=27 +k=1 +x_0=3500000 +y_0=0 +ellps=intl +step +proj=tinshift +file=fi_nls_n60_n2000.json +step +inv +proj=tmerc +lat_0=0 +lon_0=27 +k=1 +x_0=3500000 +y_0=0 +ellps=intl',
    'EPSG','5717','EPSG','3900',0.01,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,'EPSG','4123',NULL,0);
INSERT INTO "usage" VALUES('PROJ','N60_TO_N2000_USAGE','other_transformation','PROJ','N60_TO_N2000','EPSG','3333','EPSG','1024');


-- Temporary entry for NZGD2000 deformation model
INSERT INTO other_transformation VALUES(
    'PROJ','NZGD2000-20180701','NZGD2000 to ITRF96',
    'New Zealand Deformation Model. Defines the secular model (National Deformation Model) and patches for significant deformation events since 2000',
    'PROJ', 'PROJString',
    '+proj=pipeline ' ||
        '+step +proj=unitconvert +xy_in=deg +xy_out=rad ' ||
        '+step +proj=axisswap +order=2,1 ' ||
        '+step +proj=defmodel +model=nz_linz_nzgd2000-20180701.json ' ||
        '+step +proj=axisswap +order=2,1 ' ||
        '+step +proj=unitconvert +xy_in=rad +xy_out=deg',
    'EPSG','4959',
    'EPSG','7907',
    NULL, --accuracy
    NULL,NULL,NULL,NULL,NULL,NULL, -- param1
    NULL,NULL,NULL,NULL,NULL,NULL, -- param2
    NULL,NULL,NULL,NULL,NULL,NULL, -- param3
    NULL,NULL,NULL,NULL,NULL,NULL, -- param4
    NULL,NULL,NULL,NULL,NULL,NULL, -- param5
    NULL,NULL,NULL,NULL,NULL,NULL, -- param6
    NULL,NULL,NULL,NULL,NULL,NULL, -- param7
    NULL,NULL,
    '20180701', -- operation version
    0);
INSERT INTO "usage" VALUES(
    'PROJ',
    'NZGD2000-20180701_USAGE',
    'other_transformation',
    'PROJ',
    'NZGD2000-20180701',
    'EPSG','1175', -- extent
    'EPSG','1024'  -- unknown
);


INSERT INTO "concatenated_operation" VALUES('PROJ','NZGD2000_TO_ITRF97','NZGD2000 to ITRF97','Concatenation of PROJ:NZGD2000-20180701 and 9079','EPSG','4959','EPSG','7908',NULL,NULL,0);
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF97',1,'PROJ','NZGD2000-20180701');
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF97',2,'EPSG','9079');
INSERT INTO "usage" VALUES(
    'PROJ',
    'NZGD2000_TO_ITRF97_USAGE',
    'concatenated_operation',
    'PROJ',
    'NZGD2000_TO_ITRF97',
    'EPSG','1175', -- extent
    'EPSG','1024'  -- unknown
);

INSERT INTO "concatenated_operation" VALUES('PROJ','NZGD2000_TO_ITRF2000','NZGD2000 to ITRF2000','Concatenation of PROJ:NZGD2000-20180701 and 9080','EPSG','4959','EPSG','7909',NULL,NULL,0);
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2000',1,'PROJ','NZGD2000-20180701');
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2000',2,'EPSG','9080');
INSERT INTO "usage" VALUES(
    'PROJ',
    'NZGD2000_TO_ITRF2000_USAGE',
    'concatenated_operation',
    'PROJ',
    'NZGD2000_TO_ITRF2000',
    'EPSG','1175', -- extent
    'EPSG','1024'  -- unknown
);

INSERT INTO "concatenated_operation" VALUES('PROJ','NZGD2000_TO_ITRF2005','NZGD2000 to ITRF2005','Concatenation of PROJ:NZGD2000-20180701 and 9081','EPSG','4959','EPSG','7910',NULL,NULL,0);
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2005',1,'PROJ','NZGD2000-20180701');
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2005',2,'EPSG','9081');
INSERT INTO "usage" VALUES(
    'PROJ',
    'NZGD2000_TO_ITRF2005_USAGE',
    'concatenated_operation',
    'PROJ',
    'NZGD2000_TO_ITRF2005',
    'EPSG','1175', -- extent
    'EPSG','1024'  -- unknown
);

INSERT INTO "concatenated_operation" VALUES('PROJ','NZGD2000_TO_ITRF2008','NZGD2000 to ITRF2008','Concatenation of PROJ:NZGD2000-20180701 and EPSG:9082','EPSG','4959','EPSG','7911',NULL,NULL,0);
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2008',1,'PROJ','NZGD2000-20180701');
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2008',2,'EPSG','9082');
INSERT INTO "usage" VALUES(
    'PROJ',
    'NZGD2000_TO_ITRF2008_USAGE',
    'concatenated_operation',
    'PROJ',
    'NZGD2000_TO_ITRF2008',
    'EPSG','1175', -- extent
    'EPSG','1024'  -- unknown
);

INSERT INTO "concatenated_operation" VALUES('PROJ','NZGD2000_TO_ITRF2014','NZGD2000 to ITRF2014','Concatenation of PROJ:NZGD2000-20180701 and EPSG:9083','EPSG','4959','EPSG','7912',NULL,NULL,0);
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2014',1,'PROJ','NZGD2000-20180701');
INSERT INTO "concatenated_operation_step" VALUES('PROJ','NZGD2000_TO_ITRF2014',2,'EPSG','9083');
INSERT INTO "usage" VALUES(
    'PROJ',
    'NZGD2000_TO_ITRF2014_USAGE',
    'concatenated_operation',
    'PROJ',
    'NZGD2000_TO_ITRF2014',
    'EPSG','1175', -- extent
    'EPSG','1024'  -- unknown
);
