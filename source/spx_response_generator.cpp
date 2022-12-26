#include "spx_response_generator.hpp"

void
ResField::write_to_response_buffer(const std::string& content) {
	res_buffer_.insert(res_buffer_.end(), content.begin(), content.end());
	buf_size_ += content.size();
}

std::string
ResField::make_to_string() const {
	std::stringstream stream;
	stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_code_ << " " << status_
		   << CRLF;
	for (std::vector<header>::const_iterator it = headers_.begin();
		 it != headers_.end(); ++it)
		stream << it->first << ": " << it->second << CRLF;
	stream << CRLF;
	return stream.str();
}

int
ResField::file_open(const char* dir) const {
	int fd = open(dir, O_RDONLY | O_NONBLOCK, 0644);
	if (fd < 0 && errno == EACCES)
		return 0;
	return fd;
}

off_t
ResField::setContentLength(int fd) {
	if (fd < 0)
		return 0;
	off_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, SEEK_CUR, SEEK_SET);

	std::stringstream ss;
	ss << length;

	headers_.push_back(header(CONTENT_LENGTH, ss.str()));
	return length;
}

void
ResField::setContentType(std::string uri) {

	std::string::size_type uri_ext_size = uri.find_last_of('.');
	std::string			   ext;

	if (uri_ext_size)
		ext = uri.substr(uri_ext_size + 1);
	if (ext == "html" || ext == "htm")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
	else if (ext == "png")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_PNG));
	else if (ext == "jpg")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_JPG));
	else if (ext == "jpeg")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_JPEG));
	else if (ext == "txt")
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_TEXT));
	else
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_DEFUALT));
};

void
ResField::setDate(void) {
	std::time_t now			 = std::time(nullptr);
	std::tm*	current_time = std::gmtime(&now);

	char date_buf[32];
	std::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %T GMT", current_time);
	headers_.push_back(header("Date", date_buf));
}

void
ClientBuffer::make_error_response(http_status error_code) {
	res_field_t& res = req_res_queue_.front().second;
	req_field_t& req = req_res_queue_.front().first;

	res.status_		 = http_status_str(error_code);
	res.status_code_ = error_code;

	if (error_code == HTTP_STATUS_BAD_REQUEST)
		res.headers_.push_back(header(CONNECTION, CONNECTION_CLOSE));
	else
		res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));

	std::string page_path	 = req.serv_info_->get_error_page_path_(error_code);
	int			error_req_fd = open(page_path.c_str(), O_RDONLY);
	if (error_req_fd < 0) {
		std::stringstream  ss;
		const std::string& error_page = generator_error_page_(error_code);

		ss << error_page.length();
		res.headers_.push_back(header(CONTENT_LENGTH, ss.str()));
		res.headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		res.write_to_response_buffer(res.make_to_string());
		if (req.req_type_ != REQ_HEAD)
			res.write_to_response_buffer(error_page);
	}
}

// this is main logic to make response
void
ClientBuffer::make_response_header() {
	const req_field_t& req
		= req_res_queue_.back().first;
	res_field_t& res
		= req_res_queue_.back().second;

	const std::string& uri		  = req.file_path_;
	int				   req_fd	  = -1;
	int				   req_method = req.req_type_;
	std::string		   content;

	// Set Date Header
	res.setDate();
	switch (req_method) {
	case (REQ_HEAD):
		make_redirect_response();
		break;
	case (REQ_GET):
		req_fd		 = res.file_open(uri.c_str());
		res.body_fd_ = req_fd;
		if (req_fd == 0) {
			make_error_response(HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1 && req.uri_loc_ == NULL) {
			make_error_response(HTTP_STATUS_NOT_FOUND);
			return;
		} else if (req_fd == -1 && req.uri_loc_->autoindex_flag == Kautoindex_on) {
			content = generate_autoindex_page(req_fd, req.uri_loc_->root);
			if (content.empty()) {
				make_error_response(HTTP_STATUS_FORBIDDEN);
				return;
			}
		}
		res.setContentType(uri);
		off_t content_length = res.setContentLength(req_fd);
		if (req_method == REQ_GET)
			res.buf_size_ += content_length;
		res.headers_.push_back(header(CONNECTION, KEEP_ALIVE));
		res.body_fd_ = req_fd;
		break;
	}
	// settting response_header size  + content-length size to res_field
	res.write_to_response_buffer(res.make_to_string());
	if (!content.empty()) {
		res.write_to_response_buffer(content);
	}
}

void
ClientBuffer::make_redirect_response() {
	const req_field_t& req
		= req_res_queue_.front().first;
	res_field_t& res
		= req_res_queue_.front().second;

	res.setDate();
	if (req.uri_loc_ == NULL || req.uri_loc_->redirect.empty()) {
		make_error_response(HTTP_STATUS_NOT_FOUND);
		return;
	};
	res.status_code_ = HTTP_STATUS_MOVED_PERMANENTLY;
	res.status_		 = http_status_str(HTTP_STATUS_MOVED_PERMANENTLY);
	res.headers_.push_back(header("Location", req.uri_loc_->redirect));
	res.write_to_response_buffer(res.make_to_string());
}
