Hobby OS development
===================

This is, or will become, my hobby operating sytem. It doesn't do much yet, other than
boot. It as a full static module loader (with ELF module loading planned), and has working
paging/kernel memory management.

I am using clang to compile and the binutils suite to link the kernel. Clang can cross-compile
by default but you will need to compile binutils in a specific way to get it to output the proper 
binary for bare metal. At some point in the future I may provide a script that will setup the toolchain
for you.

Planned features

- Threading
- Processes
- Userspace (In Go!)
