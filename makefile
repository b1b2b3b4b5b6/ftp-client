bin/server: obj/server.obj bin/client
	gcc $< -o $@
bin/client: obj/client.obj
	gcc $^ -o $@
obj/%.obj: src/%.c
	gcc -c -g -Iinc $< -o $@

clean:
	-rm bin/*
	-rm obj/*
