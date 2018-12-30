#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <pspaudiolib.h>


#include "FC.h"
#include "MyTypes.h"
#include "LamePaula.h"



int done = 0;
int channel;

int
audio_thread (SceSize args, void *argp)
{
	ubyte* sample_buffer;
    sample_buffer = (ubyte *) malloc (1024 * 4);
    while (!done) {
		mixerFillBuffer(sample_buffer, 1024 * 4);
    	sceAudioOutputPannedBlocking(channel, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, sample_buffer); 
    }
    free(sample_buffer);
	return 0;
}


char infile[] = "host0:/data/cytax-1.fc4";


int fcplay_init(void)
{
    char *file_buffer;
    int  file_len;

    SceUID fd;
    SceIoStat stat;
    sceIoGetstat(infile,&stat);
    file_len = stat.st_size;
    printf("Open File '%s' (%i bytes) \n",infile,file_len);
    file_buffer = (char*)malloc(file_len);
    fprintf(stderr,"Song buffer=%p\n",file_buffer);
    if ((fd = sceIoOpen (infile, PSP_O_RDONLY, 0777))) {
            file_len = 
            sceIoRead (fd, file_buffer,file_len);
            sceIoClose (fd);
    } else {
        fprintf(stderr,"Cannot open '%s'\n",infile);
        return -1;
    }
    printf("Song '%s' loaded\n",infile);
    
    channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,1024,PSP_AUDIO_FORMAT_STEREO); 
    printf("Got audio channel %i \n",channel);
    if (!FC_init(file_buffer, file_len, 0, 0)) {
	    fprintf( stderr, "File format not recognized.\n");
	    return -1;
    }
    printf("FC init done\n");
    return 0;
} 

int fcplay_start()
{
    printf("Mixer init\n");
    mixerInit(44100, 16, 2, 0);
    mixerSetReplayingSpeed();

	SceUID thid;
	thid = sceKernelCreateThread ("audio_thread", audio_thread, 0x18,
				      0x1000, PSP_THREAD_ATTR_USER, NULL);
	if (thid < 0) {
		fprintf (stderr, "Error, could not create thread\n");
		return -1;
	}
	sceKernelStartThread (thid, 0, NULL);
    fprintf(stderr,"Audio Thread started\n");
    return  -1;
}
