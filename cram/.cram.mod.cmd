savedcmd_cram.mod := printf '%s\n'   cram.o cram_dev.o | awk '!x[$$0]++ { print("./"$$0) }' > cram.mod
