# HTTP server written in C++
 Simple and featureless http server written in C++.
 Testing purposes only, it was not made with anything production-related in mind.
 It has a very little HTTPS request parser.

## Usage
Simply compile with `g++ server.cpp` and run
It accepts concurrent connections.
Here is the output of running `oha https://localhost:4221` :

![Screenshot from 2024-03-20 15-35-32](https://github.com/Heroiu-Justinian/http_server/assets/72274906/df44f0aa-2d26-4b11-b6af-ef55b25407cd)

## TODO
- Identify cause of lower success rate in some cases
- Handle files
