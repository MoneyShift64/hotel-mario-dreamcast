#!/bin/sh
# Builds Ghidra's stand-alone console decompiler (decomp_opt) and the 68000 language
# files needed by decompile.py. Only needed if you want to re-run the decompile.
# Needs: git, make, a C++ compiler. Run from the hotel-mario folder.
set -e
git clone --depth 1 --filter=blob:none --sparse https://github.com/NationalSecurityAgency/ghidra tools/ghidra
cd tools/ghidra
git sparse-checkout set Ghidra/Features/Decompiler/src/decompile Ghidra/Processors/68000/data
cd Ghidra/Features/Decompiler/src/decompile/cpp
# leave out the parts that need GNU BFD (only used for loading PC executables)
sed -i.bak 's/^EXTRA= \$(filter-out \$(CORE)/EXTRA= $(filter-out analyzesigs codedata loadimage_bfd bfd_arch $(CORE)/; s/^BFDLIB=-lbfd/BFDLIB=/' Makefile
make -j8 sleigh_opt decomp_opt
cd ../../../../../Processors/68000/data/languages
../../../../Features/Decompiler/src/decompile/cpp/sleigh_opt 68040.slaspec 68040.sla
# Microware C calling convention: args in D0, D1 then stack; A6 (globals) preserved
cp ../../../../../../68000_microware.cspec 68000.cspec
echo "Done. Use:  DECOMP=tools/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/decomp_opt SLEIGHHOME=tools/ghidra python3 tools/decompile.py work src"
