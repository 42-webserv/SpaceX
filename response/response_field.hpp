#pragma once

#include <string>

typedef struct ResField {
	std::string res_header_;
	int			body_flag_;
	std::string file_path_;
	int			sent_pos_;
} t_res_field;
