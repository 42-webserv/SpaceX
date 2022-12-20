#include "spx_response_generator.hpp"

struct Response {
	typedef std::pair<std::string, std::string> header;

private:
	std::vector<header> headers_;
	int					version_minor_;
	int					version_major_;
	unsigned int		status_code_;
	std::string			status_;
	bool				keep_alive_ = true;

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
	Response::file_open(const char* dir) const {
		int fd = open(dir, O_RDONLY);
		if (fd < 0)
			return open(ERR_PAGE_URL, O_RDONLY);
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

		// request extention checking
		std::string::size_type uri_ext_size = uri.find_last_of('.');
		std::string			   ext;

		if (uri_ext_size)
			ext = uri.substr(uri_ext_size + 1);
		if (ext == "html")
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
		char* err = http_error_400_page;
		// this will write to vector<char> response_body
		return make_to_string();
	}

public:
	std::string
	Response::make_error_response(std::vector<struct kevent>& change_list, ClientBuffer& client_buffer, http_status error_code) {

		status_		 = http_status_str(error_code);
		status_code_ = error_code;

		if (error_code == HTTP_STATUS_BAD_REQUEST)
			keep_alive_ = false;

		int error_req_fd = open(ERR_PAGE_URL, O_RDONLY);
		if (error_req_fd < 0)
			return handle_static_error_page();
		add_change_list(change_list, error_req_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &client_buffer);
	}

	// this is main logic to make response
	std::string
	Response::setting_response_header(std::vector<struct kevent>& change_list, ClientBuffer& client_buffer) {
		t_req_field current_request = client_buffer.req_res_queue_.front().first;
		std::string uri				= current_request.req_target_;
		int			request_fd		= file_open(uri.c_str());

		// File Not Found - URL
		if (request_fd < 0)
			return make_error_response(change_list, client_buffer, HTTP_STATUS_NOT_FOUND);
		setContentType(uri);
		setContentLength(request_fd);
		// body event register
		add_change_list(change_list, request_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &client_buffer);
		return make_to_string();
	}
};