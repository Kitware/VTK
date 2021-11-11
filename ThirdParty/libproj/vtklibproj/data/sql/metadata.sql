-- Version of the database structure.
-- The major number indicates an incompatible change (e.g. table or column
-- removed or renamed).
-- The minor number is incremented if a backward compatible change done, that
-- is the new database can still work with an older PROJ version.
-- When updating those numbers, the DATABASE_LAYOUT_VERSION_MAJOR and
-- DATABASE_LAYOUT_VERSION_MINOR constants in src/iso19111/factory.cpp must be
-- updated as well.
INSERT INTO "metadata" VALUES('DATABASE.LAYOUT.VERSION.MAJOR', 1);
INSERT INTO "metadata" VALUES('DATABASE.LAYOUT.VERSION.MINOR', 1);

INSERT INTO "metadata" VALUES('EPSG.VERSION', 'v10.027');
INSERT INTO "metadata" VALUES('EPSG.DATE', '2021-06-17');

-- The value of ${PROJ_VERSION} is substituted at build time by the actual
-- value.
INSERT INTO "metadata" VALUES('PROJ.VERSION', '${PROJ_VERSION}');

-- Version of the PROJ-data package with which this database is the most
-- compatible.
INSERT INTO "metadata" VALUES('PROJ_DATA.VERSION', '1.7');
