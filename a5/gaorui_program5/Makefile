PROJECT_BIN		=.

CC          	= gcc
#CFLAGS      	= -Wall -g -O2 -fPIC

OBJ_KEYGEN    	=keygen.o
OBJ_ENC_SERVER  =enc_server.o
OBJ_ENC_CLIENT  =enc_client.o
OBJ_DEC_SERVER  =dec_server.o
OBJ_DEC_CLIENT  =dec_client.o
PROJECT     	=keygen enc_server dec_server enc_client dec_client

all:${PROJECT} clean

keygen:${OBJ_KEYGEN}
	${CC} ${CFLAGS} $^ -o ${PROJECT_BIN}/$@

enc_server:${OBJ_ENC_SERVER}
	${CC} ${CFLAGS} $^ -o ${PROJECT_BIN}/$@

dec_server:${OBJ_DEC_SERVER}
	${CC} ${CFLAGS} $^ -o ${PROJECT_BIN}/$@

enc_client:${OBJ_ENC_CLIENT}
	${CC} ${CFLAGS} $^ -o ${PROJECT_BIN}/$@

dec_client:${OBJ_DEC_CLIENT}
	${CC} ${CFLAGS} $^ -o ${PROJECT_BIN}/$@

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm -f *.o
