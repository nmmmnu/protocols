#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_CHUNKS	3
#define MAX_CHUNK_SIZE	1024 * 64
#define MAX_BUFFER	MAX_CHUNK_SIZE + 1024
#define INT_BUFFER_SIZE	20

#define REDIS_STAR	'*'
#define REDIS_DOLLAR	'$'

typedef struct{
	uint32_t size;			//  4 bytes
	const char *data;			//  8 bytes, system dependent
} proto_chunk;

typedef struct{
	unsigned char chunk_count;	//  1 bytes
	proto_chunk chunks[MAX_CHUNKS];	//  3 x 10  bytes
} proto_client;

static int _proto_readln( const proto_client *r, const char *buffer, int buffer_size, uint32_t *pos);
static int _proto_readint(const proto_client *r, const char *buffer, int buffer_size, uint32_t *pos);
static int _proto_readparam_redis(    proto_client *r, const char *buffer, int buffer_size, uint32_t *pos, unsigned char index);

int proto_parse_redis(proto_client *r, const char *buffer, int buffer_size){
	memset(r, 0, sizeof(*r));

	if (buffer_size < 8)	// 4 bytes - "*1\r\n$1\r\n"
		return 1;	// empty buffer

	uint32_t pos = 0;

	if (buffer[pos] != REDIS_STAR)
		return 2;	// no * at the beginnging

	pos++;

	int paramcount = _proto_readint(r, buffer, buffer_size, & pos);

	if (paramcount == 0 || paramcount > MAX_CHUNKS)
		return 3; 	// param count can not be 0

	if ( _proto_readln(r, buffer, buffer_size, & pos) )
		return 4;	// no \r\n

	unsigned char i;
	for(i = 0; i < paramcount; i++){
		if (_proto_readparam_redis(r, buffer, buffer_size, & pos, i))
			return 5;
	}

	r->chunk_count = paramcount;

	return 0;
}

void proto_dump(const proto_client *r){
	if (! r->chunk_count){
		printf("no data\n");
		return;
	}

	printf("Listing redis client %p\n", r);

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

static inline void _proto_test_redis(proto_client *r, const char *s){
	int x = proto_parse_redis(r, s, strlen(s));

	printf("proto_isdone = %d\n", x);

	proto_dump(r);
}

/*
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

int main(){
	proto_client r_placeholder;
	proto_client *r = & r_placeholder;

	_proto_test_redis(r, "*3\r\n$3\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
	_proto_test_redis(r, "*2\r\n$3\r\nGET\r\n$2\r\nBG\r\n");

	return 0;
}



static int _proto_readln(const proto_client *r, const char *src, int src_size, uint32_t *pos){
	if (src[*pos] == '\r' && src[(*pos) + 1] == '\n'){
		*pos = (*pos) + 2;
		return 0;
	}

	return 1;
}

static int _proto_readint(const proto_client *r, const char *src, int src_size, uint32_t *pos){
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

static int _proto_readparam_redis(proto_client *r, const char *src, int src_size, uint32_t *pos, unsigned char index){
	if (src[*pos] != REDIS_DOLLAR)
		return 1;	// no $ at the beginnging

	(*pos)++;

	int size = _proto_readint(r, src, src_size, pos);
	if (size <= 0 || size > MAX_CHUNK_SIZE)
		return 2;	// too big chunk

	if ( _proto_readln(r, src, src_size, pos) )
		return 3;	// no \r\n

	// store size and data
	r->chunks[index].size = size;
	r->chunks[index].data = & src[*pos];

	// the pointer is set. however we need to check if all data is there.
	*pos = *pos + size;

	if (*pos > src_size)
		return 4;	// data not received completely yet.

	if ( _proto_readln(r, src, src_size, pos) )
		return 3;	// no \r\n

	return 0;
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
*/

