#include "proto.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void _test_redis(proto_client_t *r, const char *test){
	printf("Test #2 - incremental add\n");

	size_t size = strlen(test);
	char btest[size + 1];

	int result;
	unsigned char i;
	for(i = 0; i < size; i++){
		btest[i] = test[i];
		btest[i + 1] = '\0';

		result = proto_parse(r, btest, strlen(btest));

		printf("%3u | %5d\n", i, result);

		if (result > 0){
			printf("Test FAIL!!!\n");
			printf("Buffer: %s\n", btest);
			printf("Last Error: %s\n", proto_error());
			return;
		}
	}

	printf("proto_parse = %d\n", result);

	proto_dump(r);

	printf("OK\n");
}

static void _test_redis_resp(proto_response_buffer_t *resp){
	if (resp == NULL)
		return;

	const char *mask = "begin>>%.*s<<end\n\n";
	printf(mask, resp->data_size, resp->data);
	free(resp);
}

int main(){
	proto_client_t r_placeholder;
	proto_client_t *r = & r_placeholder;

	_test_redis(r, "*2\r\n$3\r\nGET\r\n$2\r\nBG\r\n");
	_test_redis(r, "*3\r\n$3\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");

//	_test_redis(r, "*3\r\n$2\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
//	_test_redis(r, "*2\r\n$5\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
//	_test_redis(r, "*42\r\n$5\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");
//	_test_redis(r, "*\r\n$5\r\nSET\r\n$2\r\nBG\r\n$5\r\nSofia\r\n");

	_test_redis_resp(proto_response(PROTO_RESPONSE_OK,		NULL, 0));
	_test_redis_resp(proto_response(PROTO_RESPONSE_ERR,		NULL, 0));
	_test_redis_resp(proto_response(PROTO_RESPONSE_NOT_FOUND,	NULL, 0));

	_test_redis_resp(proto_response(PROTO_RESPONSE_VALUE,		"hello", 5));
	_test_redis_resp(proto_response(PROTO_RESPONSE_VALUE,		"sir", 3));

	return 0;
}

