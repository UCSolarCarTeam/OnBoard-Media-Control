#include <string>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include "I_song_loader_observer.h"
//#include "I_io_event_observer.h"
#include "Observable.h"
//#include "config.h"
#include <algorithm>
#include <vector>
#include <random>

class SongLoader: public Observable<I_SongLoaderObserver>//,public I_IoEventObserver 
{
private:
    std::vector<std::string> files;
    int current_song_index;
    bool check_library;
    bool read_song_names(std::string dir);
    void notify_listeners(std::string song_path);

public:
    SongLoader(std::string filepath);
    bool song_exists();
    std::string next_song_name();
    std::string previous_song_name();
    void io_event(int io_command);
    void next_song();
    void previous_song();
    void shuffle_songs();
};
