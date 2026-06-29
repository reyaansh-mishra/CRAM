savedcmd_cram.o := ld.lld -m elf_x86_64 --fatal-warnings -z noexecstack   -r -o cram.o @cram.mod  ; /home/reyaansh/Github/Linux/tools/objtool/objtool --hacks=jump_label --hacks=noinstr --hacks=skylake --ibt --prefix=16 --orc --retpoline --rethunk --static-call --uaccess  --link  --module cram.o

cram.o: $(wildcard /home/reyaansh/Github/Linux/tools/objtool/objtool)
