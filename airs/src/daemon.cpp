/**
 * @file daemon.cpp 将软件转换为守护进程的几个函数
 * @date 2017-01-19
 * @version 0.2
 */

#include <syslog.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <boost/bind.hpp>

#include "daemon.h"

#define write_lock(fd, offset, whence, len) lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)
#define FILE_MODE (S_IRWXU | S_IRWXG | S_IRWXO)

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len) {
	struct flock lock;
	lock.l_type    = type;
	lock.l_start   = offset;
	lock.l_whence  = whence;
	lock.l_len     = len;
	return fcntl(fd, cmd, &lock);
}

bool isProcSingleton(const char* pidfile) {
	int fd, val;
	char buff[20];

	if ((fd = open(pidfile, O_WRONLY | O_CREAT, FILE_MODE)) < 0  // ROOT privilege is required
			|| write_lock(fd, 0, SEEK_SET, 0) < 0                // process is running or failed to lock PID file
			|| ftruncate(fd, 0) < 0)                             // failed to access PID file
		return false;
	sprintf(buff, "%d\n", getpid());
	if ((size_t) write(fd, buff, strlen(buff)) != strlen(buff))  // failed to access PID file
		return false;
	if ((val = fcntl(fd, F_GETFD, 0)) < 0)                       // F_GETFD failure
		return false;
	val |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, val) < 0)                             // F_SETFD failure
		return false;

	return true;
}

bool MakeItDaemon(boost::asio::io_service &io_service) {
	io_service.notify_fork(boost::asio::io_service::fork_prepare);
	if (pid_t pid = fork()) {
		if (pid > 0)
			exit(0);
		else {
			syslog(LOG_ERR | LOG_USER, "First fork failed: %m");
			return false;
		}
	}

	setsid();	// set new parent process
	chdir("/"); // change process path
	umask(0);   // disable file privilege inherited from parent process

	if (pid_t pid = fork()) {
		if (pid > 0)
			exit(0);
		else {
			syslog(LOG_ERR | LOG_USER, "Second fork failed: %m");
			return false;
		}
	}

	// close standard I/O
	close(0);
	close(1);
	close(2);

	if (open("/dev/null", O_RDONLY) < 0) {
		syslog(LOG_ERR | LOG_USER, "Unable to open /dev/null: %m");
		return false;
	}
	// has complete prepares for background process
	io_service.notify_fork(boost::asio::io_service::fork_child);

	return true;
}
