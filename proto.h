#ifndef _PROTO_REDIS_H
#define _PROTO_REDIS_H

#include <stdint.h>

#define PROTO_MAXCHUNKS		4
#define PROTO_MAXCHUNKSIZE	1024 * 64
#define PROTO_MAXBUFFER		PROTO_MAXCHUNKSIZE + 1024

typedef struct{
	uint32_t size;				//  4 bytes
	const char *data;			//  8 bytes, system dependent
} proto_chunk_t;

typedef struct{
	unsigned char chunk_count;		//  1 bytes
	proto_chunk_t chunks[PROTO_MAXCHUNKS];	//  PROTO_MAX_CHUNKS x 12  bytes
} proto_client_t;

typedef enum {
	PROTO_RESPONSEOK,
	PROTO_RESPONSEERR,
	PROTO_RESPONSEVALUE,
	PROTO_RESPONSENOTFOUND
} proto_response_status_t;

typedef struct{
	uint32_t data_size;			//  4 bytes
	char data[];				//  dynamic
} proto_response_buffer_t;

const char *proto_system();
const char *proto_error();

int proto_parse(proto_client_t *r, const char *src, uint32_t src_size);
proto_response_buffer_t *proto_response(proto_response_status_t status, const char *data, uint32_t data_size);
void proto_dump(const proto_client_t *r);

#endif

