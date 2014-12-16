#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_CHUNKS	3
#define MAX_CHUNK_SIZE	1024 * 64
#define MAX_BUFFER	MAX_CHUNK_SIZE + 1024
#define INT_BUFFER_SIZE	20

typedef struct{
	uint32_t size;			//  4 bytes
	char *data;			//  8 bytes, system dependent
} redis_chunk;

typedef struct{
	unsigned char chunk_count;	//  1 bytes
	redis_chunk chunks[MAX_CHUNKS];	//  3 x 10  bytes
	uint32_t buffer_size;		//  8 bytes, system dependent
	char *buffer;			//  8 bytes, system dependent
} redis_client;



static char *_datacat(char *dest, const char *src, uint32_t dest_size, uint32_t src_size);



inline void proto_clear(redis_client *r){
	memset(r, 0, sizeof(*r));
}

int proto_feed(redis_client *r, const char *data, uint32_t data_size){
	if (!proto_isdone(r))
		return -1;	// done already

	uint32_t buff_size = r->buffer_size + data_size;

	if (buff_size > MAX_BUFFER)
		return 1;	// responce too big

	char *buff = _datacat(r->buffer, data, r->buffer_size, data_size);

	if (buff == NULL)
		return 2;	// out of memory

	r->buffer = buff;
	r->buffer_size = buff_size;

	return 0;
}

inline int proto_feeds(redis_client *r, const char *data){
	return proto_feed(r, data, strlen(data));
}

static int _proto_readln(const redis_client *r, uint32_t *pos){
	const char *src = r->buffer;
	uint32_t src_size = r->buffer_size;

	if (src[*pos] == '\r' && src[(*pos) + 1] == '\n'){
		*pos = (*pos) + 2;
		return 0;
	}

	return 1;
}

static int _proto_readint(const redis_client *r, uint32_t *pos){
	const char *src = r->buffer;
	uint32_t src_size = r->buffer_size;

	char buff[INT_BUFFER_SIZE + 1];
	unsigned char buff_pos = 0;

	for(;*pos < src_size && buff_pos < INT_BUFFER_SIZE; (*pos)++){
		char c = src[*pos];

		if (c < '0' || c > '9')
			break;

		buff[buff_pos] = c;
		buff_pos++;
	}

	buff[buff_pos] = '\0';

	return atoi(buff);
}

static int _proto_readparam(redis_client *r, uint32_t *pos, unsigned char index){
	char *src = r->buffer;
	uint32_t src_size = r->buffer_size;

	if (r->buffer[*pos] != '$')
		return 1;	// no $ at the beginnging

	(*pos)++;

	int size = _proto_readint(r, pos);
	if (size <= 0 || size > MAX_CHUNK_SIZE)
		return 2;	// too big chunk

	if ( _proto_readln(r, pos) )
		return 3;	// no \r\n

	// store size and data
	r->chunks[index].size = size;
	r->chunks[index].data = & src[*pos];

	// the pointer is set. however we need to check if all data is there.
	*pos = *pos + size;

	if (*pos > src_size)
		return 4;	// data not received completely yet.

	if ( _proto_readln(r, pos) )
		return 3;	// no \r\n

	return 0;
}

int proto_isdone(redis_client *r){
	if (r->buffer_size < 4)	// 4 bytes - "*1\r\n"
		return 1;	// empty buffer

	uint32_t pos = 0;

	if (r->buffer[pos] != '*')
		return 2;	// no * at the beginnging

	pos++;

	int paramcount = _proto_readint(r, & pos);

	if (paramcount == 0 || paramcount > MAX_CHUNKS)
		return 3; 	// param count can not be 0

	if ( _proto_readln(r, & pos) )
		return 4;	// no \r\n

	unsigned char i;
	for(i = 0; i < paramcount; i++){
		if (_proto_readparam(r, & pos, i))
			return 5;
	}

	r->chunk_count = paramcount;

	return 0;
}

void proto_dump(redis_client *r){
	if (! r->chunk_count){
		printf("no data\n");
		return;
	}

	printf("Listing redis client %p\n", r);

	unsigned char i;
	for(i = 0; i < r->chunk_count; i++){
		uint32_t size = r->chunks[i].size;

		char *data = r->chunks[i].data;

		if (size){
			printf("%u | %5d | %.*s\n", i, size, size, data);
		}else{
			printf("%u | %5d | %s\n", i, size, "(none)");
		}
	}
}

static inline void _proto_test(redis_client *r, const char *s){
	proto_clear(r);

	proto_feeds(r, s);

	int x = proto_isdone(r);

	printf("proto_isdone = %d\n", x);

	proto_dump(r);
}

int main(){
	redis_client r_placeholder;
	redis_client *r = & r_placeholder;

	_proto_test(r, "*3\r\n$3\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
	_proto_test(r, "*2\r\n$3\r\nGET\r\n$2\r\nBG\r\n");

	return 0;
}



static char *_datacat(char *dest, const char *src, uint32_t dest_size, uint32_t src_size){
	if (src_size == 0)
		return dest;

	char *buff = realloc(dest, dest_size + src_size);

	if (buff == NULL)
		return NULL;

	memcpy( & buff[ dest_size ], src, src_size);

	return buff;
}


