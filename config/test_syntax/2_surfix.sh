server {
    listen                  localhost:8080  default_server ;
    server_name             test_intra ;
    # error_page              ./error/error.html;
    error_page              404 ./error/404.html;
    error_page              405 ./error/405.html;
    # error_page              504 503 507 520 ./www/error50X.html;
    root                    ./data;

    location / {
        accepted_methods    GET;
        # index               index.html;
        autoindex           on;
    }

    location /index {
        accepted_methods    GET;
        root                ./data;
        autoindex           on;
    }

    location /put_test {
        accepted_methods    PUT;
        saved_path          put_saved;
    }

    location /post_body {
        accepted_methods    POST;
        saved_path          post_saved ;
        max_body_size       100;
    }

    location /directory {
        accepted_methods    GET POST;
        root                ./data/YoupiBanane;
        index               youpi.bad_extension;
        saved_path          directory_post_saved;
    }

    location .bla {
        accepted_methods    GET PUT POST;
        cgi_path_info       ./cgi_bin/42_cgi_tester;
    }

    location .pl {
        accepted_methods    GET PUT POST;
        cgi_path_info       ./cgi_bin/add_cgi.pl;
        # cgi_path_info       usr/bin/perl;
    }

    location .py {
        accepted_methods    PUT POST;
        cgi_path_info       ./cgi_bin/upload_cgi.py;
        # cgi_path_info       /Library/Frameworks/Python.framework/Versions/3.10/bin/python3;
        saved_path          ./data/upload;
    }

    location /upload   {
        accepted_methods    ALL;
        # index               index.html;
        saved_path          upload_store;
        max_body_size       100M;
        autoindex           on;
    }

    location /upload_pass   {
        accepted_methods    POST PUT;
        cgi_path_info       ./data/cgi_bin/upload_cgi.py;
        saved_path          ./data/cgi_bin/store;
    }

    location /add_pass   {
        accepted_methods    GET POST PUT;
        cgi_path_info       ./data/cgi_bin/add_cgi.pl;
    }

    location /cgi_bin {
        accepted_methods    GET POST PUT;
        root                ./cgi_bin;
        saved_path          cgi_bin_store;
    }

    location /github {
        accepted_methods GET HEAD;
        redirect         https://github.com;
    }

    location /42 {;
        accepted_methods GET HEAD;
        redirect         https://profile.intra.42.fr;
    }

    location /a {;
        accepted_methods GET HEAD;
        redirect         b;
    }

    location /i {;
        accepted_methods GET HEAD;
        redirect         index;
    }

    # location /b {;
    #     accepted_methods GET HEAD;
    #     redirect         a;
    # }
}
