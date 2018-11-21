all: thrwordcnt_3035233228.zip test

wordcnt0: wordcnt0.c
	gcc $^ -o $@ -Wall

thrwordcnt: thrwordcnt.c
	gcc -pthread $^ -o $@ -Wall

thrwordcnt_3035233228.c: thrwordcnt.c
	cp $^ $@

thrwordcnt_3035233228.zip: thrwordcnt_3035233228.c
	zip $@ $^

test: thrwordcnt
	./$^ 2 3 samples/test1.txt samples/key1.txt | tee samples/output1.txt \
	&& diff samples/output1.txt samples/reference1.txt 2>&1

clean:
	rm wordcnt0 thrwordcnt thrwordcnt_3035233228.c \
		thrwordcnt_3035233228.zip -rf

.PHONY: test clean
