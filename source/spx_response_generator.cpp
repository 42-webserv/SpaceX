#include "spx_response_generator.hpp"

void
Response::write_to_response_buffer(res_field_t& cur_res, const std::string& content) {
	cur_res.res_buffer_.insert(cur_res.res_buffer_.end(), content.begin(), content.end());
	cur_res.buf_size_ += content.size();
}

std::string
Response::make_to_string() const {
	std::stringstream stream;
	stream << "HTTP/" << version_major_ << "." << version_minor_ << " " << status_code_ << " " << status_
		   << CRLF;
	for (std::vector<Response::header>::const_iterator it = headers_.begin();
		 it != headers_.end(); ++it)
		stream << it->first << ": " << it->second << CRLF;
	stream << CRLF;
	return stream.str();
}

int
Response::file_open(const char* dir) const {
	int fd = open(dir, O_RDONLY | O_NONBLOCK, 0644);
	if (fd < 0 && errno == EACCES)
		return 0;
	return fd;
}

off_t
Response::setContentLength(int fd) {
	if (fd < 0)
		return 0;
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
Response::make_error_response(ClientBuffer& client_buffer, http_status error_code) {
	status_			 = http_status_str(error_code);
	status_code_	 = error_code;
	req_field_t& req = client_buffer.req_res_queue_.front().first;

	if (error_code == HTTP_STATUS_BAD_REQUEST)
		headers_.push_back(header(CONNECTION, CONNECTION_CLOSE));
	else
		headers_.push_back(header(CONNECTION, KEEP_ALIVE));

	std::string page_path	 = client_buffer.req_res_queue_.front().first.serv_info_->get_error_page_path_(status_code_);
	int			error_req_fd = open(page_path.c_str(), O_RDONLY);
	if (error_req_fd < 0) {
		std::stringstream  ss;
		const std::string& error_page = generator_error_page_(status_code_);

		ss << error_page.length();
		headers_.push_back(header(CONTENT_LENGTH, ss.str()));
		headers_.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		write_to_response_buffer(client_buffer.req_res_queue_.front().second, make_to_string());
		if (req.req_type_ != REQ_HEAD)
			write_to_response_buffer(client_buffer.req_res_queue_.front().second, error_page);
	}
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
void
Response::make_response_header(ClientBuffer& client_buffer) {
	const req_field_t& cur_req
		= client_buffer.req_res_queue_.front().first;
	res_field_t& cur_res
		= client_buffer.req_res_queue_.front().second;

	const std::string& uri		  = cur_req.file_path_;
	int				   req_fd	  = -1;
	int				   req_method = cur_req.req_type_;
	std::string		   content;

	// Set Date Header
	setDate();
	if ((req_method & (REQ_GET | REQ_HEAD))) {
		req_fd			 = file_open(uri.c_str());
		cur_res.body_fd_ = req_fd;
		if (req_fd == 0) {
			make_error_response(client_buffer, HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1 && cur_req.uri_loc_ == NULL) {
			make_error_response(client_buffer, HTTP_STATUS_NOT_FOUND);
			return;
		} else if (req_fd == -1 && cur_req.uri_loc_->autoindex_flag == Kautoindex_on) {
			content = generate_autoindex_page(req_fd, cur_req.uri_loc_->root);
			if (content.empty()) {
				make_error_response(client_buffer, HTTP_STATUS_FORBIDDEN);
				return;
			}
		}
		setContentType(uri);
		off_t content_length = setContentLength(req_fd);
		if (req_method == REQ_GET)
			cur_res.buf_size_ += content_length;
		headers_.push_back(header(CONNECTION, KEEP_ALIVE));
		cur_res.body_fd_ = req_fd;
	}
	// settting response_header size  + content-length size to res_field
	write_to_response_buffer(cur_res, make_to_string());
	if (!content.empty()) {
		write_to_response_buffer(cur_res, content);
	}
}

void
Response::make_redirect_response(const std::string& redirect_uri, res_field_t& res) {
	setDate();
	status_code_ = HTTP_STATUS_MOVED_PERMANENTLY;
	status_		 = http_status_str(HTTP_STATUS_MOVED_PERMANENTLY);
	headers_.push_back(header("Location", redirect_uri));
	write_to_response_buffer(res, make_to_string());
}
