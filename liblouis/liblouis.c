#include <stdlib.h>
#include <stdio.h>
#include "internal.h"
#include "liblouis-impl.h"

static graal_isolatethread_t* init_thread(void) {
	static graal_create_isolate_params_t isolate_params;
	static graal_isolate_t* isolate;
	static graal_isolatethread_t* isolatethread;
	if (isolatethread == NULL
	    && graal_create_isolate(&isolate_params, &isolate, &isolatethread) != 0) {
		fprintf(stderr, "Failed to create isolate. Exiting the program....\n");
			exit(1);
	}
	return isolatethread;
}

char *EXPORT_CALL
_lou_showDots(widechar const *dots, int length) {
	static char scratchBuf[MAXSTRING];
	lou_utils_showDots(init_thread(), dots, length, scratchBuf);
	return scratchBuf;
}

const intCharTupple *var_dotMapping() {
	return dotMapping;
}
