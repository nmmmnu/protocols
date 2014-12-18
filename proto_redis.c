#include "proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHUNKS	PROTO_MAXCHUNKS
#define MAX_CHUNK_SIZE	PROTO_MAXCHUNKSIZE
#define MAX_BUFFER	PROTO_MAXBUFFER
#define INT_BUFFER_SIZE	20

#define _STR(x)	#x

#define REDIS_STAR	'*'
#define REDIS_DOLLAR	'$'

#define REDIS_OK	"+OK\r\n"
#define REDIS_ERR	"-ERR Error\r\n"
#define REDIS_NOT_FOUND	_STR(REDIS_DOLLAR) "-1\r\n"
#define REDIS_VALUE	_STR(REDIS_DOLLAR) "%u\r\n%.*s\r\n"

static const char* g_proto_error;

#define  ERR_BUFFER_NOT_READ	"buffer not read complete, non fatal error"
#define  ERR_NO_STAR		"no " _STR(REDIS_STAR)   " at the beginnging"
#define  ERR_NO_DOLLAR		"no " _STR(REDIS_DOLLAR) " at the beginnging"
#define  ERR_PARAM_COUNT	"param count can not be 0 or can not be more than " _STR(PROTO_MAXCHUNKS)
#define  ERR_BIG_CHUNK		"chunk too big"

static int _proto_readln(const char *src, int src_size, uint32_t *pos);
static int _proto_readint(const char *src, int src_size, uint32_t *pos);
static int _proto_readparam_redis(const char *src, int src_size, uint32_t *pos, proto_chunk_t *chunk);
static int _proto_parse_redis(proto_client_t *r, const char *src, uint32_t src_size);

static proto_response_buffer_t *_proto_response_allocate_string(const char *data);
static proto_response_buffer_t *_proto_response_allocate_value(const char *value, uint32_t value_size);

const char *proto_system(){
	return "redis";
}

const char *proto_error(){
	return g_proto_error;
}

int proto_parse(proto_client_t *r, const char *src, uint32_t src_size){
	return _proto_parse_redis(r, src, src_size);
}

proto_response_buffer_t *proto_response(proto_response_status_t status, const char *data, uint32_t data_size){
	switch(status){
	case PROTO_RESPONSEOK:
		return _proto_response_allocate_string(REDIS_OK);

	case PROTO_RESPONSENOTFOUND:
		return _proto_response_allocate_string(REDIS_NOT_FOUND);

	case PROTO_RESPONSEVALUE:
		return _proto_response_allocate_value(data, data_size);

	case PROTO_RESPONSEERR:
	default:
		return _proto_response_allocate_string(REDIS_ERR);

	}
}

void proto_dump(const proto_client_t *r){
	printf("Listing redis client %p\n", r);

	if (! r->chunk_count){
		printf("no data\n");
		return;
	}

	unsigned char i;
	for(i = 0; i < r->chunk_count; i++){
		uint32_t size = r->chunks[i].size;

		const char *data = r->chunks[i].data;

		if (size){
			printf("%u | %5d | %.*s\n", i, size, size, data);
		}else{
			printf("%u | %5d | %s\n", i, size, "(none)");
		}
	}
}

static int _proto_readln(const char *src, int src_size, uint32_t *pos){
	if (*pos + 2 > src_size){
		g_proto_error = ERR_BUFFER_NOT_READ;
		return -1;
	}

	if (src[*pos] == '\r' && src[(*pos) + 1] == '\n'){
		*pos = *pos + 2;
		return 0;
	}

	return 1;
}

static int _proto_readint(const char *src, int src_size, uint32_t *pos){
	char buff[INT_BUFFER_SIZE + 1];
	unsigned char buff_pos = 0;

	for(;*pos < src_size && buff_pos < INT_BUFFER_SIZE; (*pos)++){
		char c = src[*pos];

		if (c != '-' && c != '+' && (c < '0' || c > '9') )
			break;

		buff[buff_pos] = c;
		buff_pos++;
	}

	buff[buff_pos] = '\0';

	return atoi(buff);
}

static int _proto_readparam_redis(const char *src, int src_size, uint32_t *pos, proto_chunk_t *chunk){
	if (*pos + 1 > src_size){
		g_proto_error = ERR_BUFFER_NOT_READ;
		return -1;
	}

	if (src[*pos] != REDIS_DOLLAR){
		g_proto_error = ERR_NO_DOLLAR;
		return 1;
	}

	(*pos)++;

	if (*pos + 1 > src_size){
		g_proto_error = ERR_BUFFER_NOT_READ;
		return -1;
	}

	int size = _proto_readint(src, src_size, pos);
	if (size <= 0 || size > MAX_CHUNK_SIZE){
		g_proto_error = ERR_BIG_CHUNK;
		return 2;
	}

	int ln;
	if ((ln = _proto_readln(src, src_size, pos)))
		return ln;	// no \r\n

	// store size and data
	chunk->size = size;
	chunk->data = & src[*pos];

	// the pointer is set. however we need to check if all data is there.
	*pos = *pos + size;

	if (*pos > src_size){
		g_proto_error = ERR_BUFFER_NOT_READ;
		return -4;
	}

	if ((ln = _proto_readln(src, src_size, pos)))
		return ln;	// no \r\n

	return 0;
}

static int _proto_parse_redis(proto_client_t *r, const char *src, uint32_t src_size){
	memset(r, 0, sizeof(*r));

	if (src_size < 8){	// 4 bytes - "*1\r\n$1\r\n"
		g_proto_error = ERR_BUFFER_NOT_READ;
		return -1;
	}

	uint32_t pos = 0;

	if (src[pos] != REDIS_STAR){
		g_proto_error = ERR_NO_STAR;
		return 2;
	}

	pos++;

	if (pos + 1 > src_size){
		g_proto_error = ERR_BUFFER_NOT_READ;
		return -1;
	}

	int paramcount = _proto_readint(src, src_size, & pos);

	if (paramcount == 0 || paramcount > MAX_CHUNKS){
		g_proto_error = ERR_PARAM_COUNT;
		return 3;
	}

	int ln;
	if ((ln = _proto_readln(src, src_size, & pos)))
		return ln;	// no \r\n

	unsigned char i;
	for(i = 0; i < paramcount; i++){
		if ((ln = _proto_readparam_redis(src, src_size, & pos, & r->chunks[i])))
			return ln;
	}

	r->chunk_count = paramcount;

	return 0;
}

static proto_response_buffer_t *_proto_response_allocate_string(const char *data){
	uint32_t data_size = strlen(data);

	proto_response_buffer_t *buffer = malloc(sizeof(buffer) + data_size);

	if (buffer == NULL)
		return NULL;

	buffer->data_size = data_size;
	memcpy(buffer->data, data, data_size);

	return buffer;
}

static proto_response_buffer_t *_proto_response_allocate_value(const char *value, uint32_t value_size){
	uint32_t data_size = snprintf(NULL, 0, REDIS_VALUE, value_size, value_size, value);

	proto_response_buffer_t *buffer = malloc(sizeof(buffer) + data_size);

	if (buffer == NULL)
		return NULL;

	buffer->data_size = data_size;
	sprintf(buffer->data, REDIS_VALUE, value_size, value_size, value);

	return buffer;
}

/*
static char *_datacat(char *dest, const char *src, uint32_t dest_size, uint32_t src_size){
	if (src_size == 0)
		return dest;

	char *buff = realloc(dest, dest_size + src_size);

	if (buff == NULL)
		return NULL;

	memcpy( & buff[ dest_size ], src, src_size);

	return buff;
}

int proto_feed(proto_client *r, const char *data, uint32_t data_size){
	if (!proto_isdone(r))
		return -1;	// done already

	uint32_t buff_size = buffer_size + data_size;

	if (buff_size > MAX_BUFFER)
		return 1;	// responce too big

	char *buff = _datacat(buffer, data, r->buffer_size, data_size);

	if (buff == NULL)
		return 2;	// out of memory

	r->buffer = buff;
	r->buffer_size = buff_size;

	return 0;
}

inline int proto_feeds(proto_client *r, const char *data){
	return proto_feed(r, data, strlen(data));
}

*/

