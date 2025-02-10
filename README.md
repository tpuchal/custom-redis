# custom-redis
custom implementation of core Redis functionalities in C


simple protocol for start:
the first version of protocol is going to accept messages with a simple protocol. Every message has 4 little endian bytes indicating length of the message followed by variable length payload
