clean:
	rebble clean

wipe:
	rebble wipe

build:
	rebble build

aplite: clean wipe build
	rebble install --emulator aplite

emery: clean wipe build
	rebble install --emulator emery

basalt: clean wipe build
	rebble install --emulator basalt

chalk: clean wipe build
	rebble install --emulator chalk

diorite: clean wipe build
	rebble install --emulator diorite



