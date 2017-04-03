echo $(pwd);

if [ $0 != "32" -a $0 != "16" ]; then
	echo "argument 0 must either be 32 for UTF32 builts or 16 for UTF16 builds"
	exit 1
fi

if [ -z $1 ]; then
	echo "argument 1 must be a valid filename"
	exit 1
fi

emcc ./liblouis/.libs/liblouis.a -s RESERVED_FUNCTION_POINTERS=1\
 -s EXPORTED_FUNCTIONS="['_lou_version', '_lou_translateString', '_lou_translate',\
'_lou_backTranslateString', '_lou_backTranslate', '_lou_hyphenate',\
'_lou_compileString', '_lou_getTypeformForEmphClass', '_lou_dotsToChar',\
'_lou_charToDots', '_lou_registerLogCallback', '_lou_setLogLevel',\
'_lou_logFile', '_lou_logPrint', '_lou_logEnd', '_lou_setDataPath',\
'_lou_getDataPath', '_lou_getTable', '_lou_checkTable',\
'_lou_readCharFromFile', '_lou_free', '_lou_charSize']" -s MODULARIZE=1\
 -s EXPORT_NAME="'liblouisBuild'" -s EXTRA_EXPORTED_RUNTIME_METHODS="['FS',\
'Runtime', 'stringToUTF${0}', 'Pointer_Stringify']" --pre-js ./liblouis-js/inc/pre.js\
 --post-js ./liblouis-js/inc/post.js ${2} -o ./out/${1} &&

cat ./liblouis-js/inc/append.js >> ./out/${1}
