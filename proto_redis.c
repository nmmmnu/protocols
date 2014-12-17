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

static int _proto_readln(   const proto_client *r, const char *src, int src_size, uint32_t *pos);
static int _proto_readint(  const proto_client *r, const char *src, int src_size, uint32_t *pos);
static int _proto_readparam_redis(proto_client *r, const char *src, int src_size, uint32_t *pos, unsigned char index);

const char* g_proto_error;
#define  ERR_BUFFER_NOT_READ	"buffer not read complete, non fatal error"
#define  ERR_NO_STAR		"no * at the beginnging"
#define  ERR_NO_DOLLAR		"no $ at the beginnging"
#define  ERR_PARAM_COUNT	"param count can not be 0 or can not be more than MAX_CHUNKS"
#define  ERR_BIG_CHUNK		"chunk too big"

int proto_parse_redis(proto_client *r, const char *src, int src_size){
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

	int paramcount = _proto_readint(r, src, src_size, & pos);

	if (paramcount == 0 || paramcount > MAX_CHUNKS){
		g_proto_error = ERR_PARAM_COUNT;
		return 3;
	}

	int ln;
	if ((ln = _proto_readln(r, src, src_size, & pos)))
		return ln;	// no \r\n

	unsigned char i;
	for(i = 0; i < paramcount; i++){
		if ((ln = _proto_readparam_redis(r, src, src_size, & pos, i)))
			return ln;
	}

	r->chunk_count = paramcount;

	return 0;
}

void proto_dump(const proto_client *r){
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

static int _proto_readln(const proto_client *r, const char *src, int src_size, uint32_t *pos){
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

	int size = _proto_readint(r, src, src_size, pos);
	if (size <= 0 || size > MAX_CHUNK_SIZE){
		g_proto_error = ERR_BIG_CHUNK;
		return 2;
	}

	int ln;
	if ((ln = _proto_readln(r, src, src_size, pos)))
		return ln;	// no \r\n

	// store size and data
	r->chunks[index].size = size;
	r->chunks[index].data = & src[*pos];

	// the pointer is set. however we need to check if all data is there.
	*pos = *pos + size;

	if (*pos > src_size){
		g_proto_error = ERR_BUFFER_NOT_READ;
		return -4;
	}

	if ((ln = _proto_readln(r, src, src_size, pos)))
		return ln;	// no \r\n

	return 0;
}



static inline void _proto_test_redis(proto_client *r, const char *test){
	printf("Test #2 - incremental add\n");

	size_t size = strlen(test);
	char btest[size + 1];

	int result;
	unsigned char i;
	for(i = 0; i < size; i++){
		btest[i] = test[i];
		btest[i + 1] = '\0';

		result = proto_parse_redis(r, btest, strlen(btest));

		printf("%3u | %5d\n", i, result);

		if (result > 0){
			printf("Test FAIL!!!\n");
			printf("Buffer: %s\n", btest);
			printf("Last Error: %s\n", g_proto_error);
			return;
		}
	}

	printf("proto_parse_redis = %d\n", result);

	proto_dump(r);

	printf("OK\n");
}


int main(){
	proto_client r_placeholder;
	proto_client *r = & r_placeholder;

	_proto_test_redis(r, "*2\r\n$3\r\nGET\r\n$2\r\nBG\r\n");
	_proto_test_redis(r, "*3\r\n$3\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");

//	_proto_test_redis(r, "*3\r\n$2\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
//	_proto_test_redis(r, "*2\r\n$5\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
//	_proto_test_redis(r, "*42\r\n$5\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
//	_proto_test_redis(r, "*\r\n$5\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");

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

