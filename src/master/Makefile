OBJECTS=master_ent.o master_conf.o master_service.o master_sig.o master_listen.o master_status.o  master_spawn.o master_proto.o master_avail.o master_vars.o single_server.o 

LIB = ../util/libutil.a
INCLUDE = ../util/

master:master.c $(OBJECTS) 
	gcc -o master master.c $(OBJECTS) $(LIB) -I $(INCLUDE)

master_ent.o:master_ent.c 
	gcc -c master_ent.c  -I $(INCLUDE)

master_conf.o:master_conf.c 
	gcc -c master_conf.c  -I $(INCLUDE)

master_service.o:master_service.c 
	gcc -c master_service.c  -I $(INCLUDE)

master_sig.o:master_sig.c 
	gcc -c master_sig.c  -I $(INCLUDE)

master_listen.o:master_listen.c 
	gcc -c master_listen.c  -I $(INCLUDE)

master_status.o:master_status.c 
	gcc -c master_status.c  -I $(INCLUDE)

master_spawn.o:master_spawn.c 
	gcc -c master_spawn.c  -I $(INCLUDE)

master_proto.o:master_proto.c 
	gcc -c master_proto.c  -I $(INCLUDE)

master_avail.o:master_avail.c 
	gcc -c master_avail.c  -I $(INCLUDE)

master_vars.o:master_vars.c 
	gcc -c master_vars.c  -I $(INCLUDE)

single_server.o:single_server.c 
	gcc -c single_server.c  -I $(INCLUDE)
#cp single_server.o master_proto.o ../smtpd/


clean:
	rm -f *.o

