/* This file contains a unistd.h replacement for all 
	* that is needed for lou_compare to work in windows.
	* It is used so that the code in lou_compare is cleaner.
	* Please add any more functionality here for future development.
	* Author Paul Wood (Torch Trust)
*/


#include <io.h>
#define write _write
#define open _open
#define close _close
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
