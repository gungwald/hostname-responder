.PHONY: ada c service install clean


all: ada service


ada:
	$(MAKE) -C ada/src


c:
	$(MAKE) -C c


service:
	$(MAKE) -C service



install: all
	$(MAKE) -C ada/src install
	$(MAKE) -C service install



clean:
	$(MAKE) -C ada/src clean
	$(MAKE) -C c clean
