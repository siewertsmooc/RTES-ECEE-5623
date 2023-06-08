#include <syslog.h>
#include <sys/time.h>

// Note that all syslog output goes to /var/log/syslog
//
// To view the output as your RT applicaiton is running, you can just tail the log as follows:
//
// tail -f /var/log/syslog
//
// tail -f /var/log/messages on some systems depending on configuration
//
// Syslog is more efficient than printf and causes less potential for "blocking" in the middle of your
// service execution.  Please use it instead of printf and use it because it is simpler and easier to use
// than ftrace (although ftrace is ideal, it is harder to learn and master).
//
// https://elinux.org/Ftrace
//
// So, syslog is a nice compromise between efficent software-in-circuit tracing and printf.


int main(void)
{
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *)0);
    syslog(LOG_CRIT, "My log message test @ tv.tv_sec %ld, tv.tv_usec %ld\n", tv.tv_sec, tv.tv_usec);
    gettimeofday(&tv, (struct timezone *)0);
    syslog(LOG_CRIT, "My log message test @ tv.tv_sec %ld, tv.tv_usec %ld\n", tv.tv_sec, tv.tv_usec);
}
