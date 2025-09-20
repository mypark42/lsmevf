SRC := subscriber.c
OBJ := $(SRC:.c=.o)


subscriber: $(OBJ)
	$(CC) -o $@ $(OBJ)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) -I../

clean:
	rm -f subscriber $(OBJ)

PHONY: clean subscriber
