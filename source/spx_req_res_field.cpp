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
	, _flag(0)
	, _req_mthd(REQ_LINE_PARSING)
	, _is_chnkd(false) {
}

ReqField::~ReqField() {
}

void
ReqField::clear_() {
	// _body_buf.clear_();
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
	_flag		= 0;
	_req_mthd	= REQ_LINE_PARSING;
	_is_chnkd	= false;
}

ResField::ResField()
	: _res_header()
	, _dwnl_fn()
	, _res_buf()
	, _body_fd(-1)
	, _body_read(0)
	, _body_write(0)
	, _body_size(0)
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
		page_path = cl._req._serv_info->get_error_page_path_(error_code);
		spx_log_("page_path = ", page_path);
		error_req_fd = open(page_path.c_str(), O_RDONLY);
		spx_log_("error_req_fd : ", error_req_fd);
	} else {
		spx_log_("serv info is NULL");
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
		write_to_response_buffer_(tmp);
		if ((cl._req._req_mthd & REQ_HEAD) == false) {
			// write_to_response_buffer_(error_page);
			cl._res._res_buf.add_str(error_page);
		}
		add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
		return;
	}
	if ((cl._req._req_mthd & REQ_HEAD) == false) {
		_body_fd = error_req_fd;
	} else {
		_body_fd = -1;
	}

	setContentType_(page_path);
	setContentLength_(error_req_fd);

	spx_log_("ERROR_RESPONSE!!");
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

// this is main logic to make response
void
ResField::make_response_header_(Client& cl) {

	const std::string& uri	  = cl._req._uri_resolv.script_filename_;
	int				   req_fd = -1;
	std::string		   content;

	// Set Date Header
	setDate_();
	if (!cl._req.session_id.empty()) {
		_headers.push_back(header("Set-Cookie", cl._req.session_id));
	}

	// Redirect
	if (cl._req._uri_loc != NULL && !(cl._req._uri_loc->redirect.empty())) {
		make_redirect_response_(cl);
		return;
	}

	switch (cl._req._req_mthd) {
	case REQ_GET:
	case REQ_HEAD:
		if (uri[uri.size() - 1] != '/') {
			spx_log_("uri.cstr()", uri.c_str());
			req_fd = file_open_(uri.c_str());
		} else {
			spx_log_("folder skip");
		}
		spx_log_("uri_locations", cl._req._uri_loc);
		spx_log_("req_fd", req_fd);
		if (req_fd == 0) {
			spx_log_("folder skip");
			make_error_response_(cl, HTTP_STATUS_FORBIDDEN);
			return;
		} else if (req_fd == -1
				   && (cl._req._uri_loc == NULL || cl._req._uri_loc->autoindex_flag == Kautoindex_off)) {
			make_error_response_(cl, HTTP_STATUS_NOT_FOUND);
			return;
		} else if (req_fd == -1
				   && cl._req._uri_loc->autoindex_flag == Kautoindex_on) {
			spx_log_("uri=======", cl._req._uri_resolv.script_filename_);
			content = generate_autoindex_page(req_fd, cl._req._uri_resolv);
			std::stringstream ss;
			ss << content.size();
			_headers.push_back(header(CONTENT_LENGTH, ss.str()));
			_body_size = content.size();
			if (content.empty()) {
				// autoindex fail case
				make_error_response_(cl, HTTP_STATUS_FORBIDDEN);
				return;
			}
		}
		if (req_fd != -1) {
			spx_log_("res_header");
			setContentType_(uri);
			setContentLength_(req_fd);
			if (cl._req._req_mthd == REQ_GET) {
				_body_fd = req_fd;
			} else {
				_body_fd = -1;
				close(req_fd);
			}
		} else {
			// autoindex case?
			_headers.push_back(header(CONTENT_TYPE, MIME_TYPE_HTML));
		}
		break;
	case REQ_POST:
	case REQ_PUT:
		_headers.push_back(header(CONTENT_LENGTH, "0"));
		break;
	}
	// if (cl._req._header["connection"] == "close") {
	// 	_headers.push_back(header(CONNECTION, CONNECTION_CLOSE));
	// 	cl._state = E_BAD_REQ;
	// } else {
	// 	_headers.push_back(header(CONNECTION, KEEP_ALIVE));
	// }
	write_to_response_buffer_(make_to_string_());
	if (!content.empty()) {
		_res_buf.add_str(content);
	}
	if (cl._req._req_mthd & REQ_HEAD) {
		_body_size = 0;
	}
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

void
ResField::make_cgi_response_header_(Client& cl) {

	// Set Date Header
	setDate_();
	spx_log_("make_cgi_res_header");
	std::map<std::string, std::string>::iterator it;

	it = cl._cgi._cgi_header.find("status");
	if (it != cl._cgi._cgi_header.end()) {
		_status_code = strtol(it->second.c_str(), NULL, 10);
	} else {
		_status_code = 200;
	}
	// headers_.push_back(header("Set-Cookie", "SESSIONID=123456;"));
	// settting response_header size  + content-length size to res_field
	_headers.push_back(header(CONNECTION, KEEP_ALIVE));

	it = cl._cgi._cgi_header.find("content-length");
	if (it != cl._cgi._cgi_header.end()) {
		_headers.push_back(header(CONTENT_LENGTH, it->second));
	} else {
		_body_size = cl._cgi._cgi_read;
		std::stringstream ss;
		ss << _body_size;
		_headers.push_back(header(CONTENT_LENGTH, ss.str().c_str()));
		// headers_.push_back(header(CONTENT_LENGTH, "0"));
	}
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

void
ResField::make_redirect_response_(Client& cl) {
	spx_log_("uri_loc->redirect", cl._req._uri_loc->redirect);
	_status_code = HTTP_STATUS_MOVED_PERMANENTLY;
	_status		 = http_status_str(HTTP_STATUS_MOVED_PERMANENTLY);
	_headers.push_back(header("Location", cl._req._uri_loc->redirect));
	write_to_response_buffer_(make_to_string_());
	add_change_list(*cl.change_list, cl._client_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, &cl);
}

void
ResField::write_to_response_buffer_(const std::string& content) {
	_res_header.insert(_res_header.end(), content.begin(), content.end());
	// _buf_size += content.size();
	// _write_ready = WRITE_READY;
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
ResField::setContentLength_(int fd) {
	if (fd < 0)
		return 0;
	off_t length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	std::stringstream ss;
	ss << length;
	spx_log_("setContentLength. length", length);
	_body_size = length;
	// _body_size += length;

	_headers.push_back(header(CONTENT_LENGTH, ss.str()));
	return length;
}

void
ResField::setContentType_(std::string uri) {

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
ResField::setDate_(void) {
	std::time_t now			 = std::time(nullptr);
	std::tm*	current_time = std::gmtime(&now);

	char date_buf[32];
	std::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %T GMT", current_time);
	_headers.push_back(header("Date", date_buf));
}

void
ResField::clear_() {
	_res_header.clear();
	_dwnl_fn.clear();
	_headers.clear();
	_res_buf.clear_();
	_body_fd		= -1;
	_body_read		= 0;
	_body_write		= 0;
	_body_size		= 0;
	_is_chnkd		= 0;
	_header_sent	= 0;
	_write_finished = false;
	_version_minor	= 1;
	_version_major	= 1;
	_status_code	= 200;
	_status			= "OK";
}
