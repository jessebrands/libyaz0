CFLAGS      += -std=c99 -Wall -Werror -pedantic -O3
OBJECTS     := yaz0.o stream.o inflate.o
LIB_MAJOR   = 0
LIB_MINOR   = 0
LIB_NAME    := libyaz0.so.$(LIB_MAJOR).$(LIB_MINOR)

all: $(LIB_NAME)

$(LIB_NAME): $(OBJECTS)
	$(CC) $(CFLAGS) -shared -Wl,-soname=$@ -o $@ $^

$(OBJECTS): %.o: src/%.c
	$(CC) $(CFLAGS) -fPIC -o $@ -c $^

.PHONY: clean 

clean: 
	@ rm -f *.o
	@ rm -f $(LIB_NAME)

