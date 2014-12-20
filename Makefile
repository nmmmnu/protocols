CC   = gcc -Wall -c
LINK = gcc -o



all: test_proto_redis



clean:
	rm -f *.o test_proto_redis



test_proto_redis: proto_redis.o test_proto_redis.o
	$(LINK) test_proto_redis proto_redis.o test_proto_redis.o



test_proto_redis.o: test_proto_redis.c proto.h
	$(CC) test_proto_redis.c

proto_redis.o: proto_redis.c proto.h
	$(CC) proto_redis.c
