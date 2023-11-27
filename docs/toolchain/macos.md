
# Cross compiler for macOS and Grub tools

## Homebrew
Need home brew to install the cross compiler and grub tools
```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

## Cross compiler i386-elf
```sh
brew tap nativeos/i386-elf-toolchain
brew install nativeos/i386-elf-toolchain/i386-elf-binutils -v
brew install nativeos/i386-elf-toolchain/i386-elf-gcc -v
```

## Depedencies
```sh
brew install autoconf-archive objconv pkg-config xorriso gawk
```

## Grub (for mkrescue)
```sh
git clone git://git.savannah.gnu.org/grub.git
cd grub
./bootstrap
sh autogen.sh
mkdir build
cd build
../configure --disable-werror TARGET_CC=i386-elf-gcc TARGET_OBJCOPY=i386-elf-objcopy TARGET_STRIP=i386-elf-strip TARGET_NM=i386-elf-nm TARGET_RANLIB=i386-elf-ranlib --target=i386-elf
```

## i386-pc location
/usr/local/lib/grub/i386-pc/
