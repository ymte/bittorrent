#pragma once

void init();
void die();
void cleanup();
void wait_socket_read(int sid);
void wait_socket_write(int sid);
void remove_socket(int sid);

void poll();
bool runpending();

typedef void(*proc)(void*);
void run(proc p, void* arg = 0);