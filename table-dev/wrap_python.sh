CURDIR=$(cd $(dirname "$0") && pwd)
LD_LIBRARY_PATH=$CURDIR/../liblouis/.libs:$CURDIR/.libs \
    PYTHONPATH=$CURDIR/../python:$PYTHONPATH \
    LOUIS_TABLEPATH=$CURDIR:$CURDIR/../tables \
    python "$@"
