/*
 * Copyright (c) 2003, Intel Corporation. All rights reserved.
 * Created by:  crystal.xiong REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.
 */

/*
 * mq_timedreceive test plan:
 * If message can be removed from message queue immediately,
 * mq_timedreceive will never fail with a timeout.
 */

#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "posixtest.h"

#define TEST "10-1"
#define FUNCTION "mq_timedreceive"
#define ERROR_PREFIX "unexpected error: " FUNCTION " " TEST ": "

#define NAMESIZE 50
#define BUFFER 40

int main()
{
	char mqname[NAMESIZE], msgrv[BUFFER];
	const char *msgptr = "test message";
	mqd_t mqdes;
	unsigned rvprio, sdprio = 1;
	struct timespec ts;
	struct mq_attr attr;
	int unresolved = 0, failure = 0;

	sprintf(mqname, "/" FUNCTION "_" TEST "_%d", getpid());

	attr.mq_msgsize = BUFFER;
	attr.mq_maxmsg = BUFFER;
	mqdes = mq_open(mqname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
	if (mqdes == (mqd_t) - 1) {
		perror(ERROR_PREFIX "mq_open");
		unresolved = 1;
	}

	if (mq_send(mqdes, msgptr, strlen(msgptr), sdprio) != 0) {
		perror(ERROR_PREFIX "mq_send");
		unresolved = 1;
	}

	ts.tv_sec = time(NULL) - 1;
	ts.tv_nsec = 0;
	if (mq_timedreceive(mqdes, msgrv, BUFFER, &rvprio, &ts) == -1) {
		perror("FAIL: mq_receive fails unexpectly\n");
		failure = 1;
	}

	if (mq_close(mqdes) != 0) {
		perror(ERROR_PREFIX "mq_close");
		unresolved = 1;
	}

	if (mq_unlink(mqname) != 0) {
		perror(ERROR_PREFIX "mq_unlink");
		unresolved = 1;
	}

	if (failure == 1) {
		printf("Test FAILED\n");
		return PTS_FAIL;
	}

	if (unresolved == 1) {
		printf("Test UNRESOLVED\n");
		return PTS_UNRESOLVED;
	}

	printf("Test PASSED\n");
	return PTS_PASS;
}
