#ifndef AUDIO_H
#define AUDIO_H

#ifndef __3DS__
#include <PR/os.audio.h>
#endif

/* Audio initialization and frame functions */
int audioInit(void);
void audioEndFrame(void);

/* Test pattern generation for debugging */
void audioGenerateTestPattern(void);

#endif /* AUDIO_H */
