OBJECTS=../master/single_server.o ../master/master_proto.o 

LIB = ../util/libutil.a
INCLUDE = ../util/ 

smtpd:smtpd.c $(OBJECTS)
	gcc -g -o smtpd smtpd.c -I $(INCLUDE)  -I ../master/ $(OBJECTS) $(LIB)

clean:
	rm -f smtpd 
