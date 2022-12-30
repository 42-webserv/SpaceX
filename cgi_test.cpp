#include <fcntl.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// COMSPEC="C:\Windows\system32\cmd.exe"
// DOCUMENT_ROOT="C:/Program Files (x86)/Apache Software Foundation/Apache2.4/htdocs"
// GATEWAY_INTERFACE="CGI/1.1"
// HOME="/home/SYSTEM"
// HTTP_ACCEPT="text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"
// HTTP_ACCEPT_CHARSET="ISO-8859-1,utf-8;q=0.7,*;q=0.7"
// HTTP_ACCEPT_ENCODING="gzip, deflate, br"
// HTTP_ACCEPT_LANGUAGE="en-us,en;q=0.5"
// HTTP_CONNECTION="keep-alive"
// HTTP_HOST="example.com"
// HTTP_USER_AGENT="Mozilla/5.0 (Windows NT 6.1; WOW64; rv:67.0) Gecko/20100101 Firefox/67.0"
// PATH="/home/SYSTEM/bin:/bin:/cygdrive/c/progra~2/php:/cygdrive/c/windows/system32:..."
// PATHEXT=".COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC"
// PATH_TRANSLATED="C:\Program Files (x86)\Apache Software Foundation\Apache2.4\htdocs\foo\bar"
// QUERY_STRING="var1=value1&var2=with%20percent%20encoding"
// REMOTE_ADDR="127.0.0.1"
// REMOTE_PORT="63555"
// REQUEST_METHOD="GET"
// REQUEST_URI="/cgi-bin/printenv.pl/foo/bar?var1=value1&var2=with%20percent%20encoding"
// SCRIPT_FILENAME="C:/Program Files (x86)/Apache Software Foundation/Apache2.4/cgi-bin/printenv.pl"
// SCRIPT_NAME="/cgi-bin/printenv.pl"
// SERVER_ADDR="127.0.0.1"
// SERVER_ADMIN="(server admin's email address)"
// SERVER_NAME="127.0.0.1"
// SERVER_PORT="80"
// SERVER_SIGNATURE=""
// SERVER_SOFTWARE="Apache/2.4.39 (Win32) PHP/7.3.7"
// SYSTEMROOT="C:\Windows"
// TERM="cygwin"
// WINDIR="C:\Windows"

int
func(char*& aaa) {
	return sizeof(aaa);
}

int
main(void) {
	char* envp[10];
	char* args[3];

	args[0] = "./cgi_bin/42_cgi_tester";
	args[1] = ".bla";
	args[2] = NULL;

	envp[0] = "REQUEST_METHOD=GET";
	envp[1] = "SERVER_PROTOCOL=HTTP/1.1";
	envp[2] = "PATH_INFO=./cgi_bin";
	envp[3] = NULL;

	execve(args[0], args, envp);
}
