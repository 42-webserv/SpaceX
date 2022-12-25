#include "spx_autoindex_generator.hpp"

int
generate_autoindex_page(const char* path) {
	DIR*		   dir;
	struct dirent* entry;

	std::string full_path("/");
	full_path + path;
	char* base_name = basename((char*)full_path.c_str());
	std::cout << HTML_HEAD_TITLE << base_name << HTML_HEAD_TO_BODY << base_name
			  << HTML_BEFORE_LIST;
	std::cout << "<table>";
	if ((dir = opendir(path)) != NULL) {
		/* print all the files and directories within directory */
		entry = readdir(dir);
		while ((entry = readdir(dir)) != NULL) {
			// get the name of the file
			std::string filename = entry->d_name;
			// TODO : optimize this if statement
			if (filename.size() > 1 && filename[0] == '.' && filename[1] != '.')
				continue;
			std::cout << "<tr>";
			// get the full path of the file
			std::cout << "<td>";
			std::cout << A_TAG_START << entry->d_name << A_TAG_END;

			if (filename.compare("..") == 0) {
				std::cout << filename << CLOSE_A_TAG << CRLF;
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
				std::cout << filename << CLOSE_A_TAG << "</td>"
						  << "<td " << TD_STYLE << ">" << date_buf << "</td><td>"
						  << size << "</td>" << CRLF;
			}
		}
		std::cout << "</table>" << HTML_AFTER_LIST;
		closedir(dir);
	} else {
		return -1;
		/*
		std::cout
			<< "error" << std::endl;
		perror("");
		*/
	}
}
