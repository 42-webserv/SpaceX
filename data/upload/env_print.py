import cgi
import os
import cgitb



print("Content-Type: text/html\r")
response = "<html><body><center>"
response += "<h1> env list printer <h1>"
for name, value in os.environ.items():
    response += "<p> {0}: {1}</p>\n".format(name, value)


print("\r")
print(response)

exit()
