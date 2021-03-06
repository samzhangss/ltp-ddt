/*
 * Copyright (c) 2010, Garrett Cooper.
 *
 * The clock_getcpuclockid() function may fail and return ESRCH if no process
 * can be found corresponding to the process specified by pid.
 *
 */

#define _XOPEN_SOURCE 600

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "posixtest.h"

int main(int argc, char *argv[])
{
#if !defined(_POSIX_CPUTIME) || _POSIX_CPUTIME == -1
	printf("_POSIX_CPUTIME unsupported\n");
	return PTS_UNSUPPORTED;
#else
	clockid_t clockid_1;

	if (clock_getcpuclockid(-2, &clockid_1) == 0) {
		printf("clock_getcpuclockid(-2, ..) succeeded unexpectedly\n");
		return PTS_UNRESOLVED;
	}
	printf("Test PASSED\n");
	return PTS_PASS;

#endif
}
