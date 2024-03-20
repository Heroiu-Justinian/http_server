# HTTP server written in C++
 Simple and featureless http server written in C++.
 Testing purposes only, it was not made with anything production-related in mind.
 It has a very little HTTPS request parser.

## Usage
Simply compile with `g++ server.cpp` and run.
It accepts concurrent connections.
Here is the output of running `oha https://localhost:4221` :

![Screenshot from 2024-03-20 15-49-10](https://github.com/Heroiu-Justinian/http_server/assets/72274906/50633973-47d8-4d71-a7ac-6411d8b4084b)


## TODO
- Identify cause of lower success rate in some cases
- Handle files
- Organize code in files
