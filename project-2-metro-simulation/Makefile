
CC = g++
CFLAGS = -Wall -Wextra -pthread

all: metro_simulation

metro_simulation: main.cpp
	$(CC) $(CFLAGS) main.cpp -o metro_simulation

clean:
	rm -f metro_simulation
	rm -f *.log