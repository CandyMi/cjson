.PHONY : build

default :
	@echo "======================================="
	@echo "Please use 'make build' command to build it.."
	@echo "======================================="

CFLAGS = -O2 -Wall -shared -fPIC
CC = cc

INCLUDES = -I. -I../../../src -I../../inc
LIBS = -L../ -L../../ -L../../../
DLL = -lcore

build:
	@$(CC) -o ljson.so decoder.c encoder.c $(INCLUDES) $(LIBS) $(CFLAGS) $(DLL)
	@mv *.so ../../
