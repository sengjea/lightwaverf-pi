CFLAGS := -v -fPIC -O3 -g -Wall -Werror
CC := gcc
MAJOR := 0
MINOR := 1
NAME := lightwaverf
VERSION := $(MAJOR).$(MINOR)
PREFIX := /usr/local

all: lwrfsend

lib: lib$(NAME).so.$(VERSION)
 
samples: send receive send2 mqttsend lwrfmqtt
 
lwrfsend: $(NAME).c send.c
	$(CC) $(CFLAGS) $^ -o $@ -I. -lwiringPi
 
mqttsend: lib$(NAME).so mqttsend.c
	$(CC) mqttsend.c -o $@ -L. -l$(NAME) -lwiringPi -lpaho-mqtt3c
 
lwrfmqtt: lib$(NAME).so lwrfmqtt.c
	$(CC) lwrfmqtt.c -o $@ -L. -l$(NAME) -lwiringPi -lpaho-mqtt3c
 
receive: lib$(NAME).so receive.c
	$(CC) receive.c -o $@ -L. -l$(NAME) -lwiringPi

lib$(NAME).so: lib$(NAME).so.$(VERSION)
	ldconfig -v -n .
	ln -s lib$(NAME).so.$(MAJOR) lib$(NAME).so
 
lib$(NAME).so.$(VERSION): $(NAME).o
	$(CC) -shared -Wl,-soname,lib$(NAME).so.$(MAJOR) $^ -o $@
 
clean:
	$(RM) send send2 receive *.o *.so*

install: lib$(NAME).so
	install -m 0644 lib$(NAME).so.$(VERSION) $(PREFIX)/lib
	ldconfig
	ln -f -s $(PREFIX)/lib/lib$(NAME).so.$(MAJOR) $(PREFIX)/lib/lib$(NAME).so
	install -m 0644 *.h $(PREFIX)/include
	install -m 0755 lwrfmqtt ${PREFIX}/bin
	install -m 0755 lwrfd /etc/init.d
    
.PHONY: install
