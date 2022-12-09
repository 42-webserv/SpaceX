
{

length:
	= 0 read chunk - size, chunk - ext(if any), and CRLF while (chunk - size > 0) {
		read chunk - data and CRLF append chunk - data to content length : = length + chunk - size read chunk - size, chunk - ext(if any), and CRLF
	}
	read trailer field while (trailer field is not empty) {
		if (trailer fields are stored / forwarded separately) {
			append trailer field to existing trailer fields
		} else if (trailer field is understood and defined as mergeable) {
			merge trailer field with existing header fields
		} else {
			discard trailer field
		}
		read trailer field
	}
	Content - Length : = length
							   Remove "chunked" from Transfer
						   - Encoding
}

field - size - max = 8192
