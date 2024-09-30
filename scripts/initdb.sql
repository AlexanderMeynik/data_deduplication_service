create table public.segments --todo remove
(
    segment_hash  bytea  NOT NULL GENERATED ALWAYS AS (sha256(segment_data::bytea)) STORED primary key,
    segment_data  bytea  NOT NULL,
    segment_count bigint NOT NULL
);
create table public.directories
(
    dir_id   serial primary key NOT NULL,
    dir_path tsvector           NOT NULL
);

create table public.files
(
    file_id       serial primary key                         NOT NULL,
    file_name     tsvector                                   NOT NULL,
    dir_id        int REFERENCES public.directories (dir_id) NULL,
    size_in_bytes bigint                                     NULL
);

create table public.data
(
    file_id      int REFERENCES public.files (file_id)           NOT NULL,
    segment_num  bigint                                          NOT NULL,
    segment_hash bytea REFERENCES public.segments (segment_hash) NOT NULL
);


CREATE INDEX hash_segment_hash on segments using Hash (segment_hash);
CREATE INDEX hash_segment_data on segments using Hash (segment_data);

CREATE INDEX dir_id_hash on files using Hash (dir_id);

CREATE INDEX dir_gin_index on directories using gin (dir_path);
CREATE INDEX files_gin_index on files using gin (file_name);


ALTER TABLE segments
    ADD CONSTRAINT unique_data_constr UNIQUE (segment_data);
ALTER TABLE files
    ADD CONSTRAINT unique_file_constr UNIQUE (file_name);
