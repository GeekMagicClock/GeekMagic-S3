#ifndef __GIF_H_
#define __GIF_H_
void gifInit(const char *name, bool force);
void gifDeinit();
int drawGif(const char *filename, int x, int y, bool force);
void playGif(const char *filename, int x, int y);
#endif