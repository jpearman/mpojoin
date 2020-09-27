UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CC = $(CROSS_COMPILE)gcc -arch i386 -arch x86_64  -mmacosx-version-min=10.6
else
CC = $(CROSS_COMPILE)gcc -mno-ms-bitfields
endif

export CC

all:
	$(CC) -Wall -o mpojoin -I./ \
		main.c
		
clean:
	rm -rf *.o
	rm mpojoin
	rm mpojoin.exe
