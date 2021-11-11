INSERT INTO "metadata" VALUES('NKG.SOURCE', 'https://github.com/NordicGeodesy/NordicTransformations');
INSERT INTO "metadata" VALUES('NKG.VERSION', '1.0.0');
INSERT INTO "metadata" VALUES('NKG.DATE', '2020-12-21');

-- extent for NKG2008 transformations
INSERT INTO "extent" VALUES(
    'NKG','EXTENT_2008',            -- extend auth+code
    'Nordic and Baltic countries',  -- name
    'Denmark; Estonia; Finland; Latvia; Lithuania; Norway; Sweden', -- description
    53.0,                       -- south latitude
    73.0,                       -- north latitude
    3.0,                        -- west longitude
    40.0,                       -- east longitude
    0
);

-- extent for NKG2020 transformations
INSERT INTO "extent" VALUES(
    'NKG','EXTENT_2020',            -- extend auth+code
    'Nordic and Baltic countries',  -- name
    'Denmark; Estonia; Finland; Latvia; Lithuania; Norway; Sweden', -- description
    50.0,                       -- south latitude
    75.0,                       -- north latitude
    0.0,                        -- west longitude
    49.0,                       -- east longitude
    0
);

-- Scope for both NKG2008 and NKG2020 transformations
INSERT INTO "scope" VALUES (
    'NKG', 'SCOPE_GENERIC', -- scope auth+code
    'Geodesy. High accuracy ETRS89 transformations', -- scope
    0                       --deprecated
);


-------------------------------------------------------
--                DATUM+CRS: NKG_ETRF00
-------------------------------------------------------

INSERT INTO "geodetic_datum" VALUES (
    'NKG','DATUM_NKG_ETRF00', -- auth+code
    'NKG_ETRF00',   -- name
    NULL,           -- description
    'EPSG','7019',  -- ellipsoid auth+code
    'EPSG','8901',  -- prime meridian auth+code
    '2016-03-16',   -- publication date
    2000.0,         -- frame reference epoch
    NULL,           -- ensemble accuracy
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG','5007',
    'geodetic_datum',
    'NKG','DATUM_NKG_ETRF00',
    'NKG','EXTENT_2008',    -- extend auth+code
    'NKG','SCOPE_GENERIC'   -- scope auth+code
);

-- Add CRS entry for NKG common frame ETRF_NKG00
INSERT INTO "geodetic_crs" VALUES(
    'NKG','ETRF00', -- CRS auth+code
    'NKG_ETRF00',   -- name
    'NKG Common reference frame 2000', -- description
    'geocentric',   -- type
    'EPSG','6500',  -- CRS type auth+code: ECEF
    'NKG','DATUM_NKG_ETRF00', -- datum auth+code
    NULL,           -- text definition
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5101',          -- usage auth+code
    'geodetic_crs',         -- object_table_name
    'NKG', 'ETRF00',        -- object auth+code
    'NKG', 'EXTENT_2008',   -- extent auth+code
    'NKG', 'SCOPE_GENERIC'  -- scope auth+code
);

-------------------------------------------------------
--                DATUM+CRS: NKG_ETRF14
-------------------------------------------------------

INSERT INTO "geodetic_datum" VALUES (
    'NKG','DATUM_NKG_ETRF14', -- auth+code
    'NKG_ETRF14',   -- name
    NULL,           -- description
    'EPSG','7019',  -- ellipsoid auth+code
    'EPSG','8901',  -- prime meridian auth+code
    '2021-03-01',   -- publication date
    2000.0,         -- frame reference epoch
    NULL,           -- ensemble accuracy
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG','5033',
    'geodetic_datum',
    'NKG','DATUM_NKG_ETRF14',
    'NKG','EXTENT_2020', -- extend auth+code
    'NKG','SCOPE_GENERIC' -- scope auth+code
);

-- Add CRS entry for NKG common frame ETRF_NKG00
INSERT INTO "geodetic_crs" VALUES(
    'NKG','ETRF14', -- CRS auth+code
    'NKG_ETRF14',   -- name
    'NKG Common reference frame 2014', -- description
    'geocentric',   -- type
    'EPSG','6500',  -- CRS type auth+code: ECEF
    'NKG','DATUM_NKG_ETRF14', -- datum auth+code
    NULL,           -- text definition
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5102',          -- usage auth+code
    'geodetic_crs',         -- object_table_name
    'NKG', 'ETRF14',        -- object auth+code
    'NKG', 'EXTENT_2020',   -- extent auth+code
    'NKG', 'SCOPE_GENERIC'  -- scope auth+code
);

-------------------------------------------------------
--     Transformation: ITRF2000 -> NKG_ETRF00
-------------------------------------------------------

INSERT INTO "concatenated_operation" VALUES (
    'NKG', 'ITRF2000_TO_NKG_ETRF00', -- operation auth+code
    'ITRF2000 to NKG_ETRF00', -- name
    'Time-dependent transformation from ITRF2000 to NKG_ETRF00', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'NKG', 'ETRF00',-- target_crs:  NKG_ETRF00
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG','NKG_ETRF00_TO_ETRF2000', -- operation auth+code
    'NKG_ETRF00 to ETRF2000',       -- name
    NULL,                           -- description
    'PROJ', 'PROJString',           -- method auth+code
    '+proj=deformation +t_epoch=2000.0 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','7930',  -- target_crs:  ETRF2000
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5003',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG','NKG_ETRF00_TO_ETRF2000', -- object auth+code
    'NKG','EXTENT_2008',    -- extent auth+code
    'NKG','SCOPE_GENERIC'   -- scope auth+code
);



INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_NKG_ETRF00', 2, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_NKG_ETRF00', 3, 'NKG', 'NKG_ETRF00_TO_ETRF2000')
;


INSERT INTO "usage" VALUES (
    'NKG', '5001',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_NKG_ETRF00', -- object auth+code
    'NKG', 'EXTENT_2008',       -- extent auth+code
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);


-------------------------------------------------------
--     Transformation: ITRF2014 -> NKG_ETRF14
-------------------------------------------------------

INSERT INTO "concatenated_operation" VALUES (
    'NKG', 'ITRF2014_TO_NKG_ETRF14', -- operation auth+code
    'ITRF2014 to NKG_ETRF14', -- name
    'Time-dependent transformation from ITRF2014 to NKG_ETRF14', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'NKG', 'ETRF14',-- target_crs:  NKG_ETRF14
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG','NKG_ETRF14_TO_ETRF2014', -- operation auth+code
    'NKG_ETRF14 to ETRF2014',       -- name
    NULL,                           -- description
    'PROJ', 'PROJString',           -- method auth+code
    '+proj=deformation +t_epoch=2000.0 +grids=eur_nkg_nkgrf17vel.tif',
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF14
    'EPSG','8401',  -- target_crs:  ETRF2014
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5034',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG','NKG_ETRF14_TO_ETRF2014', -- object auth+code
    'NKG','EXTENT_2020',    -- extent auth+code
    'NKG','SCOPE_GENERIC'   -- scope auth+code
);



INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_NKG_ETRF14', 2, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_NKG_ETRF14', 3, 'NKG', 'NKG_ETRF14_TO_ETRF2014')
;


INSERT INTO "usage" VALUES (
    'NKG', '5035',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_NKG_ETRF14', -- object auth+code
    'NKG', 'EXTENT_2020',       -- extent auth+code
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);




-------------------------------------------------------------
-- Intermediate transformations: NKG_ETRF00 -> ETRFyy@2000.00
-------------------------------------------------------------

-- DK
INSERT INTO "helmert_transformation" VALUES (
    'NKG','P1_2008_DK', -- operation auth+code
    'NKG_ETRF00 to ETRF92@2000.0', -- name
    'Transformation from NKG_ETRF00 to ETRF92, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF00', -- source auth+code
    'EPSG','7920',  -- target auth+code
    0.005,          -- accuracy
    0.03863,        -- x
    0.147,          -- y
    0.02776,        -- z
    'EPSG','9001',
    0.00617753,     -- rx
    5.064e-05,      -- ry
    4.729e-05,      -- rz
    'EPSG','9104',
    -0.009420,      -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2008',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5004',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','P1_2008_DK',         -- object auth+code
    'EPSG', '1080',             -- extent: Denmark - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- EE
INSERT INTO "helmert_transformation" VALUES (
    'NKG','P1_2008_EE', -- operation auth+code
    'NKG_ETRF00 to ETRF96@2000.0', -- name
    'Transformation from NKG_ETRF00 to ETRF96, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF00', -- source auth+code
    'EPSG','7926',  -- target auth+code
    0.005,           -- accuracy
    0.12194,        -- x
    0.02225,        -- y
    -0.03541,       -- z
    'EPSG','9001',
    0.00227196,     -- rx
    -0.00323934,    -- ry
    0.00247008,     -- rz
    'EPSG','9104',
    -0.005626,      -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2008',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5008',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','P1_2008_EE',         -- object auth+code
    'EPSG', '1090',             -- extent: Estonia - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- FI
INSERT INTO "helmert_transformation" VALUES (
    'NKG','P1_2008_FI', -- operation auth+code
    'NKG_ETRF00 to ETRF96@2000.0', -- name
    'Transformation from NKG_ETRF00 to ETRF96, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF00', -- source auth+code
    'EPSG','7926',  -- target auth+code
    0.005,          -- accuracy
    0.07251,        -- x
    -0.13019,       -- y
    -0.11323,       -- z
    'EPSG','9001',
    -0.00157399,    -- rx
    -0.00308833,    -- ry
    0.00410332,     -- rz
    'EPSG','9104',
    0.013012,       -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2008',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5009',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','P1_2008_FI',         -- object auth+code
    'EPSG', '1095',             -- extent: Finland - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- LV
INSERT INTO "helmert_transformation" VALUES (
    'NKG','P1_2008_LV', -- operation auth+code
    'NKG_ETRF00 to ETRF89@2000.0', -- name
    'Transformation from NKG_ETRF00 to ETRF89, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF00', -- source auth+code
    'EPSG','7914',  -- target auth+code
    0.02,           -- accuracy
    0.41812,        -- x
    -0.78105,       -- y
    -0.01335,       -- z
    'EPSG','9001',
    -0.0216436,     -- rx
    -0.0115184,     -- ry
    0.01719911,     -- rz
    'EPSG','9104',
    0.000757,       -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2008',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5010',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','P1_2008_LV',         -- object auth+code
    'EPSG', '1139',             -- extent: Latvia - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);

-- LT
INSERT INTO "helmert_transformation" VALUES (
    'NKG','P1_2008_LT', -- operation auth+code
    'NKG_ETRF00 to ETRF2000@2000.0', -- name
    'Transformation from NKG_ETRF00 to ETRF2000, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF00', -- source auth+code
    'EPSG','7930',  -- target auth+code
    0.01,           -- accuracy
    0.05692,        -- x
    0.115495,       -- y
    -0.00078,       -- z
    'EPSG','9001',
    0.00314291,     -- rx
    -0.00147975,    -- ry
    -0.00134758,    -- rz
    'EPSG','9104',
    -0.006182,      -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2008',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5011',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','P1_2008_LT',         -- object auth+code
    'EPSG', '1145',             -- extent: Lithuania - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- NO
INSERT INTO "helmert_transformation" VALUES (
    'NKG','P1_2008_NO', -- operation auth+code
    'NKG_ETRF00 to ETRF93@2000.0', -- name
    'Transformation from NKG_ETRF00 to ETRF93, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF00', -- source auth+code
    'EPSG','7922',  -- target auth+code
    0.005,          -- accuracy
    -0.13116,       -- x
    -0.02817,       -- y
    0.02036,        -- z
    'EPSG','9001',
    -0.00038674,    -- rx
    0.00408947,     -- ry
    0.00103588,     -- rz
    'EPSG','9104',
    0.006569,       -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2008',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5012',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','P1_2008_NO',         -- object auth+code
    'EPSG', '1352',             -- extent: Norway - onshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- SE
INSERT INTO "helmert_transformation" VALUES (
    'NKG','P1_2008_SE', -- operation auth+code
    'NKG_ETRF00 to ETRF97@2000.0', -- name
    'Transformation from NKG_ETRF00 to ETRF97, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF00', -- source auth+code
    'EPSG','7928',  -- target auth+code
    0.005,          -- accuracy
    -0.01642,       -- x
    -0.00064,       -- y
    -0.0305,        -- z
    'EPSG','9001',
    0.00187431,     -- rx
    0.00046382,     -- ry
    0.00228487,     -- rz
    'EPSG','9104',
    0.001861,       -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2008',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5014',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','P1_2008_SE',         -- object auth+code
    'EPSG', '1225',             -- extent: Sweden - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-------------------------------------------------------------
-- Intermediate transformations: NKG_ETRF14 -> ETRFyy@2000.00
-------------------------------------------------------------

-- DK
INSERT INTO "helmert_transformation" VALUES (
    'NKG','PAR_2020_DK', -- operation auth+code
    'NKG_ETRF14 to ETRF92@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF92, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF14', -- source auth+code
    'EPSG','7920',  -- target auth+code
    0.005,          -- accuracy
    0.66818,        -- x
    0.04453,        -- y
    -0.45049,       -- z
    'EPSG','9001',
    0.00312883,     -- rx
    -0.02373423,    -- ry
    0.00442969,     -- rz
    'EPSG','9104',
    -0.003136,      -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2020',     -- operation version
    0
);


INSERT INTO "usage" VALUES (
    'NKG', '5036',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','PAR_2020_DK',        -- object auth+code
    'EPSG', '1080',             -- extent: Denmark - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- EE
INSERT INTO "helmert_transformation" VALUES (
    'NKG','PAR_2020_EE', -- operation auth+code
    'NKG_ETRF14 to ETRF96@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF96, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF14', -- source auth+code
    'EPSG','7926',  -- target auth+code
    0.005,          -- accuracy
    -0.05027,       -- x
    -0.11595,       -- y
    0.03012,        -- z
    'EPSG','9001',
    -0.00310814,    -- rx
    0.00457237,     -- ry
    0.00472406,     -- rz
    'EPSG','9104',
    0.003191,       -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2020',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5037',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','PAR_2020_EE',         -- object auth+code
    'EPSG', '1090',             -- extent: Estonia - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- FI
INSERT INTO "helmert_transformation" VALUES (
    'NKG','PAR_2020_FI', -- operation auth+code
    'NKG_ETRF14 to ETRF96@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF96, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF14', -- source auth+code
    'EPSG','7926',  -- target auth+code
    0.005,          -- accuracy
    0.15651,        -- x
    -0.10993,       -- y
    -0.10935,       -- z
    'EPSG','9001',
    -0.00312861,    -- rx
    -0.00378935,    -- ry
    0.00403512,     -- rz
    'EPSG','9104',
    0.00529,        -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2020',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5038',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','PAR_2020_FI',        -- object auth+code
    'EPSG', '1095',             -- extent: Finland - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);


-- LV
INSERT INTO "helmert_transformation" VALUES (
    'NKG','PAR_2020_LV', -- operation auth+code
    'NKG_ETRF14 to ETRF89@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF89, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF14', -- source auth+code
    'EPSG','7914',  -- target auth+code
    0.01,           -- accuracy
    0.09745,        -- x
    -0.69388,       -- y
    0.52901,        -- z
    'EPSG','9001',
    -0.0192069,     -- rx
    0.01043272,     -- ry
    0.02327169,     -- rz
    'EPSG','9104',
    -0.049663,      -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2020',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5039',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','PAR_2020_LV',        -- object auth+code
    'EPSG', '1139',             -- extent: Latvia - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);

-- LT
INSERT INTO "helmert_transformation" VALUES (
    'NKG','PAR_2020_LT', -- operation auth+code
    'NKG_ETRF14 to ETRF2000@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF2000, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF14', -- source auth+code
    'EPSG','7930',  -- target auth+code
    0.015,          -- accuracy
    0.36749,        -- x
    0.14351,        -- y
    -0.18472,       -- z
    'EPSG','9001',
    0.0047914,      -- rx
    -0.01027566,    -- ry
    0.00276102,     -- rz
    'EPSG','9104',
    -0.003684,      -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2020',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5040',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','PAR_2020_LT',         -- object auth+code
    'EPSG', '1145',             -- extent: Lithuania - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);

-- NO
INSERT INTO "helmert_transformation" VALUES (
    'NKG','PAR_2020_NO', -- operation auth+code
    'NKG_ETRF14 to ETRF93@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF93, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF14', -- source auth+code
    'EPSG','7922',  -- target auth+code
    0.01,           -- accuracy
    -0.05172,       -- x
    0.13747,        -- y
    -0.01648,       -- z
    'EPSG','9001',
    0.00268452,     -- rx
    0.00329165,     -- ry
    -0.00116569,    -- rz
    'EPSG','9104',
    0.002583,       -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2020',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5041',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','PAR_2020_NO',        -- object auth+code
    'EPSG', '1352',             -- extent: Norway - onshore
    'NKG', 'SCOPE_GENERIC'      -- scope
);

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'NKG_ETRF14_ETRF93_2000', -- object auth+code
    'NKG_ETRF14 to ETRF93@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF93, at transformation reference epoch 2000.0', -- description / remark
    'PROJ', 'PROJString', 
    '+proj=xyzgridshift +grids=no_kv_NKGETRF14_EPSG7922_2000.tif',
    'NKG','ETRF14',  -- source auth+code
    'EPSG','7922',   -- target auth+code
    0.005,           -- accuracy
    'NKG 2020',      -- operation_version
    0                -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5064',
    'other_transformation',
    'NKG', 'NKG_ETRF14_ETRF93_2000',
    'EPSG', '1352',
    'NKG', 'SCOPE_GENERIC'
);

-- SE
INSERT INTO "helmert_transformation" VALUES (
    'NKG','PAR_2020_SE', -- operation auth+code
    'NKG_ETRF14 to ETRF97@2000.0', -- name
    'Transformation from NKG_ETRF14 to ETRF97, at transformation reference epoch 2000.0', -- description / remark
    'EPSG','1033',  -- method auth+code
    'Position Vector transformation (geocentric domain)',
    'NKG','ETRF14', -- source auth+code
    'EPSG','7928',  -- target auth+code
    0.005,          -- accuracy
    0.03054,        -- x
    0.04606,        -- y
   -0.07944,        -- z
    'EPSG','9001',
    0.00141958,     -- rx
    0.00015132,     -- ry
    0.00150337,     -- rz
    'EPSG','9104',
    0.003002,       -- s
    'EPSG','9202',
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    'NKG 2020',     -- operation version
    0
);

INSERT INTO "usage" VALUES (
    'NKG', '5042',              -- usage auth+code
    'helmert_transformation',   -- object_table_name
    'NKG','PAR_2020_SE',        -- object auth+code
    'EPSG', '1225',             -- extent: Sweden - onshore and offshore
    'NKG',  'SCOPE_GENERIC'     -- scope
);




-------------------------------------------------------
-- Transformation: NKG_ETRF00 -> ETRF92@1994.704 (DK)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'ETRF92_2000_TO_ETRF92_1994',-- object auth+code
    'ETRF92@2000.0 to ETRF92@1994.704', -- name
    NULL, -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-5.296 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'EPSG','7920',  -- source_crs:  ETRF92@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (DK)
    0.005,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5005',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'ETRF92_2000_TO_ETRF92_1994', -- object auth+code
    'EPSG', '1080',         -- extent: Denmark - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF00_TO_DK', -- operation auth+code
    'NKG_ETRF00 to ETRS89(DK)', -- name
    'Transformation from NKG_ETRF00@2000.0 to ETRF92@1994.704', -- description
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (DK)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF00_TO_DK', 1, 'NKG', 'P1_2008_DK'),
    ('NKG', 'ETRF00_TO_DK', 2, 'NKG', 'ETRF92_2000_TO_ETRF92_1994')
;


INSERT INTO "usage" VALUES (
    'NKG', '5006',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF00_TO_DK',      -- object auth+code
    'EPSG', '1080',             -- extent: Denmark - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2000 -> ETRF92@1994.704 (DK)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2000_TO_DK',  -- operation auth+code
    'ITRF2000 to ETRS89(DK)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89(DK)', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'EPSG', '4936', -- target_crs:  ETRS89(DK)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_DK', 1, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_DK', 2, 'NKG', 'NKG_ETRF00_TO_ETRF2000'),
    ('NKG', 'ITRF2000_TO_DK', 3, 'NKG', 'P1_2008_DK'),
    ('NKG', 'ITRF2000_TO_DK', 4, 'NKG', 'ETRF92_2000_TO_ETRF92_1994')
;


INSERT INTO "usage" VALUES (
    'NKG', '5013',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_DK',    -- object auth+code
    'EPSG', '1080',             -- extent: Denmark - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);

-------------------------------------------------------
-- Transformation: NKG_ETRF00 -> ETRF96@1997.56 (EE)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'ETRF96_2000_TO_ETRF96_1997_56',-- object auth+code
    'ETRF96@2000.0 to ETRF96@1997.56', -- name
    NULL, -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-2.44 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'EPSG','7926',  -- source_crs:  ETRF96@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (EE)
    0.005,          -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5015',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'ETRF96_2000_TO_ETRF96_1997_56', -- object auth+code
    'EPSG', '1090',         -- extent: Estonia - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF00_TO_EE', -- operation auth+code
    'NKG_ETRF00 to ETRS89 (EUREF-EST97)', -- name
    'Transformation from NKG_ETRF00@2000.0 to ETRF96@1997.56', -- description
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (EE)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF00_TO_EE', 1, 'NKG', 'P1_2008_EE'),
    ('NKG', 'ETRF00_TO_EE', 2, 'NKG', 'ETRF96_2000_TO_ETRF96_1997_56')
;


INSERT INTO "usage" VALUES (
    'NKG', '5016',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF00_TO_EE',      -- object auth+code
    'EPSG', '1090',             -- extent: Estonia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2000 -> ETRF96@1997.56 (EE)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2000_TO_EE',  -- operation auth+code
    'ITRF2000 to ETRS89(EE)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89 (EUREF-EST97)', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'EPSG', '4936', -- target_crs:  ETRS89(EE)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_EE', 1, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_EE', 2, 'NKG', 'NKG_ETRF00_TO_ETRF2000'),
    ('NKG', 'ITRF2000_TO_EE', 3, 'NKG', 'P1_2008_EE'),
    ('NKG', 'ITRF2000_TO_EE', 4, 'NKG', 'ETRF96_2000_TO_ETRF96_1997_56')
;


INSERT INTO "usage" VALUES (
    'NKG', '5017',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_EE',    -- object auth+code
    'EPSG', '1090',             -- extent: Estonia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);




-------------------------------------------------------
-- Transformation: NKG_ETRF00 -> ETRF96@1997.0 (FI)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'ETRF96_2000_TO_ETRF96_1997',-- object auth+code
    'ETRF96@2000.0 to ETRF96@1997.0',     -- name
    NULL,                               -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-3.0 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'EPSG','7926',  -- source_crs:  ETRF96@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (FI)
    0.005,          -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5018',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'ETRF96_2000_TO_ETRF96_1997', -- object auth+code
    'EPSG', '1095',         -- extent: Finland - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF00_TO_FI', -- operation auth+code
    'NKG_ETRF00 to ETRS89 (EUREF-FIN)', -- name
    'Transformation from NKG_ETRF00@2000.0 to ETRF96@1997.0', -- description
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (FI)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF00_TO_FI', 1, 'NKG', 'P1_2008_FI'),
    ('NKG', 'ETRF00_TO_FI', 2, 'NKG', 'ETRF96_2000_TO_ETRF96_1997')
;


INSERT INTO "usage" VALUES (
    'NKG', '5019',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF00_TO_FI',      -- object auth+code
    'EPSG', '1095',             -- extent: Finland - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2000 -> ETRF96@1997.0 (FI)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2000_TO_FI',  -- operation auth+code
    'ITRF2000 to ETRS89 (EUREF-FIN)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89 (EUREF-FIN)', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'EPSG', '4936', -- target_crs:  ETRS89(FI)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_FI', 1, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_FI', 2, 'NKG', 'NKG_ETRF00_TO_ETRF2000'),
    ('NKG', 'ITRF2000_TO_FI', 3, 'NKG', 'P1_2008_FI'),
    ('NKG', 'ITRF2000_TO_FI', 4, 'NKG', 'ETRF96_2000_TO_ETRF96_1997')
;


INSERT INTO "usage" VALUES (
    'NKG', '5020',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_FI',    -- object auth+code
    'EPSG', '1095',             -- extent: Finland - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);




-------------------------------------------------------
-- Transformation: NKG_ETRF00 -> ETRF89@1992.75 (LV)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'ETRF89_2000_TO_ETRF89_1992',-- object auth+code
    'ETRF89@2000.0 to ETRF89@1992.75',  -- name
    NULL,                               -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-7.25 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'EPSG','7914',  -- source_crs:  ETRF89@2000.0
    'EPSG','4948',  -- target_crs:  LKS-92
    0.005,          -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5021',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'ETRF89_2000_TO_ETRF89_1992', -- object auth+code
    'EPSG', '1139',         -- extent: Latvia - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF00_TO_LV', -- operation auth+code
    'NKG_ETRF00 to ETRS89 (LKS-92)', -- name
    'Transformation from NKG_ETRF00@2000.0 to ETRF89@1992.75', -- description
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','4948',  -- target_crs:  LKS-92
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF00_TO_LV', 1, 'NKG', 'P1_2008_LV'),
    ('NKG', 'ETRF00_TO_LV', 2, 'NKG', 'ETRF89_2000_TO_ETRF89_1992')
;


INSERT INTO "usage" VALUES (
    'NKG', '5022',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF00_TO_LV',      -- object auth+code
    'EPSG', '1139',             -- extent: Latvia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2000 -> ETRF89@1992.75 (LV)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2000_TO_LV',  -- operation auth+code
    'ITRF2000 to ETRS89 (LKS-92)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89 (LKS-92)', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'EPSG', '4948', -- target_crs:  LKS-92
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_LV', 1, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_LV', 2, 'NKG', 'NKG_ETRF00_TO_ETRF2000'),
    ('NKG', 'ITRF2000_TO_LV', 3, 'NKG', 'P1_2008_LV'),
    ('NKG', 'ITRF2000_TO_LV', 4, 'NKG', 'ETRF89_2000_TO_ETRF89_1992')
;


INSERT INTO "usage" VALUES (
    'NKG', '5023',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_LV',    -- object auth+code
    'EPSG', '1139',             -- extent: Latvia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: NKG_ETRF00 -> ETRF2000@2003.75 (LT)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'ETRF2000_2000_TO_ETRF_2000_2003',-- object auth+code
    'ETRF2000@2000.0 to ETRF2000@2003.75',  -- name
    NULL,                                   -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=3.75 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'EPSG','7930',  -- source_crs:  ETRF2000@2000.0
    'EPSG','4950',  -- target_crs:  LKS94
    0.005,          -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5024',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'ETRF2000_2000_TO_ETRF_2000_2003', -- object auth+code
    'EPSG', '1145',         -- extent: Lithuania - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF00_TO_LT', -- operation auth+code
    'NKG_ETRF00 to LKS94', -- name
    'Transformation from NKG_ETRF00@2000.0 to ETRF2000@2003.75', -- description
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','4950',  -- target_crs:  LKS94
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF00_TO_LT', 1, 'NKG', 'P1_2008_LT'),
    ('NKG', 'ETRF00_TO_LT', 2, 'NKG', 'ETRF2000_2000_TO_ETRF_2000_2003')
;


INSERT INTO "usage" VALUES (
    'NKG', '5025',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF00_TO_LT',      -- object auth+code
    'EPSG', '1145',             -- extent: Lithuania - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2000 -> ETRF2000@2003.75 (LT)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2000_TO_LT',  -- operation auth+code
    'ITRF2000 to ETRS89(LT)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89(LT)', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'EPSG', '4950', -- target_crs:  LKS94
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_LT', 1, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_LT', 2, 'NKG', 'NKG_ETRF00_TO_ETRF2000'),
    ('NKG', 'ITRF2000_TO_LT', 3, 'NKG', 'P1_2008_LT'),
    ('NKG', 'ITRF2000_TO_LT', 4, 'NKG', 'ETRF2000_2000_TO_ETRF_2000_2003')
;


INSERT INTO "usage" VALUES (
    'NKG', '5026',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_LT',    -- object auth+code
    'EPSG', '1145',             -- extent: Lithuania - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: NKG_ETRF00 -> ETRF93@1995.0 (NO)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'ETRF93_2000_TO_ETRF93_1995',-- object auth+code
    'ETRF93@2000.0 to ETRF93@1995.0',   -- name
    NULL,                               -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-5 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'EPSG','7922',  -- source_crs:  ETRF93@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (NO)
    0.005,          -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5027',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'ETRF93_2000_TO_ETRF93_1995', -- object auth+code
    'EPSG', '1352',         -- extent: Norway - onshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF00_TO_NO', -- operation auth+code
    'NKG_ETRF00 to ETRS89(NO)', -- name
    'Transformation from NKG_ETRF00@2000.0 to ETRF93@1995.0', -- description
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (NO)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF00_TO_NO', 1, 'NKG', 'P1_2008_NO'),
    ('NKG', 'ETRF00_TO_NO', 2, 'NKG', 'ETRF93_2000_TO_ETRF93_1995')
;


INSERT INTO "usage" VALUES (
    'NKG', '5028',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF00_TO_NO',      -- object auth+code
    'EPSG', '1352',             -- extent: Norway - onshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2000 -> ETRF93@1995.0 (NO)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2000_TO_NO',  -- operation auth+code
    'ITRF2000 to ETRS89(NO)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89(NO)', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'EPSG', '4936', -- target_crs:  ETRS89(NO)
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_NO', 1, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_NO', 2, 'NKG', 'NKG_ETRF00_TO_ETRF2000'),
    ('NKG', 'ITRF2000_TO_NO', 3, 'NKG', 'P1_2008_NO'),
    ('NKG', 'ITRF2000_TO_NO', 4, 'NKG', 'ETRF93_2000_TO_ETRF93_1995')
;


INSERT INTO "usage" VALUES (
    'NKG', '5029',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_NO',    -- object auth+code
    'EPSG', '1352',             -- extent: Norway - onshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: NKG_ETRF00 -> ETRF97@1999.5 (SE)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'ETRF97_2000_TO_ETRF97_1999',-- object auth+code
    'ETRF97@2000.0 to ETRF97@1999.5',   -- name
    NULL,                               -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-0.5 +grids=eur_nkg_nkgrf03vel_realigned.tif',
    'EPSG','7928',  -- source_crs:  ETRF97@2000.0
    'EPSG','4976',  -- target_crs:  SWEREF99
    0.005,          -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5030',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'ETRF97_2000_TO_ETRF97_1999', -- object auth+code
    'EPSG', '1225',         -- extent: Sweden - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF00_TO_SE', -- operation auth+code
    'NKG_ETRF00 to SWEREF99', -- name
    'Transformation from NKG_ETRF00@2000.0 to ETRF97@1999.5', -- description
    'NKG', 'ETRF00',-- source_crs:  NKG_ETRF00
    'EPSG','4976',  -- target_crs:  SWEREF99
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF00_TO_SE', 1, 'NKG', 'P1_2008_SE'),
    ('NKG', 'ETRF00_TO_SE', 2, 'NKG', 'ETRF97_2000_TO_ETRF97_1999')
;


INSERT INTO "usage" VALUES (
    'NKG', '5031',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF00_TO_SE',      -- object auth+code
    'EPSG', '1225',             -- extent: Sweden - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2000 -> ETRF97@1999.5 (SE)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2000_TO_SE',  -- operation auth+code
    'ITRF2000 to ETRS89(SE)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89(SE)', -- description
    'EPSG', '4919', -- source_crs:  ITRF2000
    'EPSG', '4976', -- target_crs:  SWEREF99
    0.01,           -- accuracy
    'NKG 2008',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2000_TO_SE', 1, 'EPSG', '7941'), -- ITRF2000 -> ETRF2000
    ('NKG', 'ITRF2000_TO_SE', 2, 'NKG', 'NKG_ETRF00_TO_ETRF2000'),
    ('NKG', 'ITRF2000_TO_SE', 3, 'NKG', 'P1_2008_SE'),
    ('NKG', 'ITRF2000_TO_SE', 4, 'NKG', 'ETRF97_2000_TO_ETRF97_1999')
;


INSERT INTO "usage" VALUES (
    'NKG', '5032',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2000_TO_SE',    -- object auth+code
    'EPSG', '1225',             -- extent: Sweden - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);

-------------------------------------------------------
-- Transformation: NKG_ETRF14 -> ETRF92@1994.704 (DK)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'DK_2020_INTRAPLATE',        -- object auth+code
    'ETRF92@2000.0 to ETRF92@1994.704', -- name
    NULL,                               -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=15.829 +grids=eur_nkg_nkgrf17vel.tif',
    'EPSG','7920',  -- source_crs:  ETRF92@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (DK)
    0.005,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5043',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'DK_2020_INTRAPLATE', -- object auth+code
    'EPSG', '1080',         -- extent: Denmark - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF14_TO_DK', -- operation auth+code
    'NKG_ETRF14 to ETRS89(DK)', -- name
    'Transformation from NKG_ETRF14@2000.0 to ETRF92@1994.704', -- description
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (DK)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF14_TO_DK', 1, 'NKG', 'PAR_2020_DK'),
    ('NKG', 'ETRF14_TO_DK', 2, 'NKG', 'DK_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5044',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF14_TO_DK',      -- object auth+code
    'EPSG', '1080',             -- extent: Denmark - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2014 -> ETRF92@1994.704 (DK)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2014_TO_DK',  -- operation auth+code
    'ITRF2014 to ETRS89(DK)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89(DK)', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'EPSG', '4936', -- target_crs:  ETRS89(DK)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_DK', 1, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_DK', 2, 'NKG', 'NKG_ETRF14_TO_ETRF2014'),
    ('NKG', 'ITRF2014_TO_DK', 3, 'NKG', 'PAR_2020_DK'),
    ('NKG', 'ITRF2014_TO_DK', 4, 'NKG', 'DK_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5045',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_DK',    -- object auth+code
    'EPSG', '1080',             -- extent: Denmark - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);

INSERT INTO "supersession" VALUES (
    'concatenated_operation',
    'NKG', 'ITRF2000_TO_DK',
    'concatenated_operation',
    'NKG', 'ITRF2014_TO_DK',
    'NKG',
    0
);

-------------------------------------------------------
-- Transformation: NKG_ETRF14 -> ETRF96@1997.56 (EE)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'EE_2020_INTRAPLATE',-- object auth+code
    'ETRF96@2000.0 to ETRF96@1997.56', -- name
    NULL, -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-2.44 +grids=eur_nkg_nkgrf17vel.tif',
    'EPSG','7926',  -- source_crs:  ETRF96@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (EE)
    0.005,          -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5046',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'EE_2020_INTRAPLATE', -- object auth+code
    'EPSG', '1090',         -- extent: Estonia - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF14_TO_EE', -- operation auth+code
    'NKG_ETRF14 to ETRS89 (EUREF-EST97)', -- name
    'Transformation from NKG_ETRF14@2000.0 to ETRF96@1997.56', -- description
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (EE)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF14_TO_EE', 1, 'NKG', 'PAR_2020_EE'),
    ('NKG', 'ETRF14_TO_EE', 2, 'NKG', 'EE_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5047',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF14_TO_EE',      -- object auth+code
    'EPSG', '1090',             -- extent: Estonia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2014 -> ETRF96@1997.56 (EE)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2014_TO_EE',  -- operation auth+code
    'ITRF2014 to ETRS89 (EUREF-EST97)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89 (EUREF-EST97)', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'EPSG', '4936', -- target_crs:  ETRS89(EE)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_EE', 1, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_EE', 2, 'NKG', 'NKG_ETRF14_TO_ETRF2014'),
    ('NKG', 'ITRF2014_TO_EE', 3, 'NKG', 'PAR_2020_EE'),
    ('NKG', 'ITRF2014_TO_EE', 4, 'NKG', 'EE_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5048',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_EE',    -- object auth+code
    'EPSG', '1090',             -- extent: Estonia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);


INSERT INTO "supersession" VALUES (
    'concatenated_operation',
    'NKG', 'ITRF2000_TO_EE',
    'concatenated_operation',
    'NKG', 'ITRF2014_TO_EE',
    'NKG',
    0
);


-------------------------------------------------------
-- Transformation: NKG_ETRF14 -> ETRF96@1997.0 (FI)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'FI_2020_INTRAPLATE',-- object auth+code
    'ETRF96@2000.0 to ETRF96@1997.0', -- name
    NULL, -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-3 +grids=eur_nkg_nkgrf17vel.tif',
    'EPSG','7926',  -- source_crs:  ETRF96@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (FI)
    0.005,          -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5049',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'FI_2020_INTRAPLATE', -- object auth+code
    'EPSG', '1095',         -- extent: Finland - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF14_TO_FI', -- operation auth+code
    'NKG_ETRF14 to ETRS89 (EUREF-FIN)', -- name
    'Transformation from NKG_ETRF14@2000.0 to ETRF96@1997.0', -- description
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (FI)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF14_TO_FI', 1, 'NKG', 'PAR_2020_FI'),
    ('NKG', 'ETRF14_TO_FI', 2, 'NKG', 'FI_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5050',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF14_TO_FI',      -- object auth+code
    'EPSG', '1095',             -- extent: Finland - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2014 -> ETRF96@1997.0 (FI)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2014_TO_FI',  -- operation auth+code
    'ITRF2014 to ETRS89 (EUREF-FIN)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89 (EUREF-FIN)', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'EPSG', '4936', -- target_crs:  ETRS89(FI)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_FI', 1, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_FI', 2, 'NKG', 'NKG_ETRF14_TO_ETRF2014'),
    ('NKG', 'ITRF2014_TO_FI', 3, 'NKG', 'PAR_2020_FI'),
    ('NKG', 'ITRF2014_TO_FI', 4, 'NKG', 'FI_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5051',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_FI',    -- object auth+code
    'EPSG', '1095',             -- extent: Finland - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);


INSERT INTO "supersession" VALUES (
    'concatenated_operation',
    'NKG', 'ITRF2000_TO_FI',
    'concatenated_operation',
    'NKG', 'ITRF2014_TO_FI',
    'NKG',
    0
);


-------------------------------------------------------
-- Transformation: NKG_ETRF14 -> ETRF89@1992.75 (LV)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'LV_2020_INTRAPLATE',        -- object auth+code
    'ETRF89@2000.0 to ETRF89@1992.75 (LKS-92)',  -- name
    NULL,                               -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-7.25 +grids=eur_nkg_nkgrf17vel.tif',
    'EPSG','7914',  -- source_crs:  ETRF89@2000.0
    'EPSG','4948',  -- target_crs:  LKS-92
    0.005,          -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5052',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'LV_2020_INTRAPLATE', -- object auth+code
    'EPSG', '1139',         -- extent: Latvia - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF14_TO_LV', -- operation auth+code
    'NKG_ETRF14 to ETRS89 (LKS-92)', -- name
    'Transformation from NKG_ETRF14@2000.0 to ETRF89@1992.75', -- description
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF00
    'EPSG','4948',  -- target_crs:  LKS-92
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF14_TO_LV', 1, 'NKG', 'PAR_2020_LV'),
    ('NKG', 'ETRF14_TO_LV', 2, 'NKG', 'LV_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5053',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF14_TO_LV',      -- object auth+code
    'EPSG', '1139',             -- extent: Latvia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2014 -> ETRF89@1992.75 (LV)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2014_TO_LV',  -- operation auth+code
    'ITRF2014 to ETRS89 (LKS-92)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89 (LKS-92)', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'EPSG', '4948', -- target_crs:  LKS-92
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_LV', 1, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_LV', 2, 'NKG', 'NKG_ETRF14_TO_ETRF2014'),
    ('NKG', 'ITRF2014_TO_LV', 3, 'NKG', 'PAR_2020_LV'),
    ('NKG', 'ITRF2014_TO_LV', 4, 'NKG', 'LV_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5054',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_LV',    -- object auth+code
    'EPSG', '1139',             -- extent: Latvia - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);


INSERT INTO "supersession" VALUES (
    'concatenated_operation',
    'NKG', 'ITRF2000_TO_LV',
    'concatenated_operation',
    'NKG', 'ITRF2014_TO_LV',
    'NKG',
    0
);


-------------------------------------------------------
-- Transformation: NKG_ETRF14 -> ETRF2000@2003.75 (LT)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'LT_2020_INTRAPLATE',            -- object auth+code
    'ETRF2000@2000.0 to ETRF2000@2003.75 (LKS94)',  -- name
    NULL,                                   -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=3.75 +grids=eur_nkg_nkgrf17vel.tif',
    'EPSG','7930',  -- source_crs:  ETRF2000@2000.0
    'EPSG','4950',  -- target_crs:  LKS94
    0.005,          -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5055',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'LT_2020_INTRAPLATE', -- object auth+code
    'EPSG', '1145',         -- extent: Lithuania - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF14_TO_LT', -- operation auth+code
    'NKG_ETRF14 to LKS94', -- name
    'Transformation from NKG_ETRF14@2000.0 to ETRF2000@2003.75 (LKS94)', -- description
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF00
    'EPSG','4950',  -- target_crs:  LKS94
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF14_TO_LT', 1, 'NKG', 'PAR_2020_LT'),
    ('NKG', 'ETRF14_TO_LT', 2, 'NKG', 'LT_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5056',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF14_TO_LT',      -- object auth+code
    'EPSG', '1145',             -- extent: Lithuania - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2014 -> ETRF2000@2003.75 (LT)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2014_TO_LT',  -- operation auth+code
    'ITRF2014 to ETRS89(LT)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89(LT)', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'EPSG', '4950', -- target_crs:  LKS94
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_LT', 1, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_LT', 2, 'NKG', 'NKG_ETRF14_TO_ETRF2014'),
    ('NKG', 'ITRF2014_TO_LT', 3, 'NKG', 'PAR_2020_LT'),
    ('NKG', 'ITRF2014_TO_LT', 4, 'NKG', 'LT_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5057',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_LT',    -- object auth+code
    'EPSG', '1145',             -- extent: Lithuania - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);


INSERT INTO "supersession" VALUES (
    'concatenated_operation',
    'NKG', 'ITRF2000_TO_LT',
    'concatenated_operation',
    'NKG', 'ITRF2014_TO_LT',
    'NKG',
    0
);


-------------------------------------------------------
-- Transformation: NKG_ETRF14 -> ETRF93@1995.0 (NO)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'NO_2020_INTRAPLATE',        -- object auth+code
    'ETRF93@2000.0 to ETRF93@1995.0',   -- name
    NULL,                               -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-5 +grids=eur_nkg_nkgrf17vel.tif',
    'EPSG','7922',  -- source_crs:  ETRF93@2000.0
    'EPSG','4936',  -- target_crs:  ETRS89 (NO)
    0.005,          -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5058',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'NO_2020_INTRAPLATE', -- object auth+code
    'EPSG', '1352',         -- extent: Norway - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF14_TO_NO', -- operation auth+code
    'NKG_ETRF14 to ETRS89(NO)', -- name
    'Transformation from NKG_ETRF14@2000.0 to ETRF93@1995.0', -- description
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF00
    'EPSG','4936',  -- target_crs:  ETRS89 (NO)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF14_TO_NO', 1, 'NKG', 'PAR_2020_NO'),
    ('NKG', 'ETRF14_TO_NO', 2, 'NKG', 'NO_2020_INTRAPLATE')
;

INSERT INTO "usage" VALUES (
    'NKG', '5059',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF14_TO_NO',      -- object auth+code
    'EPSG', '1352',             -- extent: Norway - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);


-------------------------------------------------------
-- Transformation: ITRF2014 -> ETRF93@1995.0 (NO)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2014_TO_NO',  -- operation auth+code
    'ITRF2014 to ETRS89(NO)', -- name
    'Time-dependent transformation from ITRF2014 to ETRS89(NO)', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'EPSG', '4936', -- target_crs:  ETRS89(NO)
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_NO', 1, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_NO', 2, 'NKG', 'NKG_ETRF14_TO_ETRF2014'), 
    ('NKG', 'ITRF2014_TO_NO', 3, 'NKG', 'NKG_ETRF14_ETRF93_2000'),
    ('NKG', 'ITRF2014_TO_NO', 4, 'NKG', 'NO_2020_INTRAPLATE')
;

INSERT INTO "usage" VALUES (
    'NKG', '5060',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_NO',    -- object auth+code
    'EPSG', '1352',             -- extent: Norway - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);

INSERT INTO "supersession" VALUES (
    'concatenated_operation',
    'NKG', 'ITRF2000_TO_NO',
    'concatenated_operation',
    'NKG', 'ITRF2014_TO_NO',
    'NKG',
    0
);


-------------------------------------------------------
-- Transformation: NKG_ETRF14 -> ETRF97@1999.5 (SE)
-------------------------------------------------------

INSERT INTO "other_transformation" (
    auth_name,
    code,
    name,
    description,
    method_auth_name,
    method_code,
    method_name,
    source_crs_auth_name,
    source_crs_code,
    target_crs_auth_name,
    target_crs_code,
    accuracy,
    operation_version,
    deprecated
)
VALUES(
    'NKG', 'SE_2020_INTRAPLATE',-- object auth+code
    'ETRF97@2000.0 to ETRF97@1999.5', -- name
    NULL, -- description
    'PROJ', 'PROJString',               -- method auth+cod
    '+proj=deformation +dt=-0.5 +grids=eur_nkg_nkgrf17vel.tif',
    'EPSG','7928',  -- source_crs:  ETRF97@2000.0
    'EPSG','4976',  -- target_crs:  SWEREF99
    0.005,          -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);

INSERT INTO "usage" VALUES (
    'NKG', '5061',          -- usage auth+code
    'other_transformation', -- object_table_name
    'NKG', 'SE_2020_INTRAPLATE', -- object auth+code
    'EPSG', '1225',         -- extent: Sweden - onshore and offshore
    'NKG',  'SCOPE_GENERIC' -- scope
);

INSERT INTO "concatenated_operation" VALUES(
    'NKG', 'ETRF14_TO_SE', -- operation auth+code
    'NKG_ETRF14 to SWEREF99', -- name
    'Transformation from NKG_ETRF14@2000.0 to SWEREF99 (ETRF97@1999.5)', -- description
    'NKG', 'ETRF14',-- source_crs:  NKG_ETRF00
    'EPSG','4976',  -- target_crs:  SWEREF99
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated
);


INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ETRF14_TO_SE', 1, 'NKG', 'PAR_2020_SE'),
    ('NKG', 'ETRF14_TO_SE', 2, 'NKG', 'SE_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5062',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ETRF14_TO_SE',      -- object auth+code
    'EPSG', '1225',             -- extent: Sweden - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);



-------------------------------------------------------
-- Transformation: ITRF2014 -> ETRF97@1999.5 (SE)
-------------------------------------------------------

INSERT INTO "concatenated_operation"  VALUES (
    'NKG', 'ITRF2014_TO_SE',  -- operation auth+code
    'ITRF2014 to ETRS89(SE)', -- name
    'Time-dependent transformation from ITRF2014 to SWEREF99', -- description
    'EPSG', '7789', -- source_crs:  ITRF2014
    'EPSG', '4976', -- target_crs:  SWEREF99
    0.01,           -- accuracy
    'NKG 2020',     -- operation_version
    0               -- deprecated

);

INSERT INTO "concatenated_operation_step" (
    operation_auth_name, operation_code, step_number, step_auth_name, step_code
) VALUES
    ('NKG', 'ITRF2014_TO_SE', 1, 'EPSG', '8366'), -- ITRF2014 -> ETRF2014
    ('NKG', 'ITRF2014_TO_SE', 2, 'NKG', 'NKG_ETRF14_TO_ETRF2014'),
    ('NKG', 'ITRF2014_TO_SE', 3, 'NKG', 'PAR_2020_SE'),
    ('NKG', 'ITRF2014_TO_SE', 4, 'NKG', 'SE_2020_INTRAPLATE')
;


INSERT INTO "usage" VALUES (
    'NKG', '5063',              -- usage auth+code
    'concatenated_operation',   -- object_table_name
    'NKG', 'ITRF2014_TO_SE',    -- object auth+code
    'EPSG', '1225',             -- extent: Sweden - onshore and offshore
    'NKG', 'SCOPE_GENERIC'      -- scope auth+code
);


INSERT INTO "supersession" VALUES (
    'concatenated_operation',
    'NKG', 'ITRF2000_TO_SE',
    'concatenated_operation',
    'NKG', 'ITRF2014_TO_SE',
    'NKG',
    0
);


