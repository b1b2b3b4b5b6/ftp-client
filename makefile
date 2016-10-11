TARGET=client

bin/$(TARGET): obj/client.o obj/tcp.o
	gcc -pthread  $^ -o $@
obj/%.o: src/%.c
	gcc  -g -pthread -c -Iinc  $< -o $@

clean:
	-rm obj/*
	-rm bin/*
