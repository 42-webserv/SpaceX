#include "spx_req_res_field.hpp"
#include "spx_cgi_module.hpp"
#include "spx_kqueue_module.hpp"

#include "spx_session_storage.hpp"

ReqField::ReqField()
	: _body_buf()
	, _body_size(0)
	, _body_read(0)
	, _body_limit(-1)
	, _cnt_len(-1)
	, _body_fd(-1)
	, _header()
	, _uri()
	, _http_ver()
	, _upld_fn()
	, _uri_loc(NULL)
	, _uri_resolv()
	, _req_mthd(REQ_LINE_PARSING)
	, _is_chnkd(false)
	, _flag(0) {
}

ReqField::~ReqField() {
}

void
ReqField::clear_() {
	_header.clear();
	_uri.clear();
	_http_ver.clear();
	_upld_fn.clear();
	_body_size	= 0;
	_body_read	= 0;
	_body_limit = -1;
	_cnt_len	= -1;
	_body_fd	= -1;
	_uri_loc	= NULL;
	_req_mthd	= REQ_LINE_PARSING;
	_is_chnkd	= false;
	_flag		= 0;
}

bool
ReqField::set_uri_info_and_cookie_(Client& cl) {
	std::string& host = _header["host"];

	_serv_info = &cl._port_info->search_server_config_(host);
	if (host_check_(host) == false) {
		cl._res.make_error_response_(cl, HTTP_STATUS_BAD_REQUEST);
		cl._state = E_BAD_REQ;
		return false;
	}
	_uri_loc = _serv_info->get_uri_location_t_(_uri, _uri_resolv, _req_mthd);

	if (_uri_loc) {
		_body_limit = _uri_loc->client_max_body_size;
	}

	if (_req_mthd & REQ_DELETE) {
		_uri_resolv.is_cgi_ = false;
	}

	cl.set_cookie_();
	return true;
}

bool
ReqField::host_check_(std::string& host) {
	if (host.empty() || (host.size() && host.find_first_of(" \t") == std::string::npos)) {
		return true;
	}
	return false;
}

bool
ReqField::res_for_get_head_req_(Client& cl) {
	cl._res.make_response_header_(cl);

	if (cl._res._body_fd > 0) {
		if (cl._res._body_size) {
			add_change_list(*cl.change_list, cl._res._body_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, &cl);
		} else {
			close(cl._res._body_fd);
			cl._res._body_fd = -1;
		}
	}

	if (_is_chnkd) {
		cl._state = REQ_SKIP_BODY_CHUNKED;
	} else if (_cnt_len == 0 || _cnt_len == SIZE_T_MAX) {
		cl._state = REQ_HOLD;
	} else {
		cl._skip_size = _cnt_len;
		cl._state	  = REQ_SKIP_BODY;
	}
	return false;
}

bool
ReqField::res_for_post_put_req_(Client& cl) {
	_upld_fn = _uri_resolv.script_filename_;

	if (_req_mthd & REQ_POST) {
		_body_fd = open(_upld_fn.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND, 0644);
	} else {
		_body_fd = open(_upld_fn.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK | O_TRUNC, 0644);
	}
	if (_body_fd < 0) {
		// 405 not allowed error with keep-alive connection.
		cl.error_response_keep_alive_(HTTP_STATUS_NOT_FOUND);
		return false;
	}

	if (_is_chnkd || _cnt_len == SIZE_T_MAX) {
		_body_size = 0;
		_cnt_len   = -1;
		cl._state  = REQ_BODY_CHUNKED;
		cl._chnkd.chunked_body_(cl);
	} else {
		if (_cnt_len > _body_limit) {
			close(_body_fd);
			remove(_upld_fn.c_str());
			cl.error_response_keep_alive_(HTTP_STATUS_RANGE_NOT_SATISFIABLE);
			return false;
		} else {
			add_change_list(*cl.change_list, _body_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
			cl._state = REQ_BODY;
			cl.state_req_body_();
		}
	}
	return false;
}

void
ReqField::res_for_delete_req_(Client& cl) {
	if (remove(_uri_resolv.script_filename_.c_str()) == -1) {
		cl.error_response_keep_alive_(HTTP_STATUS_NOT_FOUND);
	} else {
		cl._res.make_response_header_(cl);
	}
	if (_is_chnkd) {
		cl._state = REQ_SKIP_BODY_CHUNKED;
	} else if (_cnt_len == 0 || _cnt_len == SIZE_T_MAX) {
		cl._state = REQ_HOLD;
	} else {
		cl._skip_size = _cnt_len;
		cl._state	  = REQ_SKIP_BODY;
	}
}

ResField::ResField()
	: _res_header()
	, _dwnl_fn()
	, _res_buf()
	, _body_read(0)
	, _body_write(0)
	, _body_size(0)
	, _body_fd(-1)
	, _is_chnkd(false)
	, _header_sent(0)
	, _write_finished(false)
	, _headers()
	, _version_minor(1)
	, _version_major(1)
	, _status_code(200)
	, _status("OK") {
}

ResField::~ResField() {
}

void
ResField::clear_() {
	_res_header.clear();
	_dwnl_fn.clear();
	_headers.clear();
	_res_buf.clear_();
	_body_read		= 0;
	_body_write		= 0;
	_body_size		= 0;
	_body_fd		= -1;
	_is_chnkd		= false;
	_header_sent	= 0;
	_write_finished = false;
	_version_minor	= 1;
	_version_major	= 1;
	_status_code	= 200;
	_status			= "OK";
}

void
ResField::make_error_response_(Client& cl, http_status error_code) {
	_status		 = http_status_str(error_code);
	_status_code = error_code;

	// headers_.push_back(header("Server", "SpaceX/12.26"));

	if (error_code == HTTP_STATUS_BAD_REQUEST) {
		_headers.push_back(header(CONNECTION, CONNECTION_CLOSE));
	} else {
		_headers.push_back(header(CONNECTION, KEEP_ALIVE));
	}

	// page_path null case added..
	std::string page_path;
	int			error_req_fd;

	if (cl._req._serv_info) {
		page_path	 = cl._req._serv_info->get_error_page_path_(error_code);
		error_req_fd = open(page_path.c_str(), O_RDONLY);
	} else {
		error_req_fd = -1;
	}
	if (error_req_fd < 0) {
		std::stringstream ss;
		const std::string error_page = generator_error_page_(error_code);

		_body_size = error_page.length();
		ss << _body_size;
		_headers.push_back(header(CONTENT_LENGTH, ss.str()));
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		std::string tmp = make_to_string_();

		if (!cl._req.session_id.empty()) {
			_headers.push_back(header("Set-Cookie", cl._req.session_id));
		}

		write_to_response_buffer_(tmp);

		if ((cl._req._req_mthd & REQ_HEAD) == false) {
			// write_to_response_buffer_(error_page);
			cl._res._res_buf.add_str(error_page);
		}
		add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
		return;
	}

	set_content_type_(page_path);
	set_content_length_(error_req_fd);

	if ((cl._req._req_mthd & REQ_HEAD)) {
		close(error_req_fd);
		_body_fd = -1;
	} else {
		_body_fd = error_req_fd;
	}

	if (!cl._req.session_id.empty()) {
		_headers.push_back(header("Set-Cookie", cl._req.session_id));
	}

	write_to_response_buffer_(make_to_string_());
	if (_body_fd <= 0) {
		add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
	}
}

// this is main logic to make response
void
ResField::make_response_header_(Client& cl) {
	const std::string& uri	  = cl._req._uri_resolv.script_filename_;
	int				   req_fd = -1;
	std::string		   content;

	// Set Date Header
	set_date_();

	// Redirect
	if (cl._req._uri_loc != NULL && !(cl._req._uri_loc->redirect.empty())) {
		make_redirect_response_(cl);
		return;
	}

	switch (cl._req._req_mthd) {
	case REQ_GET:
	case REQ_HEAD:
		// folder skip logic
		if (uri[uri.size() - 1] != '/') {
			req_fd = file_open_(uri.c_str());
		}
		if (req_fd == 0) {
			make_error_response_(cl, HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1
				   && (cl._req._uri_loc == NULL || cl._req._uri_loc->autoindex_flag == Kautoindex_off)) {
			make_error_response_(cl, HTTP_STATUS_NOT_FOUND);
			return;
		} else if (req_fd == -1
				   && cl._req._uri_loc->autoindex_flag == Kautoindex_on) {
			content = generate_autoindex_page(req_fd, cl._req._uri_resolv);
			std::stringstream ss;
			ss << content.size();
			_headers.push_back(header(CONTENT_LENGTH, ss.str()));
			_body_size = content.size();

			// autoindex fail case
			if (content.empty()) {
				make_error_response_(cl, HTTP_STATUS_FORBIDDEN);
				return;
			}
		}
		if (req_fd != -1) {
			set_content_type_(uri);
			set_content_length_(req_fd);
			if (cl._req._req_mthd == REQ_GET) {
				_body_fd = req_fd;
			} else {
				_body_fd = -1;
				close(req_fd);
			}
		} else {
			_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		}
		break;
	case REQ_POST:
	case REQ_PUT:
		_headers.push_back(header(CONTENT_LENGTH, "0"));
		break;
	case REQ_DELETE:
		_headers.push_back(header(CONTENT_LENGTH, "0"));
	}

	if (cl._req._header["connection"] == "close") {
		_headers.push_back(header(CONNECTION, CONNECTION_CLOSE));
		cl._state = E_BAD_REQ;
	} else {
		_headers.push_back(header(CONNECTION, KEEP_ALIVE));
	}
	if (!cl._req.session_id.empty()) {
		_headers.push_back(header("Set-Cookie", cl._req.session_id));
	}

	write_to_response_buffer_(make_to_string_());
	if (!content.empty()) {
		_res_buf.add_str(content);
	}
	if (cl._req._req_mthd & REQ_HEAD) {
		_body_size = 0;
	}
	if (_body_fd <= 0) {
		add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
	}
}

void
ResField::make_cgi_response_header_(Client& cl) {
	// Set Date Header
	set_date_();
	std::map<std::string, std::string>::iterator it;

	it = cl._cgi._cgi_header.find("status");
	if (it != cl._cgi._cgi_header.end()) {
		_status_code = strtol(it->second.c_str(), NULL, 10);
	} else {
		_status_code = 200;
	}

	_headers.push_back(header(CONNECTION, KEEP_ALIVE));

	it = cl._cgi._cgi_header.find("content-length");
	if (it != cl._cgi._cgi_header.end()) {
		_headers.push_back(header(CONTENT_LENGTH, it->second));
	} else {
		_body_size = cl._cgi._from_cgi.buf_size_();
		std::stringstream ss;
		ss << _body_size;
		_headers.push_back(header(CONTENT_LENGTH, ss.str().c_str()));
	}
	if (!cl._req.session_id.empty()) {
		_headers.push_back(header("Set-Cookie", cl._req.session_id));
	}
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
}

void
ResField::make_redirect_response_(Client& cl) {
	_status_code = HTTP_STATUS_MOVED_PERMANENTLY;
	_status		 = http_status_str(HTTP_STATUS_MOVED_PERMANENTLY);
	_headers.push_back(header("Location", cl._req._uri_loc->redirect));
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &cl);
}

int
ResField::file_open_(const char* dir) const {
	struct stat buf;

	stat(dir, &buf);
	if (S_ISDIR(buf.st_mode))
		return -1;
	int fd = open(dir, O_RDONLY | O_NONBLOCK, 0644);
	if (fd < 0 && errno == EACCES)
		return 0;
	return fd;
}

off_t
ResField::set_content_length_(int fd) {
	if (fd < 0)
		return 0;
	off_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	std::stringstream ss;
	ss << length;
	_body_size = length;

	_headers.push_back(header(CONTENT_LENGTH, ss.str()));
	return length;
}

void
ResField::set_content_type_(std::string uri) {
	std::string::size_type uri_ext_size = uri.find_last_of('.');
	std::string			   ext;

	if (uri_ext_size != std::string::npos) {
		ext = uri.substr(uri_ext_size + 1);
	}
	if (ext == "html" || ext == "htm")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
	else if (ext == "png")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_PNG));
	else if (ext == "jpg")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_JPG));
	else if (ext == "jpeg")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_JPEG));
	else if (ext == "txt")
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_TEXT));
	else
		_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_DEFUALT));
}

void
ResField::set_date_(void) {
	std::time_t now			 = std::time(nullptr);
	std::tm*	current_time = std::gmtime(&now);

	char date_buf[32];
	std::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %T GMT", current_time);
	_headers.push_back(header("Date", date_buf));
}

std::string
ResField::make_to_string_() const {
	std::stringstream stream;
	stream << "HTTP/" << _version_major << "." << _version_minor
		   << " " << _status_code << " " << _status << CRLF;
	for (std::vector<header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		stream << it->first << ": " << it->second << CRLF;
	}
	stream << CRLF;
	return stream.str();
}

void
ResField::write_to_response_buffer_(const std::string& content) {
	_res_header.insert(_res_header.end(), content.begin(), content.end());
}
