all: bin build bin/x64_compiler bin/front


ReverseLang:
	git clone https://github.com/foxidokun/ReverseLang

compile_revlang: bin ReverseLang
	cd ReverseLang && \
	make all && \
	cd .. && \
	cp ReverseLang/bin/{middle,front} ./bin/

bin/front: ReverseLang compile_revlang
bin/middle: ReverseLang compile_revlang
bin/back: compile_current

compile_current: src/asm_stdlib/stdlib.out bin/x64_compiler

bin/x64_compiler: build src/asm_stdlib/stdlib.out
	cd build 			&& \
    cmake -GNinja ..	&& \
    ninja all			&& \
    cd ..				&& \
    cp build/x64_compiler bin/

src/asm_stdlib/stdlib.out: src/asm_stdlib/stdlib.nasm
	cd src/asm_stdlib && \
	nasm -f elf64 stdlib.nasm && \
    ld -e safety_entry -s -S stdlib.o -o stdlib.out

bin:
	mkdir -p bin

build:
	mkdir -p build