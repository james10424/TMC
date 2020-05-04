#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#define checkquit if (key_pressed == 'q') break
#define yield sleep(1)

struct timestamp {
    int id;
    int nitems;
    int duration;
    char* s_duration;
    int* times;
    char* title;
    char** names;
};

struct timestamp wild, pat1982, totally, pat1983, belinda1988, belinda1990, peppermint;

struct timestamp* songs;
int num_songs;

struct timestamp cur_song;

WINDOW* w_progress;
WINDOW* w_tracks;
WINDOW* w_playlist;

int key_pressed = 0; // 0 is nothing
int cur_time = 0; // time in s
int cur_track = 0;
int cur_playing = 0; // within the video 

const char* position_cmd = "printf \"%.0f\" `osascript -e 'tell app \"itunes\" to player position'`";
int get_position() {
    FILE* fp = popen(position_cmd, "r");
    int s;
    fscanf(fp, "%d", &s);
    pclose(fp);
    return s;
}

const char* track_cmd = "osascript -e 'tell application \"iTunes\" to set current_track to the current track' | awk '{print $4}'";
int get_track() {
    FILE* fp = popen(track_cmd, "r");
    int s;
    fscanf(fp, "%d", &s);
    pclose(fp);
    return s; 
}

const char* playlist_cmd = "osascript -e 'tell application \"iTunes\"' -e 'set n_track to count of tracks of current playlist' -e 'set all_tracks to every track of current playlist' -e 'n_track & all_tracks' -e 'end Tell' | sed $'s/, /\\\n/g' | awk '{if (NR>1) {print $4} else {print $1}}'";
/* first element = size */
int* get_playlist() {
    FILE* fp = popen(playlist_cmd, "r");
    int n;
    fscanf(fp, "%d", &n); // items
    int* playlist = malloc((n + 1) * sizeof(int));
    playlist[0] = n;
    for (int i = 1; i < n + 1; i++) {
        fscanf(fp, "%d", &playlist[i]);
    }
    pclose(fp);
    return playlist;
}

const char* get_playlist_id_cmd = "osascript -e 'tell application \"iTunes\" to set a to selection' | awk '{print $9}'";
int get_playlist_id() {
    FILE* fp = popen(get_playlist_id_cmd, "r");
    int s;
    fscanf(fp, "%d", &s);
    pclose(fp);
    return s; 
}

const char* play_track_cmd = //"osascript -e 'tell application \"iTunes\"' -e 'stop' -e 'play track id %d of playlist id %d' -e 'end tell'";
"osascript"
" -e 'tell application \"System Events\"'"
    " -e 'tell process \"iTunes\"'"
        " -e 'click UI element 1 of row %d of table 1 of scroll area 2 of splitter group 1 of window \"iTunes\"'"
    " -e 'end tell'"
" -e 'end tell' >/dev/null";

void play_track(int i) {
    char* song;
    asprintf(&song, play_track_cmd, i + 1);
    system(song);
    free(song);
}

const char* jump_cmd = "osascript -e 'tell app \"itunes\" to set player position to %d'";
void jump(int i) {
    char* song;
    asprintf(&song, jump_cmd, cur_song.times[i]);
    system(song);
    free(song);
}

int max(int a, int b) {
    return a > b ? a : b;
}

int min(int a, int b) {
    return a < b ? a : b;
}

void get_playing() {
    int t = cur_time;
    int i;
    for (i = 0; i < cur_song.nitems; i++) {
        if (t < cur_song.times[i]) {
            break;
        }
    }
    cur_playing = max(i - 1, 0);
}

void draw_default_track(WINDOW* w) {
    wmove(w, 0, 0);
    for (int i = 0; i < cur_song.nitems; i++) {
        wprintw(w, "%s\n", cur_song.names[i]);
    }
}

void refresh_track(WINDOW* w) {
    // wmove(w, last_playing, 0);
    // wclrtoeol(w);
    // mvwprintw(w, last_playing, 0, cur_song.names[last_playing]);
    
    wattron(w, A_BOLD);
    mvwprintw(w, cur_playing, 0, "> %s", cur_song.names[cur_playing]);
    wattroff(w, A_BOLD);
}

bool update_track(WINDOW* w) {
    for (int i = 0; i < num_songs; i++) {
        if (cur_track == songs[i].id) {
            int l = strlen(songs[i].title);
            cur_song = songs[i];
            // 10 to 65, 37.5 mid
            mvwprintw(w, 0, 40 - (l >> 1), songs[i].title);
            return TRUE; // there is a known song playing
        }
    }
    // unknown
    mvwprintw(w, 0, 40 - 3, "unknown");
    return FALSE; // unknown song
}

int _check_track(int id) {
    for (int j = 0; j < num_songs; j++) {
        if (songs[j].id == id) {
            return j;
        }
    }
    return -1;
}

void update_playlist(WINDOW* w) {
    int* playlist = get_playlist();
    int n = playlist[0];
    int j;
    wclear(w);
    wmove(w, 0, 0);
    for (int i = 1; i < n + 1; i++) {
        if ((j = _check_track(playlist[i])) >= 0) {
            if (playlist[i] == cur_song.id) {
                // sort
                if (i - 1 != j) {
                    struct timestamp tmp = songs[i - 1];
                    songs[i - 1] = cur_song;
                    songs[j] = tmp;
                }

                wattron(w, A_BOLD);
                wprintw(w, "> %d %s\n", i - 1, songs[j].title);    
                wattroff(w, A_BOLD);
            } else {
                wprintw(w, "%d %s\n", i - 1, songs[j].title);
            }
        } else {
            wprintw(w, "%d unknown\n", i - 1);
        }
    }
    free(playlist);
}

void update_progress() {
    int t = get_position();
    cur_time = t;
    int m = t/60;
    int s = t%60;

    int remain = cur_song.duration - t;
    int r_m = remain/60;
    int r_s = remain%60;
    // 5 chars
    wclear(w_progress);
    mvwprintw(w_progress, 0, 10, "<-[a]--");
    mvwprintw(w_progress, 0, 63, "--[d]->");
    
    mvwprintw(w_progress, 0, 0, "%s%d:%s%d", m < 10 ? "0" : "", m, s < 10 ? "0" : "", s);
    mvwprintw(w_progress, 0, 74, "-%s%d:%s%d", r_m < 10 ? "0" : "", r_m, r_s < 10 ? "0" : "", r_s);
    wmove(w_progress, 1, 0);
    // progress bar
    double percent = (double)cur_time / cur_song.duration;
    int progress = percent * 80;
    for (int i = 0; i < progress; i++)
        wprintw(w_progress, "-");
    wprintw(w_progress, ">");
    for (int i = progress + 1; i < 80; i++)
        wprintw(w_progress, ".");
}

void* update(void* arg) {
    // draw controls
    cur_track = get_track();
    bool known = FALSE;
    if ((known = update_track(w_progress))) {
        draw_default_track(w_tracks);
    }

    while (1) {
        checkquit;
        if (known) {
            update_progress();
        }
        // tracks
        cur_track = get_track();
        if ((known = update_track(w_progress))) {
            wclear(w_tracks);
            draw_default_track(w_tracks);
            get_playing();
            refresh_track(w_tracks);
        }

        // playlist
        update_playlist(w_playlist);

        wrefresh(w_progress);
        wrefresh(w_tracks);
        wrefresh(w_playlist);

        yield;
    }
    return NULL;
}

void handle_mouse() {
    MEVENT e;
    if (getmouse(&e) == OK) {
        if (e.x < 40 &&
            e.y < cur_song.nitems &&
            e.x < strlen(cur_song.names[e.y])) {
            jump(e.y);
        }
        if (e.x > 40 &&
            e.y < num_songs &&
            e.y < strlen(songs[e.y].title)) {
            play_track(e.y);
        }
    }
}

void* keypress(void* arg) {
    while (1) {
        key_pressed = getch();
        switch (key_pressed) {
            case KEY_MOUSE:
                handle_mouse();
                break;
            case 'a':
                jump(max(cur_playing - 1, 0));
                break;
            case 'd':
                jump(min(cur_playing + 1, cur_song.nitems - 1));
                break;
        }
        checkquit;
    }
    return NULL;
}

int main() {
    wild.id = 1097;
    wild.nitems = 13;
    wild.duration = 3141;
    wild.s_duration = "52:21";
    wild.title = "Wild At The Greek 1984";
    wild.times = (int[]) {64, 329, 549, 828, 996, 1232, 1433, 1701, 1939, 2171, 2379, 2640, 2808};
    wild.names = (char*[]) {
        " 0 - Head Over Heels",
        " 1 - Our Lips Are Sealed",
        " 2 - Forget That Day",
        " 3 - We Got The Beat",
        " 4 - Turn To You",
        " 5 - Tonite",
        " 6 - Your Thought",
        " 7 - Yes Or No",
        " 8 - I'm With You",
        " 9 - This Town",
        "10 - Can't Stop The World",
        "11 - Vacation",
        "12 - I'm The Only One"
    };

    pat1982.id = 1099;
    pat1982.nitems = 15;
    pat1982.duration = 4284;
    pat1982.s_duration = "71:24";
    pat1982.title = "Pat Benatar 1982";
    pat1982.times = (int[]) {0, 215, 411, 678, 960, 1319, 1610, 1989, 2248, 2491, 2859, 3050, 3414, 3732, 4051};
    pat1982.names = (char*[]) {
        " 0 Treat Me Right",
        " 1 You Better Run",
        " 2 I Need a Lover",
        " 3 The Victim",
        " 4 Precious Time",
        " 5 Anxiety (Get Nervous)",
        " 6 In the Heat of the Night",
        " 7 Little Too Late",
        " 8 Fire and Ice",
        " 9 Promises in the Dark",
        "10 Hit Me with Your Best Shot",
        "11 Heartbreaker",
        "12 Hell Is for Children",
        "13 No You Don't",
        "14 Just Like Me",
    };

    totally.id = 1098;
    totally.duration = 4366;
    totally.s_duration = "72:46";
    totally.nitems = 19;
    totally.title = "Totally GoGo's 1981";
    totally.times = (int[]) {160, 340, 623, 851, 1082, 1240, 1517, 1682, 1917, 2303, 2501, 2718, 2954, 3150, 3272, 3514, 3637, 3932, 4117};
    totally.names = (char*[]) {
        " 0 Skidmarks On My Heart",
        " 1 How Much More",
        " 2 Tonite",
        " 3 Fading Fast",
        " 4 London Boys",
        " 5 Cool Jerk",
        " 6 Automatic",
        " 7 Lust To Love",
        " 8 Can't Stop The World",
        " 9 This Town",
        "10 Vacation (early version)",
        "11 You Can't Walk In Your Sleep",
        "12 Our Lips Are Sealed",
        "13 Let's Have A Party",
        "14 We Got The Beat",
        "15 Surfing And Spying",
        "16 Beatnik Beach",
        "17 The Way You Dance",
        "18 (Remember) Walking In The Sand",
    };

    pat1983.id = 1100;
    pat1983.nitems = 13;
    pat1983.duration = 3458;
    pat1983.s_duration = "57:38";
    pat1983.title = "Pat Benatar 1983";
    pat1983.times = (int[]) {13, 260, 475, 665, 918, 1185, 1397, 1636, 1870, 2350, 2635, 2899, 3100};
    pat1983.names = (char*[]) {
        " 0 Anxiety (Get Nervous)",
        " 1 Fire And Ice",
        " 2 You Better Run",
        " 3 Little Too Late",
        " 4 Fight It Out",
        " 5 Looking For A Stranger",
        " 6 I Want Out",
        " 7 We Live For Love",
        " 8 In The Heat Of The Night",
        " 9 Shadows Of The Night",
        "10 Heartbreaker",
        "11 Hit Me With Your Best Shot",
        "12 Hell Is For Children"
    };

    belinda1988.id = 1101;
    belinda1988.nitems = 13;
    belinda1988.duration = 3143;
    belinda1988.s_duration = "52:23";
    belinda1988.title = "Belinda Carlisle 1988";
    belinda1988.times = (int[]) {0, 232, 502, 791, 1004, 1287, 1581, 1780, 1939, 2120, 2384, 2688, 2933};
    belinda1988.names = (char*[]) {
        " 0 Mad About You",
        " 1 Lust For Love",
        " 2 I Get Weak",
        " 3 Fool For Love",
        " 4 Circle In The Sand",
        " 5 World Without You",
        " 6 Nobody Owns Me",
        " 7 Our Lips Are Sealed",
        " 8 Vacation",
        " 9 Heaven Is A Place On Earth",
        "10 Love Never Dies",
        "11 Head Over Heels",
        "12 We Got The Beat",
    };

    belinda1990.id = 1102;
    belinda1990.nitems = 17;
    belinda1990.duration = 4729;
    belinda1990.s_duration = "78:49";
    belinda1990.title = "Belinda Carlisle 1990";
    belinda1990.times = (int[]) {40, 329, 640, 872, 1161, 1407, 1680, 1925, 2209, 2547, 2837, 3118, 3384, 3713, 3868, 4105, 4394};
    belinda1990.names = (char*[]) {
        " 0 Runaway Horses",
        " 1 Summer Rain",
        " 2 (We Want) The Same Thing",
        " 3 Whatever It Takes",
        " 4 Mad About You",
        " 5 Circle In The Sand",
        " 6 Nobody Owns Me",
        " 7 I Get Weak",
        " 8 Valentine",
        " 9 La Luna",
        "10 Vision Of You",
        "11 Leave A Light On",
        "12 Heaven Is A Place On Earth",
        "13 Our Lips Are Sealed",
        "14 We Got The Beat",
        "15 World Without You",
        "16 Shades Of Michaelangel",
    };

    peppermint.id = 3242;
    peppermint.nitems = 15;
    peppermint.duration = 2866;
    peppermint.s_duration = "47:46";
    peppermint.title = "Peppermint Lounge";
    peppermint.times = (int[]) {0,144,340,579,838,1046,1247,1419,1668,1831,1955,2148,2321,2536,
2746};
    peppermint.names = (char*[]) {
        " 0 Beatnik Beach",
        " 1 How Much More",
        " 2 Our Lips Are Sealed",
        " 3 He's So Strange",
        " 4 Can't Stop The World",
        " 5 You Can't Walk In Your Sleep",
        " 6 Cool Jerk",
        " 7 Lust To Love",
        " 8 London Boys",
        " 9 Surfing And Spying",
        "10 (Remember) Walking In The Sand",
        "11 Skidmarks On My Heart",
        "12 We Got The Beat",
        "13 This Town",
        "14 Let's Have A Party",
    };

    songs = (struct timestamp[]) {wild, pat1982, totally, pat1983, belinda1988, belinda1990, peppermint};
    num_songs = 7;

    initscr(); // init screen
    raw(); // no enter
    noecho(); // no print
    curs_set(0); // no cursor
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    // update progress bar thread
    pthread_t t_progress, t_keypress;
    w_progress = newwin(2, 80, 22, 0);
    w_tracks = newwin(20, 40, 0, 0);
    w_playlist = newwin(20, 40, 0, 40);
    refresh(); // for new window to show up
    pthread_create(&t_keypress, NULL, &keypress, NULL);
    pthread_create(&t_progress, NULL, &update, NULL);

    void* status;
    pthread_join(t_keypress, &status);
    pthread_cancel(t_progress);

    endwin();
    return 0;
}