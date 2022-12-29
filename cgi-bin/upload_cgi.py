#!/usr/bin/python
import cgi, os
import cgitb; cgitb.enable()
# import cgitb; cgitb.enable(display=0, logdir="./log")


print("Content-Type: text/html")
# Parse the form data
form = cgi.FieldStorage()
# Extract the uploaded files
files = {}
message = "file uploaded failed"
for field in form.keys():
	message = "file uploaded success"
	# Check if the field is a file field
	if isinstance(form[field], cgi.FieldStorage) and form[field].filename:
		# Save the file to the specified location
		with open("./tmp/" + form[field].filename, 'wb') as f:
			f.write(form[field].file.read())
		files[field] = form[field].filename

# Print the uploaded files
response = "<html><body><center>"
response += "<h1>{}<h1>".format(message)
for field, filename in files.items():
    response += "<p>{}: {}</p>\n".format(field, filename)
response += "</center></body></html>"

print("Content-Length: {}".format(len(response)))
print(response)
