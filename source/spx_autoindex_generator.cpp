#include "spx_autoindex_generator.hpp"

std::string
get_file_timetable(struct stat file_status) {
	std::stringstream result;

	// get the size of the file in bytes
	size_t size = file_status.st_size;
	// get the time of the last modification
	time_t	 mtime		 = file_status.st_mtime;
	std::tm* modify_time = std::gmtime(&mtime);

	char date_buf[32];
	std::strftime(date_buf, sizeof(date_buf), "%d-%b-%Y %H:%M",
				  modify_time);

	result << date_buf << "</td><td>"
		   << size;
	return result.str();
}

void
generate_file_status(std::stringstream& result, std::string& filename, uri_resolved_t& path_info) {
	struct stat file_status;
	filename = path_info.script_filename_ + "/" + filename;
	if (stat(filename.c_str(), &file_status) == 0) {
		result << get_file_timetable(file_status);
	}
}

void
generate_root_path(std::string& full_path, uri_resolved_t& path_info) {
	if (path_info.resolved_request_uri_.size() != 1) {
		full_path = path_info.resolved_request_uri_;
	}
	if (full_path.size() != 0) {
		size_t original_size = full_path.size();
		full_path.erase(full_path.begin() + full_path.find_last_of('/'), full_path.end());
		if (full_path.size() == original_size - 1)
			full_path.erase(full_path.begin() + full_path.find_last_of('/'), full_path.end());
	}
	if (full_path.size() == 0) {
		full_path = "/";
	}
}

void
generate_file_path(std::string& full_path, std::string& filename, uri_resolved_t& path_info) {
	if (path_info.resolved_request_uri_.size() != 1) {
		full_path = path_info.resolved_request_uri_ + "/" + filename;
	} else {
		full_path = filename;
	}
}

std::string
generate_autoindex_page(int& req_fd, uri_resolved_t& path_info) {
	DIR*			  dir;
	struct dirent*	  entry;
	std::stringstream result;
	std::string&	  path = path_info.script_filename_;
	std::string		  full_path;

	char* base_name = basename((char*)path.c_str());
	result << HTML_HEAD_TITLE << base_name << HTML_HEAD_TO_BODY << path_info.script_name_ << HTML_BEFORE_LIST;
	result << "<table>";
	if ((dir = opendir(path.c_str())) != NULL) {
		entry = readdir(dir);
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_type & DT_DIR) {
				std::string filename = entry->d_name;
				result << "<tr>";
				result << "<td>";
				if (filename.compare("..") == 0) {
					generate_root_path(full_path, path_info);
					result << A_TAG_START << full_path << A_TAG_END;
					result << "📁 " << filename << CLOSE_A_TAG << CRLF;
					continue;
				} else {
					generate_file_path(full_path, filename, path_info);
					result << A_TAG_START << full_path << A_TAG_END;
				}
				result << "📁 " << filename << "/" << CLOSE_A_TAG << "</td>"
					   << "<td " << TD_STYLE << ">";
				generate_file_status(result, filename, path_info);
				result << "</td>"
					   << "</tr>" << CRLF;
			}
		}
		rewinddir(dir);
		while ((entry = readdir(dir)) != NULL) {
			if ((entry->d_type & DT_DIR) == false) {
				// get the name of the file
				std::string filename = entry->d_name;
				if (filename.size() > 1 && filename[0] == '.' && filename[1] != '.')
					continue;
				result << "<tr>";
				result << "<td>";
				if (path_info.resolved_request_uri_.size() != 1) {
					full_path = path_info.resolved_request_uri_ + "/" + filename;
				} else {
					full_path = filename;
				}
				result << A_TAG_START << full_path << A_TAG_END;
				result << filename << CLOSE_A_TAG << "</td>"
					   << "<td " << TD_STYLE << ">";

				generate_file_status(result, filename, path_info);

				result << "</td>"
					   << "</tr>" << CRLF;
			}
		}
		result << "</table>" << HTML_AFTER_LIST;
		closedir(dir);
		return result.str();
	} else {
		req_fd = -1;
		return "";
	}
}
