#pragma once
struct timeval { long tv_sec,tv_usec; }; int gettimeofday(struct timeval*,void*);
