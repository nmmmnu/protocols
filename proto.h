#ifndef _PROTO_REDIS_H
#define _PROTO_REDIS_H

#include <stdint.h>

#define PROTO_REDIS_MAX_CHUNKS		4
#define PROTO_REDIS_MAX_CHUNK_SIZE	1024 * 64
#define PROTO_REDIS_MAX_BUFFER		PROTO_REDIS_MAX_CHUNK_SIZE + 1024

typedef struct{
	uint32_t size;					//  4 bytes
	const char *data;				//  8 bytes, system dependent
} proto_chunk;

typedef struct{
	unsigned char chunk_count;			//  1 bytes
	proto_chunk chunks[PROTO_REDIS_MAX_CHUNKS];	//  PROTO_REDIS_MAX_CHUNKS x 12  bytes
} proto_client;

const char *proto_system();
const char *proto_error();

int proto_parse(proto_client *r, const char *src, int src_size);

void proto_dump(const proto_client *r);

#endif

