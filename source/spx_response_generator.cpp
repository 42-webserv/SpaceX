#include "spx_response_generator.hpp"

std::string
Response::make_to_string() const {
	std::stringstream stream;
	stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_code_ << " " << status_
		   << CRLF;
	for (std::vector<Response::header>::const_iterator it = headers_.begin();
		 it != headers_.end(); ++it)
		stream << it->first << ": " << it->second << CRLF;
	return stream.str();
}

int
Response::file_open(const char* dir, int mode) const {
	int fd = open(dir, mode);

	if (fd < 0) {
		if (errno == EACCES) {
			// permisson err;
		} else
			return open(ERR_PAGE_URL, mode); // ERR_PAGE_URL will changed to ERR_CONFIG
	}

	return fd;
}

void
Response::setContentLength(int fd) {
	off_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, SEEK_CUR, SEEK_SET);

	std::stringstream ss;
	ss << length;

	headers_.push_back(header(CONTENT_LENGTH, ss.str()));
}

void
Response::setContentType(std::string uri) {

	std::string::size_type uri_ext_size = uri.find_last_of('.');
	std::string			   ext;

	if (uri_ext_size)
		ext = uri.substr(uri_ext_size + 1);
	if (ext == "html" || ext == "htm")
		headers_.push_back(header("Content-Type", MIME_TYPE_HTML));
	else if (ext == "png")
		headers_.push_back(header("Content-Type", MIME_TYPE_PNG));
	else if (ext == "jpg")
		headers_.push_back(header("Content-Type", MIME_TYPE_JPG));
	else if (ext == "jpeg")
		headers_.push_back(header("Content-Type", MIME_TYPE_JPEG));
	else if (ext == "txt")
		headers_.push_back(header("Content-Type", MIME_TYPE_TEXT));
	else
		headers_.push_back(header("Content-Type", MIME_TYPE_DEFUALT));
};

std::string
Response::handle_static_error_page() {
	// char* err = http_error_400_page;
	// this will write to vector<char> response_body
	return make_to_string();
}

std::string
Response::make_error_response(ClientBuffer& client_buffer, http_status error_code) {

	status_		 = http_status_str(error_code);
	status_code_ = error_code;

	// TODO : Connection Header settings
	if (error_code == HTTP_STATUS_BAD_REQUEST)
		headers_.push_back(header(CONNECTION, CONNECTION_CLOSE));
	else
		headers_.push_back(header(CONNECTION, KEEP_ALIVE));

	int error_req_fd = open(ERR_PAGE_URL, O_RDONLY);
	if (error_req_fd < 0)
		return handle_static_error_page();
}

void
Response::setDate(void) {
	std::time_t now			 = std::time(nullptr);
	std::tm*	current_time = std::gmtime(&now);

	char date_buf[32];
	std::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %T GMT", current_time);
	headers_.push_back(header("Date", date_buf));
}

// this is main logic to make response
std::string
Response::setting_response_header(ClientBuffer& client_buffer) {

	// error page finding from config
	const server_info_t& server_info		 = client_buffer.serv_info_->search_server_config_();
	std::string			 error_page_location = server_info.get_error_page_path_(HTTP_STATUS_BAD_REQUEST);

	if (error_page_location.empty()) {
		const std::string& errorpage = generator_error_page_(HTTP_STATUS_BAD_REQUEST);
	}

	req_field_t& cur_req
		= client_buffer.req_res_queue_.front().first;
	res_field_t& cur_res
		= client_buffer.req_res_queue_.front().second;

	std::string uri = cur_req.req_target_;
	int			req_fd;
	int			req_method = cur_req.req_type_;

	if (req_method == REQ_GET) {
		req_fd = file_open(uri.c_str(), O_RDONLY);
		if (req_fd < 0)
			make_error_response(client_buffer, HTTP_STATUS_NOT_FOUND);
		setContentLength(req_fd);
		setContentType(uri);
	}

	if (req_method == REQ_POST || req_method == REQ_PUT) {
		int request_fd = file_open(uri.c_str(), cur_res.);
	}

	// File Not Found - URL
	if (req_fd < 0)
		return make_error_response(client_buffer, HTTP_STATUS_NOT_FOUND);

	headers_.push_back(header(CONNECTION, KEEP_ALIVE));
	return make_to_string();
}

// TODO : Redirection 300