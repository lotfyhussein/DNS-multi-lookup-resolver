# DNS-multi-lookup-resolver

Name: Lotfy Abdel KHaliq


DNS MULTILOOKUP

Functions: 

1- Requester: handles reading inpt files and pushinng them to queue

2- Resolver: handles popping files from the queue and calling dns lookup function



How to Compile:

gcc -pthread multi-lookup.c queue.c util.c -o multi-lookup


HOW TO RUN:

time ./multi-lookup 2 5 results.txt names1.txt names2.txt names3.txt names4.txt names5.txt
