CC   = gcc -Wall -c
LINK = gcc -o



all: proto_redis_test



clean:
	rm -f *.o proto_redis_test



proto_redis_test: proto_redis.o proto_redis_test.o
	$(LINK) proto_redis_test proto_redis.o proto_redis_test.o


proto_redis_test.o: proto_redis_test.c proto_redis.h
	$(CC) proto_redis_test.c

proto_redis.o: proto_redis.c proto_redis.h
	$(CC) proto_redis.c

