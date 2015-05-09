#include <highgui.h>
#include <cv.h>
#include "opencv2/opencv.hpp"
#include <SongLoader.h>

//for the rasperry pi
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif


extern "C" {
	#include <SDL.h>
	#include <SDL_thread.h>
}

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

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

Mix_Music *gMusic = NULL;
TTF_Font *font = NULL;
SDL_Texture *textTexture = NULL;
SDL_Texture *volumeTexture = NULL;
int textWSong = 0;
int textHSong = 0;
int textWVol = 0;
int textHVol = 0;
SongLoader loader;
int songCount;
int quit;
int volume = 128;
int songIndex = 0;
string songArray[20];
void close();
int cameraWorker(void* data);
int gpsWorker(void* data);
bool setTextSong(std::string text, SDL_Color color);
bool setTextVolume(std::string text, SDL_Color color);
bool loadFont();

IplImage threadImage1;

bool updatedImage1 = false;

VideoCapture cap(0);
int cameraHeight;
int cameraWidth;

Mat frame;

SDL_Renderer* renderer = NULL;
SDL_Window* window = NULL;
SDL_Rect videoRect;


/***********************************************************************
/*							SDL functions 
/***********************************************************************/
// Initializes SDL window / renderer for use

bool init_SDL()
{


	bool success = true;
	if (SDL_Init(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) < 0 || TTF_Init() < 0)
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else 
	{
		window = SDL_CreateWindow("Video Application", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL)
		{
			printf("error");
			success = false;
		}
		else {
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (renderer == NULL)
			{
				printf("Renderer could not be created. SDL_Error: %s \n", SDL_GetError());
				success = false;
			}
			else 
			{

				int MusicFlags = MIX_INIT_MP3;
	
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
				if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
				{
					printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
					success = false;
				}



			}
		}
	}
	videoRect.x = 0;
	videoRect.y = 0;
	videoRect.w = 1280;	//640
	videoRect.h = 720;	//480

	cameraWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	cameraHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	loadFont();
	printf("Camera Width%d, Camera Height %d \n",cameraWidth,cameraHeight);

	return success;
}

bool loadFont()
{
	bool success = true;
	font = TTF_OpenFont("assets/LiberationSerif-Regular.ttf", 20);
	if (font == NULL)
	{
		printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
		success = false;
	}
	return success;
}

bool loadSong(std::string name = "")
{
	if (name == "")
		return false;
	if (gMusic != NULL)
	{
		Mix_FreeMusic(gMusic);
		gMusic = NULL;
	}
	bool success = true;
	//Load music
	std::string path = "assets/" + name;
	gMusic = Mix_LoadMUS(path.c_str());
	if( gMusic == NULL )
	{
		printf( "Failed to load beat music! SDL_mixer Error: %s\n", Mix_GetError() );
		success = false;
	}
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
 
 int main(int argc, char* argv[])
 {
	
 	if (!init_SDL())
 	{
 		fprintf(stderr, "Could not initialize SDL!\n");
 		return -1;
 	}
  
 	if (!cap.isOpened())
 	{
 		fprintf(stderr, "Failed to load file!\n");
 		return -1;
 	}
 	SDL_RenderClear(renderer);
 	songCount = loader.readSongNames(songArray);
	int N = (sizeof(songArray) / sizeof(songArray[0]));
	std::sort(songArray, songArray + songCount);
	if( !loadSong(songArray[songIndex]))
	{ // Dummy value added to mimic future usage of loadSong function
		printf( "Failed to load media!\n" );
	}

 	SDL_Thread* threadID = SDL_CreateThread(cameraWorker, "Backup Camera Thread", NULL);

	int screenUpdate = 0;
	int threadReturnValue;	

 	while (!quit)
 	{
 		SDL_Event event;
 		while (SDL_PollEvent(&event)){
	 		switch(event.type)
	 		{
	 			case SDL_QUIT:
		 			quit = 1;
		 			close();
		 			SDL_Quit();
		 			exit(0);
		 			break;
		 		case SDL_KEYUP:
					switch(event.key.keysym.sym) {
						case SDLK_2:
							break;
							
						case SDLK_9:
							//If there is no music playing
							if( Mix_PlayingMusic() == 0 )
							{
								//Play the music
								Mix_PlayMusic( gMusic, -1 );
							}
							//If music is being played
							else
							{
								//If the music is paused
								if( Mix_PausedMusic() == 1 )
								{
									//Resume the music
									Mix_ResumeMusic();
								}
								//If the music is playing
								else
								{
									//Pause the music
									Mix_PauseMusic();
								}
							}
							break;
							
							case SDLK_3:
							if (songIndex > 0)
								songIndex--;
							else
								songIndex = 0;
							if( !loadSong(songArray[songIndex]))
							{ 	
								printf( "Failed to load media!\n" );
							}
							else
							{
								Mix_PlayMusic( gMusic, -1 );
							}
							SDL_RenderClear(renderer);
							break;
						case SDLK_4:
							if (songIndex < songCount -1)
							{
								songIndex++;
							}
							else
								songIndex = (songCount - 1);
							if( !loadSong(songArray[songIndex]))
							{ 
								printf( "Failed to load media at %d!\n", songIndex );
							}
							else
							{
								Mix_PlayMusic( gMusic, -1 );
							}
							SDL_RenderClear(renderer);
							break;
						case SDLK_5:
							printf("%d\n",Mix_SetMusicPosition(30.0));
							break;
						default:
							break;
					}

	 			case SDL_KEYDOWN:
	 				switch(event.key.keysym.sym) {
				 		case SDLK_ESCAPE: 
				 			SDL_SetRenderDrawColor( renderer, 0x00, 0x00, 0xFF, 0xFF );   
				 		 	printf("Esc was Pressed!");
				       	 	quit = true;
				       	 	SDL_WaitThread(threadID, &threadReturnValue);
				       	 	printf("\nThread returned value: %d", threadReturnValue);
				       	 	close();
				 			SDL_Quit();
				 			exit(0);
				        	break;
						
						case SDLK_1:
							SDL_RenderClear(renderer);
							if (volume < 128)
								volume+= 4;
							Mix_VolumeMusic(volume);
							break;
						case SDLK_2:
							SDL_RenderClear(renderer);
							if (volume > 0)
								volume-= 4;
							Mix_VolumeMusic(volume);
							break;
							
						
						case SDLK_0:
							//Stop the music
							Mix_HaltMusic();
							break;

			        	default:
			 				break;
			    }

	 		}
 		}
		
		screenUpdate = show_Camera(&threadImage1);
		
		if (screenUpdate == 1){
			SDL_Color textColor = { 0xFF, 0xFF, 0xFF };
			setTextSong(songArray[songIndex], textColor);
			SDL_RendererFlip flip = SDL_FLIP_NONE;
			SDL_Rect textQuadSong = { 100, 720, textWSong, textHSong};
			SDL_RenderCopyEx(renderer, textTexture, NULL, &textQuadSong ,0, NULL, flip);
			
			std::string volString("Volume: " + std::to_string( (volume * 100 / 128)));
			setTextVolume(volString, textColor);
			SDL_Rect textQuadVol = { 700, 720, textWVol, textHVol};
			SDL_RenderCopyEx(renderer, volumeTexture, NULL, &textQuadVol ,0, NULL, flip);
			SDL_RenderPresent(renderer);
		}

	}


	SDL_WaitThread(threadID, NULL);
	return 0;
}

	void close()
	{
		Mix_FreeMusic(gMusic);
		Mix_CloseAudio();
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		window = NULL;
		renderer = NULL;
		gMusic = NULL;
		SDL_Quit();
	}
	
bool setTextSong(std::string text, SDL_Color color)
{
	
	if (textTexture != NULL)
		SDL_DestroyTexture(textTexture);
	SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
	if (textSurface == NULL)
	{
		 printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() ); 
	}
	else
	{ 
		//Create texture from surface pixels 
		textTexture = SDL_CreateTextureFromSurface( renderer, textSurface ); 
		if( textTexture == NULL ) 
		{ 
			printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() ); 
		} 
		else 
		{ 
			//Get image dimensions 
			textWSong = textSurface->w; 
			textHSong = textSurface->h; 
		} 
		//Get rid of old surface 
		SDL_FreeSurface( textSurface ); 
	} 
		//Return success 
	return textTexture != NULL;
}

bool setTextVolume(std::string text, SDL_Color color)
{
	
	if (volumeTexture != NULL)
		SDL_DestroyTexture(volumeTexture);
	SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
	if (textSurface == NULL)
	{
		 printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() ); 
	}
	else
	{ 
		//Create texture from surface pixels 
		volumeTexture = SDL_CreateTextureFromSurface( renderer, textSurface ); 
		if( volumeTexture == NULL ) 
		{ 
			printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() ); 
		} 
		else 
		{ 
			//Get image dimensions 
			textWVol = textSurface->w; 
			textHVol = textSurface->h; 
		} 
		//Get rid of old surface 
		SDL_FreeSurface( textSurface ); 
	} 
		//Return success 
	return volumeTexture != NULL;
}
