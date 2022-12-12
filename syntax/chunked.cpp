// length := 0
//   read chunk-size, chunk-ext (if any), and CRLF
//   while (chunk-size > 0) {
//      read chunk-data and CRLF
//      append chunk-data to content
//      length := length + chunk-size
//      read chunk-size, chunk-ext (if any), and CRLF
//   }
//   read trailer field
//   while (trailer field is not empty) {
//      if (trailer fields are stored/forwarded separately) {
//          append trailer field to existing trailer fields
//      }
//      else if (trailer field is understood and defined as mergeable) {
//          merge trailer field with existing header fields
//      }
//      else {
//          discard trailer field
//      }
//      read trailer field
//   }
//   Content-Length := length
//   Remove "chunked" from Transfer-Encoding

/*
chunked-body   = *chunk
				   last-chunk
				   trailer-section
				   CRLF

  chunk          = chunk-size [ chunk-ext ] CRLF
				   chunk-data CRLF
  chunk-size     = 1*HEXDIG
  last-chunk     = 1*("0") [ chunk-ext ] CRLF

  chunk-data     = 1*OCTET ; a sequence of chunk-size octets
*/

/*
   chunk-ext      = *( BWS ";" BWS chunk-ext-name
					   [ BWS "=" BWS chunk-ext-val ] )

   chunk-ext-name = token
   chunk-ext-val  = token / quoted-string
*/

// trailer-section   = *( field-line CRLF )

// field - line = field - name ":" OWS field - value OWS

// field-name     = token
//   field-value    = *field-content
//   field-content  = field-vchar
//                    [ 1*( SP / HTAB / field-vchar ) field-vchar ]
//   field-vchar    = VCHAR / obs-text
//   obs-text       = %x80-FF

//  token          = 1*tchar

//   tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
//                  / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
//                  / DIGIT / ALPHA
//                  ; any VCHAR, except delimiters
