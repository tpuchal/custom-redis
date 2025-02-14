CC = gcc

CFLAGS = -Wall -g

TARGET = server

SRCS = server.c uint8tvector.c connection.c connectionvector.c pollfdvector.c 

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)