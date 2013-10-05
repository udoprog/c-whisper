// vim: foldmethod=marker
#include "wsp_buffer.h"

// parse & dump macros {{{
#if BYTE_ORDER == LITTLE_ENDIAN
#define READ4(t, l) do {\
    (t)[0] = (l)[3];\
    (t)[1] = (l)[2];\
    (t)[2] = (l)[1];\
    (t)[3] = (l)[0];\
} while (0);

#define READ8(t, l) do {\
    (t)[0] = (l)[7];\
    (t)[1] = (l)[6];\
    (t)[2] = (l)[5];\
    (t)[3] = (l)[4];\
    (t)[4] = (l)[3];\
    (t)[5] = (l)[2];\
    (t)[6] = (l)[1];\
    (t)[7] = (l)[0];\
} while (0);
#else
#define READ4(t, l) do {\
    (t)[0] = (l)[0];\
    (t)[1] = (l)[1];\
    (t)[2] = (l)[2];\
    (t)[3] = (l)[3];\
} while (0);

#define READ8(t, l) do {\
    (t)[0] = (l)[0];\
    (t)[1] = (l)[1];\
    (t)[2] = (l)[2];\
    (t)[3] = (l)[3];\
    (t)[4] = (l)[4];\
    (t)[5] = (l)[5];\
    (t)[6] = (l)[6];\
    (t)[7] = (l)[7];\
} while (0);
#endif
// inline parse & dump functions }}}

// __wsp_parse_metadata {{{
void __wsp_parse_metadata(
    wsp_metadata_b *buf,
    wsp_metadata_t *m
)
{
    READ4((char *)&m->aggregation, buf->aggregation);
    READ4((char *)&m->max_retention, buf->max_retention);
    READ4((char *)&m->x_files_factor, buf->x_files_factor);
    READ4((char *)&m->archives_count, buf->archives_count);
} // __wsp_parse_metadata }}}

// __wsp_dump_metadata {{{
void __wsp_dump_metadata(
    wsp_metadata_t *m,
    wsp_metadata_b *buf
)
{
    READ4(buf->aggregation, (char *)&m->aggregation);
    READ4(buf->max_retention, (char *)&m->max_retention);
    READ4(buf->x_files_factor, (char *)&m->x_files_factor);
    READ4(buf->archives_count, (char *)&m->archives_count);
} // __wsp_dump_metadata }}}

// __wsp_dump_archives {{{
void __wsp_dump_archives(
    wsp_archive_t *archives,
    uint32_t count,
    wsp_archive_b *buf
)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        __wsp_dump_archive(archives + i, buf + i);
    }
} // __wsp_dump_archives }}}

// __wsp_parse_archive {{{
void __wsp_parse_archive(
    wsp_archive_b *buf,
    wsp_archive_t *archive
)
{
    READ4((char *)&archive->offset, buf->offset);
    READ4((char *)&archive->spp, buf->spp);
    READ4((char *)&archive->count, buf->count);
} // __wsp_parse_archive }}}

// __wsp_dump_archive {{{
void __wsp_dump_archive(
    wsp_archive_t *archive,
    wsp_archive_b *buf
)
{
    READ4(buf->offset, (char *)&archive->offset);
    READ4(buf->spp, (char *)&archive->spp);
    READ4(buf->count, (char *)&archive->count);
} // __wsp_dump_archive }}}

// __wsp_parse_point {{{
void __wsp_parse_point(
    wsp_point_b *buf,
    wsp_point_t *p
)
{
    READ4((char *)&p->timestamp, buf->timestamp);
    READ8((char *)&p->value, buf->value);
} // __wsp_parse_point }}}

// __wsp_dump_point {{{
void __wsp_dump_point(
    wsp_point_t *p,
    wsp_point_b *buf
)
{
    READ4(buf->timestamp, (char *)&p->timestamp);
    READ8(buf->value, (char *)&p->value);
} // __wsp_dump_point }}}
