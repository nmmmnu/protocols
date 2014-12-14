#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_CHUNKS 3
#define MAX_BUFFER_SIZE(bits) ((1UL << bits))

typedef struct{
	uint16_t size;			//  2 bytes
	char *data;			//  8 bytes, system dependent
} redis_chunk;				// 10  bytes

typedef struct{
	unsigned char chunk_count;	//  1 bytes
	redis_chunk chunks[MAX_CHUNKS];	//  3 x 10  bytes
	uint32_t buffer_size;		// 32 bytes
	char *buffer;			//  8 bytes, system dependent
} redis_client;				// 39 bytes



inline void proto_clear(redis_client *r){
	memset(r, 0, sizeof(*r));
}

static char *_datacat(char *dest, const char *src, uint32_t dest_size, uint32_t src_size){
	if (src_size == 0)
		return dest;

	uint32_t buff_size = dest_size + src_size;
	char *buff = realloc(dest, buff_size);

	if (buff == NULL)
		return dest;

	memcpy( & dest[ dest_size ], src, buff_size);

	return dest;
}

int proto_feed(redis_client *r, const char *data, uint32_t data_size){
	if (proto_isdone(r))
		return 1; // done already

	if (data_size <= 0)
		return 2; // no data

	uint64_t buff_size = r->buffer_size + data_size;

	if (buff_size > MAX_BUFFER_SIZE(32))
		return 3; // overflow

	char *buff = realloc(r->buffer, buff_size);

	if (buff == NULL)
		return 4; // out of memory

	memcpy( & buff[ r->buffer_size ], data, data_size);

	r->buffer = buff;
	r->buffer_size = buff_size;

	return 0;
}

int proto_isdone(redis_client *r){
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
		printf("%u | %5d | %s\n",
				i,
				r->chunks[i].size,
				r->chunks[i].size > 0 ?r->chunks[i].data : "(none)"
		);
	}
}

int main(){
	redis_client r;

	proto_clear(&r);

	proto_feed(&r, "Niki", 4);
	proto_feed(&r, " is ", 4);
	proto_feed(&r, "good", 4);
	proto_feed(&r, "\0", 1);

	printf("%s\n", r.buffer);

//	r.chunk_count = 1;
//	proto_dump(&r);

	return 0;
}

