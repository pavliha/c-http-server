.PHONY: configure build dev test clean run kill

configure:
	cmake --preset=dev

build:
	cmake --build build

kill:
	@pkill -9 c-http-server 2>/dev/null || true
	@sleep 0.5

dev: kill build
	cd build && cmake --build . --target dev

test:
	cd build && cmake --build . --target run_tests

clean:
	rm -rf build

run: configure dev

all: configure build
