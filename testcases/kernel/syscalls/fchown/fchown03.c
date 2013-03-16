/*
 *
 *   Copyright (c) International Business Machines  Corp., 2001
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * Test Name: fchown03
 *
 * Test Description:
 *  Verify that, fchown(2) succeeds to change the group of a file specified
 *  by path when called by non-root user with the following constraints,
 *	- euid of the process is equal to the owner of the file.
 *	- the intended gid is either egid, or one of the supplementary gids
 *	  of the process.
 *  Also, verify that fchown() clears the setuid/setgid bits set on the file.
 *
 * Expected Result:
 *  fchown(2) should return 0 and the ownership set on the file should match
 *  the numeric values contained in owner and group respectively.
 *
 * Algorithm:
 *  Setup:
 *   Setup signal handling.
 *   Create temporary directory.
 *   Pause for SIGUSR1 if option specified.
 *
 *  Test:
 *   Loop if the proper options are given.
 *   Execute system call
 *   Check return code, if system call failed (return=-1)
 *	Log the errno and Issue a FAIL message.
 *   Otherwise,
 *	Verify the Functionality of system call
 *      if successful,
 *		Issue Functionality-Pass message.
 *      Otherwise,
 *		Issue Functionality-Fail message.
 *  Cleanup:
 *   Print errno log and/or timing stats if options given
 *   Delete the temporary directory created.
 *
 * Usage:  <for command-line>
 *  fchown03 [-c n] [-f] [-i n] [-I x] [-P x] [-t]
 *     where,  -c n : Run n copies concurrently.
 *             -f   : Turn off functionality Testing.
 *	       -i n : Execute test n times.
 *	       -I x : Execute test for x seconds.
 *	       -P x : Pause for x seconds between iterations.
 *	       -t   : Turn on syscall timing.
 *
 * HISTORY
 *	07/2001 Ported by Wayne Boyer
 *
 * RESTRICTIONS:
 *  This test should be run by 'non-super-user' (uid != 0) only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>

#include "test.h"
#include "usctest.h"

#define FILE_MODE (mode_t)(S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define NEW_PERMS (mode_t)(S_IFREG | S_IRWXU | S_IRWXG | S_ISUID | S_ISGID)
#define FCHOWN_PERMS	(mode_t)(NEW_PERMS & ~(S_ISUID | S_ISGID))
#define TESTFILE	"testfile"

int fildes;			/* File descriptor for test file */
char *TCID = "fchown03";	/* Test program identifier. */
int TST_TOTAL = 1;		/* Total number of test conditions */
char nobody_uid[] = "nobody";
struct passwd *ltpuser;

void setup();			/* setup function for the test */
void cleanup();			/* cleanup function for the test */

int main(int ac, char **av)
{
	struct stat stat_buf;	/* stat(2) struct contents */
	int lc;
	char *msg;
	uid_t user_id;		/* Owner id of the test file. */
	gid_t group_id;		/* Group id of the test file. */

	/* Parse standard options given to run the test. */
	msg = parse_opts(ac, av, NULL, NULL);
	if (msg != NULL) {
		tst_brkm(TBROK, NULL, "OPTION PARSING ERROR - %s", msg);

	}

	setup();

	for (lc = 0; TEST_LOOPING(lc); lc++) {

		Tst_count = 0;

		/* Get the euid/egid of the process */
		user_id = geteuid();
		group_id = getegid();

		/*
		 * Call fchwon(2) with different user id and
		 * group id (numeric values) to set it on
		 * testfile.
		 */
		TEST(fchown(fildes, -1, group_id));

		if (TEST_RETURN == -1) {
			tst_resm(TFAIL, "fchown() on %s Fails, errno=%d",
				 TESTFILE, TEST_ERRNO);
			continue;
		}
		/*
		 * Perform functional verification if test
		 * executed without (-f) option.
		 */
		if (STD_FUNCTIONAL_TEST) {
			/*
			 * Get the testfile information using
			 * fstat(2).
			 */
			if (fstat(fildes, &stat_buf) < 0) {
				tst_brkm(TFAIL, cleanup, "fstat(2) of %s "
					 "failed, errno:%d",
					 TESTFILE, TEST_ERRNO);
			}

			/*
			 * Check for expected Ownership ids
			 * set on testfile.
			 */
			if ((stat_buf.st_uid != user_id) ||
			    (stat_buf.st_gid != group_id)) {
				tst_resm(TFAIL, "%s: Incorrect "
					 "ownership set, Expected %d %d",
					 TESTFILE, user_id, group_id);
			}

			/*
			 * Verify that setuid/setgid bits
			 * set on the testfile in setup() are
			 * cleared by fchown()
			 */
			if (stat_buf.st_mode != FCHOWN_PERMS) {
				tst_resm(TFAIL, "%s: Incorrect mode permissions"
					 " %#o, Expected %#o", TESTFILE,
					 stat_buf.st_mode, FCHOWN_PERMS);
			} else {
				tst_resm(TPASS, "fchown() on %s succeeds: "
					 "Setuid/gid bits cleared", TESTFILE);
			}
		} else {
			tst_resm(TPASS, "call succeeded");
		}
	}

	cleanup();

	return (0);
}

/*
 * setup() - performs all ONE TIME setup for this test.
 *	     Create a temporary directory and change directory to it.
 *	     Create a test file under temporary directory and close it
 *	     Change the ownership on testfile.
 */
void setup()
{

	tst_sig(FORK, DEF_HANDLER, cleanup);

	/* Switch to nobody user for correct error code collection */
	if (geteuid() != 0) {
		tst_brkm(TBROK, NULL, "Test must be run as root");
	}
	ltpuser = getpwnam(nobody_uid);
	if (seteuid(ltpuser->pw_uid) == -1) {
		tst_brkm(TBROK, cleanup, "seteuid failed to "
			 "to set the effective uid to %d: %s", ltpuser->pw_uid,
			 strerror(errno));
	}

	TEST_PAUSE;

	tst_tmpdir();

	/* Create a test file under temporary directory */
	if ((fildes = open(TESTFILE, O_RDWR | O_CREAT, FILE_MODE)) == -1) {
		tst_brkm(TBROK, cleanup,
			 "open(%s, O_RDWR|O_CREAT, %o) Failed, errno=%d : %s",
			 TESTFILE, FILE_MODE, errno, strerror(errno));
	}
	seteuid(0);
	if (fchown(fildes, -1, 0) < 0)
		tst_brkm(TBROK, cleanup, "Fail to modify Ownership of %s: %s",
			 TESTFILE, strerror(errno));

	if (fchmod(fildes, NEW_PERMS) < 0)
		tst_brkm(TBROK, cleanup, "Fail to modify Mode of %s: %s",
			 TESTFILE, strerror(errno));

	setegid(ltpuser->pw_gid);
	seteuid(ltpuser->pw_uid);

}

/*
 * cleanup() - performs all ONE TIME cleanup for this test at
 *	       completion or premature exit.
 *	       Close the temporary file.
 *	       Remove the test directory and testfile created in the setup.
 */
void cleanup()
{
	/*
	 * print timing stats if that option was specified.
	 */
	TEST_CLEANUP;

	/* Close the test file created above */
	if (close(fildes) == -1) {
		tst_resm(TBROK, "close(%s) Failed, errno=%d : %s",
			 TESTFILE, errno, strerror(errno));
	}

	tst_rmdir();

}
