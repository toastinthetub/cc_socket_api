#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include "../../../src/cc_socket_api.h"

#define MAX_MESSAGE 1024
#define NICKNAME_SIZE 32
#define MAX_CHAT_HISTORY 1000
#define INPUT_HEIGHT 3

char nickname[NICKNAME_SIZE];
WINDOW *chat_win;
WINDOW *input_win;
char chat_history[MAX_CHAT_HISTORY][MAX_MESSAGE];
int chat_history_pos = 0;
int chat_history_count = 0;
pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_windows() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    chat_win = newwin(max_y - INPUT_HEIGHT, max_x, 0, 0);
    scrollok(chat_win, TRUE);
    wrefresh(chat_win);

    input_win = newwin(INPUT_HEIGHT, max_x, max_y - INPUT_HEIGHT, 0);
    scrollok(input_win, TRUE);
    box(input_win, 0, 0);
    wmove(input_win, 1, 1);
    wrefresh(input_win);
}

void add_to_chat(const char* message) {
    pthread_mutex_lock(&screen_mutex);

    strncpy(chat_history[chat_history_pos], message, MAX_MESSAGE - 1);
    chat_history_pos = (chat_history_pos + 1) % MAX_CHAT_HISTORY;
    if (chat_history_count < MAX_CHAT_HISTORY) chat_history_count++;

    werase(chat_win);

    int start = (chat_history_pos - chat_history_count + MAX_CHAT_HISTORY) % MAX_CHAT_HISTORY;
    for (int i = 0; i < chat_history_count; i++) {
        int pos = (start + i) % MAX_CHAT_HISTORY;
        wprintw(chat_win, "%s\n", chat_history[pos]);
    }

    wrefresh(chat_win);
    wrefresh(input_win);

    pthread_mutex_unlock(&screen_mutex);
}

void handle_message(const char* data, int len) {
    char message[MAX_MESSAGE];
    strncpy(message, data, len < MAX_MESSAGE - 1 ? len : MAX_MESSAGE - 1);
    message[len < MAX_MESSAGE - 1 ? len : MAX_MESSAGE - 1] = '\0';

    add_to_chat(message);
}

void cleanup_windows() {
    delwin(chat_win);
    delwin(input_win);
    endwin();
}

int main() {
    printf("enter your nickname: ");
    fgets(nickname, NICKNAME_SIZE, stdin);
    nickname[strcspn(nickname, "\n")] = 0;

    if (init_socket_api(handle_message) < 0) {
        printf("failed to initialize socket API\n");
        return 1;
    }

    init_windows();

    add_to_chat("chat initialized, you are logged in as:");
    add_to_chat(nickname);
    add_to_chat("commands: /connect <ip>, /quit");

    char input[MAX_MESSAGE];
    char formatted_msg[MAX_MESSAGE + NICKNAME_SIZE + 4];
    int input_pos = 0;
    int ch;

    while (1) {
        ch = wgetch(input_win);

        if (ch == '\n') {
            input[input_pos] = '\0';

            wmove(input_win, 1, 1);
            wclrtoeol(input_win);
            box(input_win, 0, 0);
            wrefresh(input_win);

            if (input_pos == 0) {
                input_pos = 0;
                continue;
            }

            if (strcmp(input, "/quit") == 0) {
                break;
            }
            else if (strncmp(input, "/connect ", 9) == 0) {
                char* ip = input + 9;
                if (connect_to(ip) < 0) {
                    snprintf(formatted_msg, sizeof(formatted_msg), "failed to connect to %s", ip);
                } else {
                    snprintf(formatted_msg, sizeof(formatted_msg), "connected to %s", ip);
                }
                add_to_chat(formatted_msg);
            }
            else {
                snprintf(formatted_msg, sizeof(formatted_msg), "%s: %s", nickname, input);
                broadcast(formatted_msg, strlen(formatted_msg));
            }

            input_pos = 0;
        }
        else if (ch == KEY_BACKSPACE || ch == 127) {
            if (input_pos > 0) {
                input_pos--;
                wmove(input_win, 1, input_pos + 1);
                wdelch(input_win);
                box(input_win, 0, 0);
                wrefresh(input_win);
            }
        }
        else if (input_pos < MAX_MESSAGE - 1 && ch >= 32 && ch <= 126) {
            input[input_pos++] = ch;
            wechochar(input_win, ch);
            box(input_win, 0, 0);
            wrefresh(input_win);
        }
    }

    cleanup_windows();
    cleanup_socket_api();
    printf("glub glub\n");

    return 0;
}
