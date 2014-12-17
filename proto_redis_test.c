#include "proto_redis.h"

#include <stdio.h>
#include <string.h>

static void _proto_test_redis(proto_client *r, const char *test){
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

