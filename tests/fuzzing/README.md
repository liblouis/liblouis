# Translation fuzzers

There are 3 fuzzers related to translation: [`fuzz_translate`](fuzz_translate.c) and
[`fuzz_translate_generic`](fuzz_translate_generic.c) target `lou_translateString`,
[`fuzz_backtranslate`](fuzz_backtranslate.c) fuzzes `lou_backTranslateString`.

`fuzz_translate` is run continuously through a Github workflow. `fuzz_translate_generic`
and `fuzz_backtranslate` are run continously through OSS-fuzz.

# Table fuzzer

[`table_fuzzer`](table_fuzzer.cc) fuzzes the `compileFile` function. It is run continously
through OSS-fuzz.

# Configure Liblouis for fuzzing

`./configure --with-fuzzer` will check if your are using clang and clang++ as compilers
(by looking at `CC` and `CXX`) and enables generation of compilation instructions for the
fuzzer targets (translation fuzzers only). The `--with-coverage` switch will add
`-fprofile-instr-generate -fcoverage-mapping` to `AM_CPPFLAGS` (in
[liblouis/Makefile.am](../../liblouis/Makefile.am) and
[tests/fuzzing/Makefile.am](Makefile.am)).

To configure and build the project with fuzzer and coverage:

```sh
./autogen.sh
export CC=clang
export CXX=clang++
./configure --with-coverage --with-fuzzer
make -j8
```

Now you are able to run the fuzzers.

# Run `fuzz_translate`

fuzz_translate requires two parameters. With the `FUZZ_TABLE` environment variable you set
the table(s) to fuzz. An input corpus can optionally be provided via the first argument. A
corpus is a directory with files containing sample inputs that will be used by libfuzzer
to craft the data passed to the fuzzing function. The idea is to keep corpus as minimal as
possible. If you don't provide a corpus directory, libfuzzer generates random inputs.

Here is an example:

```sh
# move to tests/fuzzing directory
cd tests/fuzzing

# we assume here you have added corpus files into the "CORPUS" directory
export FUZZ_TABLE=../../tables/en-us-g2.ctb
./fuzz_translate CORPUS/

# run the fuzzer using parallelization
# you can also set more jobs than workers (stopped jobs will instantly be replaced by a new fuzzer process)
./fuzz_translate CORPUS/ -workers=8 -jobs=8

# to start the fuzzer and let it generate random input (without corpus files):
# here we don't parallelize the process
./fuzz_translate
```

After running the fuzzer multiple times with the same corpus, it might be possible that
many corpus files added by the fuzzer explore the same code paths. Hopefully, libfuzzer
allows you to minimize a corpus. There is a simple bash script that allows you to do that:

```sh
./minimize-corpus.sh CORPUS/
```

If you have added a PoC file in the corpus directory that you want to keep intact, change
its extension to .txt and use the `--preserve-txt` switch:

```sh
./minimize-corpus.sh --preserve-txt CORPUS/
```

# Look at fuzzer coverage

If you want to see what are the code parts that are explored by the fuzzer, you can use
clang coverage. Configure with the `--with-overage` switch, run the fuzzer and show
coverage data from the run with llvm tools. To be able to use the coverage data, you first
need to compile the raw profile data file of the run. By default, this file is created
after execution under the name of default.profraw but you can specify it with
`LLVM_PROFILE_FILE`. Here is how to do that:

```
export LLVM_PROFILE_FILE=fuzz_translate.profraw
export FUZZ_TABLE=../../tables/en-us-g2.ctb
./fuzz_translate CORPUS/ -workers=8 -jobs=8

# wait for a bit and press Ctrl+C

# compile raw profile
llvm-profdata merge -sparse fuzz_translate.profraw -o fuzz_translate.profdata

# show coverage (red lines are the one wich are reached)
llvm-cov show .libs/fuzz_translate -instr-profile=fuzz_translate.profdata
```

# Continous integration

## OSS-Fuzz

OSS-Fuzz compiles and runs [`table_fuzzer`](table_fuzzer.cc),
[`fuzz_translate_generic`](fuzz_translate_generic.c) and
[`fuzz_backtranslate`](fuzz_backtranslate.c) using the Dockerfile and build.sh script from
the [Liblouis OSS-Fuzz project
folder](https://github.com/google/oss-fuzz/blob/master/projects/liblouis). The Dockerfile
clones the Liblouis repository and build.sh invokes
[tests/fuzzing/build.sh](test/fuzzing/build.sh).

Reported issues can be found in Google's [Monorail issue
tracker](https://bugs.chromium.org/p/oss-fuzz/issues/list?q=label%3AProj-liblouis). Note
that some issues are only visible for the maintainers.

### Reproducing OSS-Fuzz issues

The [OSS-Fuzz
documenation](https://google.github.io/oss-fuzz/advanced-topics/reproducing/#building-using-docker)
explains how to reproduce OSS-Fuzz issues using the reproducer (or testcase) files and
Docker.

If you have configured Liblouis for fuzzing (with clang) you can also reproduce issues
natively:

```sh
export CC=clang
export CXX=clang++
# optional: enable 'address' and 'undefined' sanitizers in order to be able to reproduce ASAN and UBSAN crashes
export CFLAGS="-fsanitize=address,undefined"
./configure --with-fuzzer
make
tests/fuzzing/fuzz_translate_generic <path/to/testcase_file>
```

## Github workflows

### [`fuzzing.yml`](../../.github/workflows/fuzzing.yml)

Builds and runs `fuzz_translate` for every table with extenion .utb or .ctb, and uploads
possible crash PoC's as artifacts. This happens once a week. See [the list of previous
runs](https://github.com/liblouis/liblouis/actions/workflows/fuzzing.yml).

### [`cifuzz.yml`](../../.github/workflows/cifuzz.yml)

This will do the same as OSS-Fuzz, but on Github's infrastructure. The idea is to be able
to catch fuzzing issues in an earlier stage. Possible crash PoC's are uploaded as
artifacts. The workflow is triggered on pull requests. See [the list of previous
runs](https://github.com/liblouis/liblouis/actions/workflows/cifuzz.yml).
