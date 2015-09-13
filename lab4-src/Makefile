CXX = g++ -g
NETLIBS= -lnsl -lpthread 

all: daytime-server daytime-client http-server git

daytime-client : daytime-client.o
	$(CXX) -o $@ $@.o $(NETLIBS)

daytime-server : daytime-server.o
	$(CXX) -o $@ $@.o $(NETLIBS)

http-server : http-server.o
	$(CXX) -o $@ $@.o $(NETLIBS)

%.o: %.cc
	@echo 'Building $@ from $<'
	$(CXX) -o $@ -c -I. $<

git: 
	git add Makefile *.c *.cc *.h  *.c 2>&1 >> .local.git.out || echo	
	git commit -a -m "Commit HTTP Server" 2>&1  >> .local.git.out || echo

clean:
	rm -f *.o use-dlopen hello.so
	rm -f *.o daytime-server daytime-client http-server


