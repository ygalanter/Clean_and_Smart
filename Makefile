.PHONY: clean wipe build aplite emery basalt chalk diorite

clean:
	pebble clean

wipe:
	pebble wipe

build:
	pebble build

aplite: clean wipe build
	pebble install --emulator aplite

emery: clean wipe build
	bash docs/tools/emery-kill.sh
	pebble install --emulator emery

basalt: clean wipe build
	pebble install --emulator basalt

chalk: clean wipe build
	pebble install --emulator chalk

diorite: clean wipe build
	pebble install --emulator diorite
