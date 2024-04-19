
-- Append NKG to authority references
UPDATE
   authority_to_authority_preference
SET
   allowed_authorities = allowed_authorities || ',NKG'
WHERE
   source_auth_name = 'EPSG' AND target_auth_name = 'EPSG';

INSERT INTO "authority_to_authority_preference"
    (source_auth_name,target_auth_name, allowed_authorities)
VALUES
    ('NKG', 'EPSG', 'NKG,PROJ,EPSG');
