create table public.segments
(
    segment_id bigserial primary key NOT NULL,
    segment_hash bytea NOT NULL GENERATED ALWAYS AS (sha256(segment_data::bytea)) STORED,
    segment_data char(64) NOT NULL,
    segment_count bigint NOT NULL
);

create table public.files
(
    file_id bigserial primary key NOT NULL,
    file_name text
);

create table public.data
(
    file_id bigint REFERENCES public.files(file_id),
    segment_num bigint NOT NULL,
    segment_id bigint REFERENCES public.segments(segment_id) NOT NULL
);




CREATE INDEX hash_segment_hash on segments using Hash(segment_hash);
CREATE INDEX hash_segment_id on segments using Hash(segment_id);



CREATE OR REPLACE FUNCTION select_from_temp_table(_tbl regclass)
    RETURNS table(pos bigint, data char[64])
    LANGUAGE plpgsql AS
$func$
BEGIN
    RETURN QUERY EXECUTE format('SELECT * FROM %I', _tbl);
END
$func$;


CREATE OR REPLACE FUNCTION process_file_data(file_name text)
    RETURNS int
    LANGUAGE plpgsql
AS $$
DECLARE
    file_id_ bigint;
BEGIN
    IF NOT EXISTS (SELECT * FROM pg_tables WHERE tablename='temp_file_data')
    THEN
        return -1;
    end if;

    insert into public.files (file_name) values (file_name) returning file_id into file_id_;

    INSERT INTO public.segments (segment_data, segment_count)
    SELECT DISTINCT t.data, 0
    FROM temp_file_data t
    WHERE NOT EXISTS (
        SELECT 1
        FROM public.segments s
        WHERE s.segment_hash = sha256(t.data::bytea)
    );

    update public.segments s SET segment_count=segment_count+sub.count
    from
        (
            select data,count(data) as count
            from temp_file_data t
            group by t.data
        ) as sub
    where s.segment_data=sub.data;

    INSERT INTO public.data (segment_num, segment_id,file_id)
    select pos,se.segment_id,file_id_
    from temp_file_data
             inner join public.segments se
                        on temp_file_data.data = se.segment_data;
    Drop  table if exists temp_file_data;
    return 0;
END $$;



CREATE OR REPLACE FUNCTION process_file_data2(file_name text)
    RETURNS int
    LANGUAGE plpgsql
AS $$
DECLARE
    file_id_ bigint;
    table_name text := 'temp_file_' || file_name;
    query text;
BEGIN
    IF NOT EXISTS (SELECT * FROM pg_tables WHERE tablename = table_name)
    THEN
        return -1;
    end if;

    insert into public.files (file_name) values (file_name) returning file_id into file_id_;

    query := 'INSERT INTO public.segments (segment_data, segment_count)
              SELECT DISTINCT t.data, 0
              FROM ' || quote_ident(table_name) || ' t
              WHERE NOT EXISTS (
                  SELECT 1
                  FROM public.segments s
                  WHERE s.segment_hash = sha256(t.data::bytea)
              )';

    EXECUTE query;

    query := 'UPDATE public.segments s SET segment_count = segment_count + sub.count
              FROM (
                  SELECT data, COUNT(data) AS count
                  FROM ' || quote_ident(table_name) || ' t
                  GROUP BY t.data
              ) AS sub
              WHERE s.segment_data = sub.data';

    EXECUTE query;

    query := 'INSERT INTO public.data (segment_num, segment_id, file_id)
              SELECT pos, se.segment_id, ' || file_id_ || '
              FROM ' || quote_ident(table_name) || '
              INNER JOIN public.segments se
              ON ' || quote_ident(table_name) || '.data = se.segment_data';

    EXECUTE query;

    EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(table_name);

    RETURN 0;
END
$$;




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
                 inner join public.segments s on s.segment_id = data.segment_id
        where data.file_id=(select files.file_id from files where files.file_name=fileName)
        order by segment_num;

END $$;

