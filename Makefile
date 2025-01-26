#gcc ECO_server.c -o ECO_server -lws2_32

#ECO_server.o: ECO_server.c ECO_common.h 

#ECO_chess: ECO_chess.c ECO_chess.h - gcc chess.c -o chess

#gcc ECO_chess2.c ECO_server.c -o ECO_server -lws2_32

ECO_server: ECO_chess.c ECO_server.c
	 gcc ECO_chess.c ECO_server.c -o ECO_server -lws2_32 
	
ECO_client: ECO_chess.c ECO_client.c
	 gcc ECO_chess.c ECO_client.c -o ECO_client -lws2_32 

#ECO_server: ECO_server.c 
#	gcc ECO_chess2.c ECO_server.c  -o ECO_server 

#ECO_client: ECO_client.c 
#	gcc ECO_chess2.c CO_client.c  -o ECO_client -lws2_32

all: ECO_server ECO_client