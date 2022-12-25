#include "spx_autoindex_generator.hpp"

std::string
generate_autoindex_page(uintptr_t& req_fd, const std::string& path) {
	DIR*			  dir;
	struct dirent*	  entry;
	std::stringstream result;

	std::string full_path("/");
	full_path + path;
	char* base_name = basename((char*)full_path.c_str());
	result << HTML_HEAD_TITLE << base_name << HTML_HEAD_TO_BODY << base_name
		   << HTML_BEFORE_LIST;
	result << "<table>";
	if ((dir = opendir(path.c_str())) != NULL) {
		/* print all the files and directories within directory */
		entry = readdir(dir);
		while ((entry = readdir(dir)) != NULL) {
			// get the name of the file
			std::string filename = entry->d_name;
			// TODO : optimize this if statement
			if (filename.size() > 1 && filename[0] == '.' && filename[1] != '.')
				continue;
			result << "<tr>";
			// get the full path of the file
			result << "<td>";
			result << A_TAG_START << entry->d_name << A_TAG_END;

			if (filename.compare("..") == 0) {
				result << filename << CLOSE_A_TAG << CRLF;
				continue;
			}
			std::string full_path = path + filename;
			struct stat file_status;
			if (stat(full_path.c_str(), &file_status) == 0) {
				// get the size of the file in bytes
				size_t size = file_status.st_size;
				// get the time of the last modification
				time_t	 mtime		 = file_status.st_mtime;
				std::tm* modify_time = std::gmtime(&mtime);

				char date_buf[32];
				std::strftime(date_buf, sizeof(date_buf), "%d-%b-%Y %H:%M",
							  modify_time);
				// print the name, size, and modification time
				result << filename << CLOSE_A_TAG << "</td>"
					   << "<td " << TD_STYLE << ">" << date_buf << "</td><td>"
					   << size << "</td>" << CRLF;
			}
		}
		result << "</table>" << HTML_AFTER_LIST;
		closedir(dir);
		return result.str();
	} else {
		req_fd = -1;
		return NULL;
	}
}
