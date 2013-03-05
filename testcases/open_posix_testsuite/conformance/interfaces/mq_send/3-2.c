/*
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.
 */

/*
 * Test that mq_send() inserts messages into the message queue according
 * to the priority given.  Specifically, test that messages with equal
 * priority values are placed after previously sent messages.
 *
 * Test similar to 3-1.c; however, messages 3 and 4 will have equal priority
 * order, but 3 will be sent first.
 *
 * 3/13/03 - Added fix from Gregoire Pichon for specifying an attr
 *           with a mq_maxmsg >= BUFFER.
 */

#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "posixtest.h"

#define NAMESIZE 50
#define MSG1 "1234567890"
#define PRI1 10
#define MSG2 "234567890"
#define PRI2 8
#define MSG3 "34567890"
#define PRI3 6
#define MSG4 "4567890"
#define PRI4 6
#define MSG5 "567890"
#define PRI5 1
#define BUFFER 40
#define MAXMSG 10

int main()
{
	char qname[NAMESIZE], msgrcd[BUFFER];
	const char *msgptr1 = MSG1;
	const char *msgptr2 = MSG2;
	const char *msgptr3 = MSG3;
	const char *msgptr4 = MSG4;
	const char *msgptr5 = MSG5;
	mqd_t queue;
	struct mq_attr attr;
	int unresolved = 0, failure = 0;
	unsigned pri;

	sprintf(qname, "/mq_send_3-2_%d", getpid());

	attr.mq_msgsize = BUFFER;
	attr.mq_maxmsg = MAXMSG;
	queue = mq_open(qname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
	if (queue == (mqd_t) - 1) {
		perror("mq_open() did not return success");
		return PTS_UNRESOLVED;
	}

	if (mq_send(queue, msgptr3, strlen(msgptr3), PRI3) != 0) {
		perror("mq_send() did not return success");
		printf("error sending %s\n", msgptr3);
		failure = 1;
	}

	if (mq_send(queue, msgptr1, strlen(msgptr1), PRI1) != 0) {
		perror("mq_send() did not return success");
		printf("error sending %s\n", msgptr1);
		failure = 1;
	}

	if (mq_send(queue, msgptr4, strlen(msgptr4), PRI4) != 0) {
		perror("mq_send() did not return success");
		printf("error sending %s\n", msgptr4);
		failure = 1;
	}

	if (mq_send(queue, msgptr2, strlen(msgptr2), PRI2) != 0) {
		perror("mq_send() did not return success");
		printf("error sending %s\n", msgptr2);
		failure = 1;
	}

	if (mq_send(queue, msgptr5, strlen(msgptr5), PRI5) != 0) {
		perror("mq_send() did not return success");
		printf("error sending %s\n", msgptr5);
		failure = 1;
	}

	if (mq_receive(queue, msgrcd, BUFFER, &pri) == -1) {
		perror("mq_receive() returned failure");
		unresolved = 1;
	}

	if (strncmp(msgptr1, msgrcd, strlen(msgptr1)) != 0) {
		printf("FAIL:  sent %s received %s\n", msgptr1, msgrcd);
		failure = 1;
	}

	if (mq_receive(queue, msgrcd, BUFFER, &pri) == -1) {
		perror("mq_receive() returned failure");
		unresolved = 1;
	}

	if (strncmp(msgptr2, msgrcd, strlen(msgptr2)) != 0) {
		printf("FAIL:  sent %s received %s\n", msgptr2, msgrcd);
		failure = 1;
	}

	if (mq_receive(queue, msgrcd, BUFFER, &pri) == -1) {
		perror("mq_receive() returned failure");
		unresolved = 1;
	}

	if (strncmp(msgptr3, msgrcd, strlen(msgptr3)) != 0) {
		printf("FAIL:  sent %s received %s\n", msgptr3, msgrcd);
		failure = 1;
	}

	if (mq_receive(queue, msgrcd, BUFFER, &pri) == -1) {
		perror("mq_receive() returned failure");
		unresolved = 1;
	}

	if (strncmp(msgptr4, msgrcd, strlen(msgptr4)) != 0) {
		printf("FAIL:  sent %s received %s\n", msgptr4, msgrcd);
		failure = 1;
	}

	if (mq_receive(queue, msgrcd, BUFFER, &pri) == -1) {
		perror("mq_receive() returned failure");
		unresolved = 1;
	}

	if (strncmp(msgptr5, msgrcd, strlen(msgptr5)) != 0) {
		printf("FAIL:  sent %s received %s\n", msgptr5, msgrcd);
		failure = 1;
	}

	if (mq_close(queue) != 0) {
		perror("mq_close() did not return success");
		unresolved = 1;
	}

	if (mq_unlink(qname) != 0) {
		perror("mq_unlink() did not return success");
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
