CXX = gcc
CXXFLAGS = -Wall -Og -ggdb
OBJS = disk_image.o

all: disk_image

disk_image: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.c
	$(CXX) $(CXXFLAGS)  -c -o $@ $<

clean:
	rm -f *.o disk_image

.PHONY: clean all