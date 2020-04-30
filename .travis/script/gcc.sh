./autogen.sh &&
./configure $ENABLE_UCS4 --with-yaml &&
make &&
make check &&
# check display names
if [[ -n ${ENABLE_UCS4+x} ]]; then
    cd extra/generate-display-names &&
    if ! make; then
        cat generate.log
        false
    fi
fi
