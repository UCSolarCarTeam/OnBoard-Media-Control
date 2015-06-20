
#include <cv.h>
#include "opencv2/opencv.hpp"
#include <SongLoader.h>
#include "SongPlayer.h"

//for the rasperry pi
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
	#include <SDL.h>
	#include <SDL_thread.h>
	#include <SDL_ttf.h>
	#include <SDL_mixer.h>
}

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#ifdef __amd64__
	#ifndef RUNNINGONINTEL
	#define RUNNINGONINTEL
	#endif
#else
	#ifndef RUNNINGONPI
	#define RUNNINGONPI
	#endif
	#include <wiringPi.h>
	//printf("Running on the Pi!\n");
#endif

using namespace cv;

#define SCREEN_HEIGHT 768
#define SCREEN_WIDTH 1232



int cameraWorker(void* data);
void processEvents();
void signalToQuit();
void close();

IplImage threadImage1;
bool updatedImage1 = false;

VideoCapture cap(0);

Mat frame;
SDL_Renderer* renderer = NULL;
SDL_Window* window = NULL;

SDL_Rect videoRect;
int cameraHeight;
int cameraWidth;

SDL_Thread* SDLCameraThread;
SDL_Thread* SDLMusicThread;



int quit;

/***********************************************************************
/*							SDL functions 
/***********************************************************************/
// Initializes SDL window / renderer for use

bool init_SDL()
{
	bool success = true;
	
	if (SDL_Init(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) < 0)
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	} else {
		window = SDL_CreateWindow("Video Application", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL)
		{
			printf("error");
			success = false;
		} else {
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (renderer == NULL)
			{
				printf("Renderer could not be created. SDL_Error: %s \n", SDL_GetError());
				success = false;
			} else {
				SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			}
		}
	}
	return success;
}

bool init_CameraSettings()
{
	bool success = true;
	videoRect.x = 0;
	videoRect.y = 0;
	videoRect.w = 1280;	//640 not actually sure why we have this
	videoRect.h = 720;	//480

	cameraWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	cameraHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	printf("Camera Width%d, Camera Height %d \n",cameraWidth,cameraHeight);
	return success;
}

int cameraWorker(void* data) 
{
	while (!quit)
	{
		cap >> frame;
		threadImage1 = frame;
		updatedImage1 = true;
	}
	return 0;
}


// Shows an individual frame of the supplied video
int show_Camera(IplImage* img)
{		
	if(updatedImage1 == true)
	{
		SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)img->imageData,
			img->width,
			img->height,
			img->depth * img->nChannels,
			img->widthStep,
			0xff0000, 0x00ff00, 0x0000ff, 0
			);
		updatedImage1 = false;

		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
		surface = NULL;
		SDL_RendererFlip flip = SDL_FLIP_HORIZONTAL;
		SDL_RenderCopyEx(renderer, texture, NULL, &videoRect ,0, NULL, flip);
		SDL_DestroyTexture(texture);
		return 1;
	}
		return 0;

}

/***********************************************************************
 * 
 * *********************************************************************/
 
 Mix_Music *gMusic = NULL;
 
 int main(int argc, char* argv[])
 {
	 
 	if (!init_SDL()){
 		fprintf(stderr, "Could not initialize SDL!\n");
 		return -1;
 	}
 	if (!cap.isOpened()){
 		fprintf(stderr, "Failed to load file!\n");
 		return -1;
 	}
 	if (!init_CameraSettings()){
 		printf("failed to load settings\n");
 		return -1;
 	}
 	
 	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
 	{
		printf("Error opening audio: %s\n", Mix_GetError());
	}
 	
 	gMusic = Mix_LoadMUS("SongLibrary/Polaris.mp3");
 	if (gMusic == NULL)
		printf("Error loading music: %s\n", Mix_GetError());
		
	Mix_PlayMusic(gMusic, -1);
 	
 	initSongPlayer();
 	loadSong((char *)currentSong().c_str());
 	//songThread();

 	SDLCameraThread = SDL_CreateThread(cameraWorker, "Backup Camera Thread", NULL);
 	SDLMusicThread = SDL_CreateThread(songThread, "Music Playing Thread", NULL);

	int screenUpdate = 0;

 	while (!quit)
 	{
 		
		screenUpdate = show_Camera(&threadImage1);
		processEvents();
		if (screenUpdate == 1){
			SDL_RenderPresent(renderer);
		}
	}

	SDL_WaitThread(SDLCameraThread, NULL);
	SDL_WaitThread(SDLMusicThread, NULL);
	return 0;
}

void processEvents()
{
	SDL_Event event;
 		while (SDL_PollEvent(&event))
 		{
	 		switch(event.type)
	 		{
	 			case SDL_QUIT:
	 				printf("SDL_QUIT was called\n");
	 				signalToQuit();
	 				close();
		 			break;

	 			case SDL_KEYDOWN:
	 				switch(event.key.keysym.sym) 
	 				{
				 		case SDLK_ESCAPE: 
				 		 	printf("Esc was Pressed!\n");
				 		 	signalToQuit();
				 		 	close();
							break;
						case SDLK_LEFT:
							printf("Left arrow was Pressed!\n");
							previousSong();
							break;
						case SDLK_RIGHT:
							printf("Right arrow was Pressed!\n");
							nextSong();
							break;
			    	}
	 		}
 		}
}

/* Signals all the threads to quit and then waits for the threads */
void signalToQuit()
{
	quit = true;
	songQuit();	
	SDL_WaitThread(SDLCameraThread, NULL);
	SDL_WaitThread(SDLMusicThread, NULL);
}

/* Cleans up and should free everything used in the program*/
void close()
{
	Mix_FreeMusic(gMusic);
	Mix_CloseAudio();
	closeSongPlayer();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;
	SDL_Quit();
	exit(0);
}
