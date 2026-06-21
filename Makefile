clean:
	rebble clean

wipe:
	rebble wipe

build:
	rebble build

aplite: clean wipe build
	rebble install --emulator aplite

emery: clean wipe build
	rebble kill --force 2>/dev/null || true
	rebble install --emulator emery --throttle 0.001

basalt: clean wipe build
	rebble install --emulator basalt

chalk: clean wipe build
	rebble install --emulator chalk

diorite: clean wipe build
	rebble install --emulator diorite



