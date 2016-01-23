IP = localhost
Port = 5000
Total_time = 120
Nthreads = 2
Mode = random 
Sleep_time = 0

multi-client: multi-client.c
	gcc -pthread multi-client.c -o multi-client
	./multi-client $(IP) $(Port) $(Nthreads) $(Total_time) $(Sleep_time) $(Mode)


server: server-mp.c
	gcc server-mp.c -o server
	./server $(Port)

clean:
	@rm -f multi-client server 
