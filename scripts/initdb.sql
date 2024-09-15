create table public.segments
(
    segment_hash bytea NOT NULL GENERATED ALWAYS AS (sha256(segment_data::bytea)) STORED primary key,
    segment_data char(64) NOT NULL,
    segment_count bigint NOT NULL
);

create table public.files
(
    file_id serial primary key NOT NULL,
    file_name text NOT NULL
);

create table public.data
(
    file_id int REFERENCES public.files(file_id) NOT NULL,
    segment_num bigint NOT NULL,
    segment_hash bytea REFERENCES public.segments(segment_hash) NOT NULL
);


CREATE INDEX hash_segment_hash on segments using Hash(segment_hash);
CREATE INDEX hash_segment_data on segments using Hash(segment_data);

ALTER TABLE segments ADD CONSTRAINT unique_data_constr UNIQUE(segment_data);


CREATE OR REPLACE FUNCTION process_file_data(file_name text)
    RETURNS int
    LANGUAGE plpgsql
AS $$
DECLARE
    file_id_ bigint;
    table_name text := 'temp_file_' || file_name;
    aggregation_table_name text:='new_segments'|| file_name;
    query text;
BEGIN
    IF NOT EXISTS (SELECT * FROM pg_tables WHERE tablename = table_name)
    THEN
        return -1;
    end if;

    insert into public.files (file_name) values (file_name) returning file_id into file_id_;
    query := 'CREATE TABLE ' || quote_ident(aggregation_table_name) ||
             ' AS SELECT DISTINCT t.data, COUNT(t.data) AS count
             FROM '||quote_ident(table_name) ||' t ' ||
             'GROUP BY t.data;';

    EXECUTE query;
    query := 'INSERT INTO public.segments (segment_data, segment_count)
              SELECT ns.data, ns.count
              FROM ' || quote_ident(aggregation_table_name) || ' ns
              ON CONFLICT ON CONSTRAINT unique_data_constr
              DO UPDATE
                SET segment_count = public.segments.segment_count +  excluded.segment_count;';

    EXECUTE query;

    EXECUTE 'drop table ' || quote_ident(aggregation_table_name) || ';';

    query := 'INSERT INTO public.data (segment_num, segment_hash, file_id)
              SELECT pos, se.segment_hash, ' || file_id_ || '
              FROM ' || quote_ident(table_name) || '
              INNER JOIN public.segments se
              ON ' || quote_ident(table_name) || '.data = se.segment_data';

    EXECUTE query;

    EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(table_name);
    return 0;
END $$;




CREATE OR REPLACE FUNCTION get_file_data(fileName text)
    returns table
            (
                data char(64)
            )
    LANGUAGE plpgsql
AS $$
BEGIN
    return query
        select s.segment_data
        from data
                 inner join public.segments s on s.segment_hash = data.segment_hash
        where data.file_id=(select files.file_id from files where files.file_name=fileName)
        order by segment_num;

END $$;