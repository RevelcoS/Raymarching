LIBS="mingw-w64-ucrt-x86_64-gcc";
LIBS+=" make";
LIBS+=" git";
LIBS+=" rsync";

for lib in $LIBS
do
pacman -S $lib --noconfirm
done