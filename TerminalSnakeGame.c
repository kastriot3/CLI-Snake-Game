#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#define INITIAL_SNAKE_SIZE 3   // The initial size of the snake


/* The trophies:
Trophies are represented by a digit randomly chosen from 1 to 9.
There's always exactly one trophy in the snake pit at any given moment.
When the snake eats the trophy, its length is increased by the corresponding number of characters.
A trophy expires after a random interval from 1 to 9 seconds.
A new trophy is shown at a random location on the screen after the previous one has either expired or is eaten by the snake.
*/


/* The gameplay:
The snake dies and the game ends if:
It runs into the border; or
It runs into itself; or
The user attempts to reverse the snake's direction.
The user wins the game if the snake's length grows to the length equal to half the perimeter of the border. */ 


int get_terminal_width();
int get_terminal_height();

typedef struct {
    int x, y;    // The coordinates of a point
} Point;

typedef struct {
    int x, y;
} Trophy;

Point snake[1000];   // The array to store the coordinates of the snake
int snake_length = INITIAL_SNAKE_SIZE;   // The length of the snake
int snake_direction;   // The moving direction of the snake
int new_direction; // New moving direction of the snake

Trophy trophy;         // The trophy
int trophy_val;        // The value of the trophy
int trophy_exists;     // Flag to track the presence of an active trophy
time_t trophy_time;    // The time when the trophy was dropped
int trophy_expiration; // The time in seconds until the trophy expires

int terminal_width;
int terminal_height;

int kbhit() {
    struct termios original_term, modified_term;
    int character;
    int original_flags;

    tcgetattr(STDIN_FILENO, &original_term);   // Get the original terminal settings
    modified_term = original_term;
    modified_term.c_lflag &= ~(ICANON | ECHO); // Modify the terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &modified_term);
    original_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, original_flags | O_NONBLOCK);  // Set the terminal to non-blocking mode

    character = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &original_term);   // Restore the original terminal settings
    fcntl(STDIN_FILENO, F_SETFL, original_flags);

    if(character != EOF) {
        ungetc(character, stdin);
        return 1;
    }

    return 0;
}


int is_snake_body(int x, int y) {
    for (int i = 0; i < snake_length; i++) { // Start from 0 to include the snake's head
        if (snake[i].x == x && snake[i].y == y)
            return 1;
    }
    return 0;
}

/*
Rocky

*/

void drop_trophy() {
    // Get a list of all possible locations in the snake pit
    int pitWidth = get_terminal_width() - 2;
    int pitHeight = get_terminal_height() - 4;
    int numLocations = pitWidth * pitHeight;
    Point* locations = malloc(numLocations * sizeof(Point));

    for (int y = 0; y < pitHeight; y++) {
        for (int x = 0; x < pitWidth; x++) {
            locations[y * pitWidth + x].x = x + 1;
            locations[y * pitWidth + x].y = y + 1;
        }
    }

    // Remove the locations that contain the snake's body
    for (int i = 0; i < snake_length; i++) {
        for (int j = 0; j < numLocations; j++) {
            if (locations[j].x == snake[i].x && locations[j].y == snake[i].y) {
                // Remove this location by swapping it with the last location and reducing the count
                locations[j] = locations[numLocations - 1];
                numLocations--;
                break;
            }
        }
    }

    // Choose a random location from the remaining valid locations
    int index = rand() % numLocations;
    trophy.x = locations[index].x;
    trophy.y = locations[index].y;

    // Free the memory for the locations array
    free(locations);

    trophy_val = (rand() % 9) + 1;
    trophy_exists = 1;
    trophy_time = time(NULL);
    trophy_expiration = (rand() % 9) + 1;
}

int get_terminal_width() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

int get_terminal_height() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
}


//sets up the game, handles terminal resizes
void setup() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    long time_in_micro = tv.tv_sec * 1000000 + tv.tv_usec;

    srand(time_in_micro);

    terminal_width = get_terminal_width();
    terminal_height = get_terminal_height();

    int i;
    for (i = 0; i < INITIAL_SNAKE_SIZE; i++) {
        snake[i].x = terminal_width / 2 - i;     // Adjust the initial snake position based on terminal width
        snake[i].y = terminal_height / 2;        // Adjust the initial snake position based on terminal height
    }

    snake_direction = rand() % 4;

    trophy_exists = 0;

    drop_trophy();
}


//prints current game state (updated until game over)
void draw() {
    printf("\033[H\033[J");  // Clear the console

    int i, j;

    printf("╔");
    for (i = 0; i < get_terminal_width() - 2; i++)
        printf("═");
    printf("╗\n");

    int topSectionHeight = (get_terminal_height() - 6) / 2;  // Calculate the height of the top section

    for (i = 0; i < topSectionHeight; i++) {
        printf("║");
        for (j = 0; j < get_terminal_width() - 2; j++) {
            char ch = ' ';

            if (i == trophy.y && j == trophy.x) {
                if (trophy_exists && trophy_val >= 1 && trophy_val <= 9) {
                    ch = '0' + trophy_val;
                } else {
                    ch = '?';
                }
            } else {
                for (int k = 0; k < snake_length; k++) {
                    if (snake[k].x-1 == j && snake[k].y-1 == i) {
                        ch = (k == 0) ? '@' : '*';
                        break;
                    }
                }
            }

            printf("%c", ch);
        }
        printf("║\n");
    }

    for (i = 0; i < get_terminal_height() - topSectionHeight - 4; i++) {
        printf("║");
        for (j = 0; j < get_terminal_width() - 2; j++) {
            char ch = ' ';

            if (i + topSectionHeight == trophy.y && j == trophy.x) {
                if (trophy_exists && trophy_val >= 1 && trophy_val <= 9) {
                    ch = '0' + trophy_val;
                } else {
                    ch = '?';
                }
            } else {
                for (int k = 0; k < snake_length; k++) {
                    if (snake[k].x == j + 1 && snake[k].y == i + topSectionHeight + 1) {
                        ch = (k == 0) ? '@' : '*';
                        break;
                    }
                }
            }

            printf("%c", ch);
        }
        printf("║\n");
    }

    printf("╚");
    for (i = 0; i < get_terminal_width() - 2; i++)
        printf("═");
    printf("╝\n");
}


void handle_input() {
    if (kbhit()) {
        switch (getchar()) {
            case 'w': snake_direction = 0; break;
            case 'd': snake_direction = 1; break;
            case 's': snake_direction = 2; break;
            case 'a': snake_direction = 3; break;
        }
    }
}

void grow_snake(int growth) {
    for (int i = 0; i < growth; i++) {
        // Append to tail of the snake
        snake[snake_length + i] = snake[snake_length - 1];
    }
    snake_length += growth;
}

void update() {
    int i;
    int has_eaten = 0;

    Point old_tail = snake[snake_length - 1]; // save the last position before moving the snake

    // move the snake body
    for (i = snake_length - 1; i > 0; i--) {
        if (snake[i].x == snake[0].x && snake[i].y == snake[0].y) {
            printf("GAME OVER!\n");
            exit(0);
        }
        snake[i] = snake[i - 1];
    }

    // move the snake head
    switch (snake_direction) {
        case 0: snake[0].y--; break;
        case 1: snake[0].x++; break;
        case 2: snake[0].y++; break;
        case 3: snake[0].x--; break;
    }

    // check for out of bounds
    if (snake[0].x < 1 || snake[0].x > get_terminal_width() - 2 || snake[0].y < 1 || snake[0].y > get_terminal_height() - 2) {
        printf("Game Over\n");
        exit(0);
    }

    // check for collision with trophy
    if (snake[0].x == trophy.x+1 && snake[0].y == trophy.y+1) {
        // snake eats the trophy
        trophy_exists = 0;
        has_eaten = 1;
        
        // extend the snake
        for(i = 0; i < trophy_val; i++) {
            snake_length++;
            snake[snake_length-1] = old_tail;
        }
    }

    // check for trophy expiration and drop new one if needed
    if (!has_eaten && (!trophy_exists || difftime(time(NULL), trophy_time) > trophy_expiration)) {
        drop_trophy();
    }

    // check for winning condition
    if (snake_length >= (get_terminal_width() - 4) * (get_terminal_height() - 6)) {
        printf("You win!\n");
        exit(0);
    }
}


int main() {
    char c;
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║                                                ║\n");
    printf("║                                                ║\n");
    printf("║                   SNAKE GAME                   ║\n");
    printf("║                                                ║\n");
    printf("║                Kastriot  Rexhaj	             ║\n");
    printf("║                                                ║\n");
    printf("║                *Controls: WASD*                ║\n");
    printf("║                                                ║\n");
    printf("║              Press enter to start...           ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    c = getchar();

    setup();
    
    while (1) {
        draw();
        handle_input();
        update();
        int sleep_duration = 200000 - snake_length * 1000;
        if (sleep_duration < 50000) { // Don't go below 50,000 microseconds
            sleep_duration = 50000;
        }
        usleep(sleep_duration);
    }
    
    return 0;
}
