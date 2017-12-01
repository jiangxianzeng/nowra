OBJECTS= vbuf.o vbuf_print.o  msg.o msg_output.o msg_filelog.o  mymalloc.o vstring.o ring.o events.o iostuff.o thpool.o inet_stuff.o argv.o stringop.o binhash.o

LIB = libutil.a

CFLAGS = -g

main:main.c $(LIB)
	gcc $(CFLAGS) -o main main.c $(LIB)  -lpthread

events:events.c $(LIB)
	gcc $(CFLAGS) -DTEST -o events events.c $(LIB)

$(LIB):$(OBJECTS)
	ar -r $(LIB) $(OBJECTS)

vbuf.o:vbuf.c
	gcc $(CFLAGS) -c vbuf.c

vbuf_print.o:vbuf_print.c
	gcc $(CFLAGS) -c vbuf_print.c

msg.o:msg.c
	gcc $(CFLAGS) -c msg.c

msg_output.o:msg_output.c
	gcc $(CFLAGS) -c msg_output.c

msg_filelog.o:msg_filelog.c
	gcc $(CFLAGS) -c msg_filelog.c

mymalloc.o:mymalloc.c
	gcc $(CFLAGS) -c mymalloc.c

vstring.o:vstring.c
	gcc $(CFLAGS)  -c vstring.c

ring.o:ring.c
	gcc $(CFLAGS) -c ring.c

events.o:events.c
	gcc $(CFLAGS) -c events.c

iostuff.o:iostuff.c
	gcc $(CFLAGS) -c iostuff.c

thpool.o:thpool.c
	gcc $(CFLAGS) -c thpool.c

inet_stuff.o:inet_stuff.c
	gcc $(CFLAGS) -c inet_stuff.c

argv.o:argv.c
	gcc $(CFLAGS) -c argv.c

stringop.o:stringop.c
	gcc $(CFLAGS) -c stringop.c

binhash.o:binhash.c
	gcc $(CFLAGS) -c binhash.c

clean:
	rm -f *.o libutil.a
