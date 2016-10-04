bin/portscan: obj/portscan.o
	gcc -pthread $^ -o $@
obj/%.o: src/%.c
	gcc -g -Iinc -c -pthread $< -o $@
clean:
	-rm bin/*
	-rm obj/*
