#!/usr/bin/python
import cgi
import os
import cgitb
cgitb.enable()
# import cgitb; cgitb.enable(display=0, logdir="./log")

save_path = os.getenv("SAVED_PATH")

if save_path is None:
    save_path = "./tmp/"
else:
    save_path += "/"

print("Content-Type: text/html\r")

if not os.path.exists(save_path):
    os.mkdir(save_path)

# Parse the form data
form = cgi.FieldStorage()
# Extract the uploaded files
files = {}
for field in form.keys():
    # Check if the field is a file field
    if isinstance(form[field], cgi.FieldStorage) and form[field].filename:
        # Save the file to the specified location
        with open(save_path + form[field].filename, 'wb') as f:
            f.write(form[field].file.read())
        files[field] = form[field].filename

if len(files) > 0:
    message = "file uploaded success"
else:
    message = "file uploaded failed"

# Print the uploaded files
response = "<html><body><center>"
response += "<h1>{}<h1>".format(message)
for field, filename in files.items():
    response += "<p>{}: {}</p>\n".format(field, filename)
response += "</center></body></html>"

print("\r")
print(response)

exit()
