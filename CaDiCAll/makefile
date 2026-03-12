.PHONY: all src checker clean

all: src checker

src:
	$(MAKE) -C src

checker:
	$(MAKE) -C checker

clean:
	$(MAKE) -C src clean
	$(MAKE) -C checker clean
	rm -f -r ./tests/tmp/
	rm -f -r ./stats/tmp/
