#include "utils.h"

void usage(char *progname, FILE *outstream)
{
	fprintf(outstream, "Usage: %s [OPTION]... FILE1 ...\n", progname);
	(void) fputs(	"Options:	-c	Conserve memory by reading input files twice\n"
			"		-f	Enable faster processing by keeping modules in memory (default)\n"
			"		-h	Print this help message\n", outstream);
}

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if(ptr == NULL) {
		perror("Error allocating memory ");
		exit(EXIT_FAILURE);
	} else
		return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if(ptr == NULL) {
		perror("Error allocating memory ");
		exit(EXIT_FAILURE);
	} else
		return ptr;
}

char *xstrdup(const char *str)
{
	char *new_str = NULL;
	size_t len = strlen(str);

	/* strdup is not ANSI C. We do its equivalent in this function */
	new_str = malloc(len + 1);
	if(new_str == (char*) NULL) {
		perror("Error allocating memory ");
		exit(EXIT_FAILURE);
	} else
		return memcpy(new_str, str, len + 1);
}

FILE *xfopen(const char *path, const char *mode)
{
	FILE *f = fopen(path, mode);

	if(f == (FILE*) NULL) {
		perror("Error opening file ");
		exit(EXIT_FAILURE);
	} else
		return f;
}

void __parseerror(int errcode, int l_num, int l_offset)
{
	static char* errstr[] = {
		"NUM_EXPECTED",			/* Number expect */
		"SYM_EXPECTED",			/* Symbol Expected */
		"ADDR_EXPECTED",		/* Addressing Expected */
		"SYM_TOLONG",			/* Symbol Name is to long */
		"TO_MANY_DEF_IN_MODULE",	/* .. */
		"TO_MANY_USE_IN_MODULE",
		"TO_MANY_SYMBOLS",
		"TO_MANY_INSTR",
	};
	printf("Parse Error line %d offset %d: %s\n", l_num, l_offset, errstr[errcode]);
	/*printf("Parse Error in %s line %d offset %d: %s\n", current_workfile, l_num, l_offset, errstr[errcode]); */
}

