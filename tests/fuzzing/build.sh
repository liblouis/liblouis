#!/bin/bash -eu
# Build Liblouis
./autogen.sh
./configure
make -j$(nproc) V=1

cd tests/fuzzing
cp ../tables/empty.ctb $OUT/
find ../.. -name "*.o" -exec ar rcs fuzz_lib.a {} \;
$CXX $CXXFLAGS -c table_fuzzer.cc -I/src/liblouis -o table_fuzzer.o
$CXX $CXXFLAGS $LIB_FUZZING_ENGINE table_fuzzer.o -o $OUT/table_fuzzer fuzz_lib.a

$CC $CFLAGS -c fuzz_translate_generic.c -o fuzz_translate_generic.o \
    -I$SRC/liblouis -I$SRC/liblouis/liblouis
$CXX $CXXFLAGS $LIB_FUZZING_ENGINE fuzz_translate_generic.o \
    -o $OUT/fuzz_translate_generic fuzz_lib.a

$CC $CFLAGS -c fuzz_backtranslate.c -o fuzz_backtranslate.o \
    -I$SRC/liblouis -I$SRC/liblouis/liblouis
$CXX $CXXFLAGS $LIB_FUZZING_ENGINE fuzz_backtranslate.o \
    -o $OUT/fuzz_backtranslate fuzz_lib.a

# Build corpus
zip $OUT/table_fuzzer_seed_corpus.zip $SRC/liblouis/tables/latinLetterDef6Dots.uti

# Copy out dictionary
cp $SRC/liblouis/tests/fuzzing/fuzz_translate_generic.dict $OUT/fuzz_translate_generic.dict
cp $SRC/liblouis/tests/fuzzing/fuzz_translate_generic.dict $OUT/fuzz_backtranslate.dict
