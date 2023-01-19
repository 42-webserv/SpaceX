# SpaceX
![parrot_dance](https://user-images.githubusercontent.com/13278955/213354563-523fdc49-b0a8-4af0-b16c-e12fae7d9653.gif)


## About
This project is..

Webserver benchmarking nginx's config and some features.

I/O Multiplexing with kqueue

> Nginx 의 기능일부와 config를 벤치마킹하여 만든 "웹 서버" 입니다.

## What we did was... 

* Socket & socket settings
* IO/Multiplexing with Kqueue
  + kqueue kevent management
* read & write buffer management
* CGI processing
  + CGI environment setting
  + pipe management
* Parsing
  + configuration file syntax checking
  + HTTP 1.1 Request Message
    + URI, HTTP METHOD, header
    + parsial message process
* Cookie & Session Management

> 소켓, Kqueue, 읽기/쓰기 버퍼 관리, CGI 처리, HTTP 요청/응답처리, 쿠키/세션 처리 


## Period 

2022-11-22 ~ 2023-01-18
### Evaluation Score
![image](https://user-images.githubusercontent.com/13278955/213350628-33502676-d02c-4c62-bd4c-f3b25a0574f2.png)

## Project Things
[Project Convention](https://github.com/42-webserv/SpaceX/wiki/CodeConvention)

[Blueprint](https://github.com/42-webserv/SpaceX/wiki/Blueprint)

[RFC Documents](https://github.com/42-webserv/SpaceX/wiki/RFC)

[References](https://github.com/42-webserv/SpaceX/wiki/References)

---


## Logic flow chart
![webserv-kqueue-module](https://user-images.githubusercontent.com/13278955/213333779-18277531-1c9b-4e98-9efe-c6f982e0a1df.jpg)

