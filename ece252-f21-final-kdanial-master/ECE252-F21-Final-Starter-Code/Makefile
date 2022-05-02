CC := gcc
CFLAGS := -std=gnu99 -O0 -g
PTHREAD_FLAGS := -pthread -lm
BSD_FLAG := -D _DEFAULT_SOURCE

all: q3 q4 q5

q3: q3/pagerank

q4: q4/taxidcheck

q5: q5/asyncsocket

q3/pagerank: q3/pagerank.c q3/q3helper.c
	@printf "Compiling Q3: Page Rank\n"
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) -o $@ q3/pagerank.c q3/q3helper.c

q4/taxidcheck: q4/taxidcheck.c
	@printf "Compiling Q4: Tax ID Check\n"
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) -o $@ q4/taxidcheck.c

q5/asyncsocket: q5/asyncsocket.c
	@printf "Compiling Q5: Async Socket\n"
	$(CC) $(CFLAGS) -o $@ q5/asyncsocket.c 
	
clean:
	$(RM) q3/pagerank
	$(RM) q3/ranks.txt
	$(RM) q4/taxidcheck
	$(RM) q5/asyncsocket
	

.PHONY: all q3 q4 q5
