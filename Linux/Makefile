ECO_server.exe: ECO_chess.c ECO_server.c
	 gcc ECO_chess.c payloadFormating.c ECO_server.c -o ECO_server.exe
	
ECO_client.exe: ECO_chess.c ECO_client.c
	 gcc ECO_chess.c ECO_client.c payloadFormating.c -o ECO_client.exe

all: ECO_server.exe ECO_client.exe

clean: 
	rm *.exe

.PHONY: clean
