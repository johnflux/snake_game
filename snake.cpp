// SnakeAI.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define ARENA_HEIGHT 14
#define ARENA_WIDTH 14
#define ARENA_SIZE ((ARENA_WIDTH) * (ARENA_HEIGHT))

int tourToNumber[ARENA_SIZE];

/* Take an x,y coordinate, and turn it into an index in the tour */
int getPathNumber(int x, int y) {
    return tourToNumber[x + ARENA_WIDTH * y];
}

int path_distance(int a, int b) {
    if (a < b)
        return b - a - 1;
    return b - a - 1 + ARENA_SIZE;
}

struct Food {
    int x = 2*ARENA_WIDTH/3;
    int y = 2*ARENA_HEIGHT/3;
    int value = 1;
} food;

enum SnakeDirection
{
    None = 0,
    Right = 1,
    Left = 2,
    Up = 4,
    Down = 8
};

struct Snake {
    int head_x = ARENA_WIDTH/3;
    int head_y = ARENA_HEIGHT/3;
    int tail_x = ARENA_WIDTH/3;
    int tail_y = ARENA_HEIGHT/3;
    int growth_length = 1;
    int drawn_length = 1;

    /* Store the body in two parts.  For each x,y store the direction
       that the snake come *from* and the direction it is going to.
       So if a snake goes moves down, the previous head square gets
       the 'out' set to down, and the new head square gets the in set
       to down. */
    SnakeDirection snake_body_in[ARENA_SIZE];
    SnakeDirection snake_body_out[ARENA_SIZE];

    SnakeDirection get_snake_body_in_at(int x, int y) {
        return snake_body_in[x + y *ARENA_WIDTH];
    }
    SnakeDirection get_snake_body_out_at(int x, int y) {
        return snake_body_out[x + y *ARENA_WIDTH];
    }


    void set_snake_body_in_at(int x, int y, SnakeDirection new_body_dir) {
        snake_body_in[x + y *ARENA_WIDTH] = new_body_dir;
    }

    void set_snake_body_out_at(int x, int y, SnakeDirection new_body_dir) {
        snake_body_out[x + y *ARENA_WIDTH] = new_body_dir;
    }

    bool has_snake_at(int x, int y) {
        return get_snake_body_in_at(x,y) != SnakeDirection::None ||
            get_snake_body_out_at(x,y) != SnakeDirection::None;
    }

    const char *get_snake_body_glyph_at(int x, int y) {
        SnakeDirection dir = (SnakeDirection)(get_snake_body_in_at(x,y) | get_snake_body_out_at(x,y));
        bool is_tail = tail_x == x && tail_y == y;
        switch(dir) {
            case Right:
                return is_tail ? "╶" : "╺";
            case Left:
                return is_tail ? "╴" : "╸";
            case Up:
                return is_tail ? "╵" : "╹";
            case Down:
                return is_tail ? "╷" : "╻";
            case (SnakeDirection)(Right | Left):
                return "━";
            case (SnakeDirection)(Up | Down):
                return "┃";
            case (SnakeDirection)(Up | Left):
                return "┛";
            case (SnakeDirection)(Up | Right):
                return "┗";
            case (SnakeDirection)(Down | Left):
                return "┓";
            case (SnakeDirection)(Down | Right):
                return "┏";
            default:
                return " ";
        }
    }

    SnakeDirection reverse_direction(SnakeDirection dir) {
        switch (dir) {
            case Right:
                return Left;
            case Left:
                return Right;
            case Up:
                return Down;
            case Down:
                return Up;
        }
        return None;
    }

    void move_snake_head(SnakeDirection dir) {
        int x = head_x;
        int y = head_y;
        switch (dir) {
        case Right: ++x; break;
        case Left: --x; break;
        case Down: ++y; break;
        case Up: --y; break;
        }

        // Now update the snake_body
        set_snake_body_out_at(head_x, head_y, dir);
        set_snake_body_in_at(x, y, reverse_direction(dir));

        head_x = x;
        head_y = y;

        if (head_x == food.x && head_y == food.y) {
            // eat food
            growth_length += food.value;
            food.value++;
            if (drawn_length < ARENA_SIZE - 1) {
                do {
                    food.x = rand() % ARENA_WIDTH;
                    food.y = rand() % ARENA_HEIGHT;
                } while(has_snake_at(food.x, food.y));
            }
        }

        if (growth_length > 0) {
            growth_length--;
            drawn_length++;
        } else {
            /* Now shrink the tail */
            int x = tail_x;
            int y = tail_y;
            switch (get_snake_body_out_at(tail_x, tail_y)) {
                case Right: ++x; break;
                case Left: --x; break;
                case Down: ++y; break;
                case Up: --y; break;
            }
            set_snake_body_out_at(tail_x, tail_y, SnakeDirection::None);
            set_snake_body_in_at(x, y, SnakeDirection::None);
            tail_x = x;
            tail_y = y;
        }
    }
} snake;

struct Maze {
    struct Node {
        bool visited : 1;
        bool canGoRight : 1;
        bool canGoDown : 1;
    };
    Node nodes[ARENA_SIZE / 4];
    void markVisited(int x, int y) {
        nodes[x + y * ARENA_WIDTH / 2].visited = true;
    }
    void markCanGoRight(int x, int y) {
        nodes[x + y * ARENA_WIDTH / 2].canGoRight = true;
    }
    void markCanGoDown(int x, int y) {
        nodes[x + y * ARENA_WIDTH / 2].canGoDown = true;
    }
    bool canGoRight(int x, int y) {
        return nodes[x + y * ARENA_WIDTH / 2].canGoRight;;
    }
    bool canGoDown(int x, int y) {
        return nodes[x + y * ARENA_WIDTH / 2].canGoDown;
    }
    bool canGoLeft(int x, int y) {
        if (x == 0) return false;
        return nodes[(x - 1) + y * ARENA_WIDTH / 2].canGoRight;
    }

    bool canGoUp(int x, int y) {
        if (y == 0) return false;
        return nodes[x + (y - 1) * ARENA_WIDTH / 2].canGoDown;
    }

    bool isVisited(int x, int y) {
        return nodes[x + y * ARENA_WIDTH / 2].visited;
    }

    void generate() {
        memset(nodes, 0, sizeof(nodes));
        generate_r(-1, -1, 0, 0);
        generateTourNumber();
    }

    void generate_r(int fromx, int fromy, int x, int y) {
        if (x < 0 || y < 0 || x >= ARENA_WIDTH / 2 || y >= ARENA_HEIGHT / 2)
            return;
        if (isVisited(x, y))
            return;
        markVisited(x, y);

        if (fromx != -1) {
            if (fromx < x)
                markCanGoRight(fromx, fromy);
            else if (fromx > x)
                markCanGoRight(x, y);
            else if (fromy < y)
                markCanGoDown(fromx, fromy);
            else if (fromy > y)
                markCanGoDown(x, y);

            //Remove wall between fromx and fromy
        }

        /* We want to visit the four connected nodes randomly,
         * so we just visit two randomly (maybe already visited)
         * then just visit them all non-randomly.  It's okay to
         * visit the same node twice */
        for (int i = 0; i < 2; i++) {
            int r = rand() % 4;
            switch (r) {
            case 0: generate_r(x, y, x - 1, y); break;
            case 1: generate_r(x, y, x + 1, y); break;
            case 2: generate_r(x, y, x, y - 1); break;
            case 3: generate_r(x, y, x, y + 1); break;
            }
        }
        generate_r(x, y, x - 1, y);
        generate_r(x, y, x + 1, y);
        generate_r(x, y, x, y + 1);
        generate_r(x, y, x, y - 1);
    }

    SnakeDirection findNextDir(int x, int y, SnakeDirection dir) {
        if (dir == Right) {
            if (canGoUp(x, y))
                return Up;
            if (canGoRight(x, y))
                return Right;
            if (canGoDown(x, y))
                return Down;
            return Left;
        }
        else if (dir == Down) {
            if (canGoRight(x, y))
                return Right;
            if (canGoDown(x, y))
                return Down;
            if (canGoLeft(x, y))
                return Left;
            return Up;
        }
        else if (dir == Left) {
            if (canGoDown(x, y))
                return Down;
            if (canGoLeft(x, y))
                return Left;
            if (canGoUp(x, y))
                return Up;
            return Right;
        }
        else if (dir == Up) {
            if (canGoLeft(x, y))
                return Left;
            if (canGoUp(x, y))
                return Up;
            if (canGoRight(x, y))
                return Right;
            return Down;
        }
        return (SnakeDirection)-1; //Unreachable
    }
    void setTourNumber(int x, int y, int number) {
        if (getPathNumber(x, y) != 0)
            return; /* Back to the starting node */
        tourToNumber[x + ARENA_WIDTH * y] = number;
    }

    void debug_print_maze_path() {
        for (int y = 0; y < ARENA_HEIGHT; ++y) {
            for (int x = 0; x < ARENA_WIDTH; ++x)
                printf("%4d ", getPathNumber(x,y));
            printf("\n");
        }
    }

    void generateTourNumber() {
        const int start_x = 0;
        const int start_y = 0;
        int x = start_x;
        int y = start_y;
        const SnakeDirection start_dir = canGoDown(x, y) ? Up : Left;
        SnakeDirection dir = start_dir;
        int number = 0;
        do {
            SnakeDirection nextDir = findNextDir(x, y, dir);
            switch (dir) {
            case Right:
                setTourNumber(x * 2, y * 2, number++);
                if (nextDir == dir || nextDir == Down || nextDir == Left)
                    setTourNumber(x * 2 + 1, y * 2, number++);
                if (nextDir == Down || nextDir == Left)
                    setTourNumber(x * 2 + 1, y * 2 + 1, number++);
                if (nextDir == Left)
                    setTourNumber(x * 2, y * 2 + 1, number++);
                break;
            case Down:
                setTourNumber(x * 2 + 1, y * 2, number++);
                if (nextDir == dir || nextDir == Left || nextDir == Up)
                    setTourNumber(x * 2 + 1, y * 2 + 1, number++);
                if (nextDir == Left || nextDir == Up)
                    setTourNumber(x * 2, y * 2 + 1, number++);
                if (nextDir == Up)
                    setTourNumber(x * 2, y * 2, number++);
                break;
            case Left:
                setTourNumber(x * 2 + 1, y * 2 + 1, number++);
                if (nextDir == dir || nextDir == Up || nextDir == Right)
                    setTourNumber(x * 2, y * 2 + 1, number++);
                if (nextDir == Up || nextDir == Right)
                    setTourNumber(x * 2, y * 2, number++);
                if (nextDir == Right)
                    setTourNumber(x * 2 + 1, y * 2, number++);
                break;
            case Up:
                setTourNumber(x * 2, y * 2 + 1, number++);
                if (nextDir == dir || nextDir == Right || nextDir == Down)
                    setTourNumber(x * 2, y * 2, number++);
                if (nextDir == Right || nextDir == Down)
                    setTourNumber(x * 2 + 1, y * 2, number++);
                if (nextDir == Down)
                    setTourNumber(x * 2 + 1, y * 2 + 1, number++);
                break;
            }
            dir = nextDir;

            switch (nextDir) {
            case Right: ++x; break;
            case Left: --x; break;
            case Down: ++y; break;
            case Up: --y; break;
            }

        } while (number != ARENA_SIZE); //Loop until we return to the start
    }
};

bool is_outside_maze(int x, int y) {
    return x < 0 || y < 0 || x >= ARENA_WIDTH || y >= ARENA_HEIGHT;
}

bool check_for_collision(int x, int y) {
    if (is_outside_maze(x, y))
        return true;

    return snake.has_snake_at(x,y);
}

SnakeDirection aiGetNewSnakeDirection() {
    int x = snake.head_x;
    int y = snake.head_y;
    const int pathNumber = getPathNumber(x, y);
    const int distanceToFood = path_distance(pathNumber, getPathNumber(food.x, food.y));
    const int distanceToTail = path_distance(pathNumber, getPathNumber(snake.tail_x, snake.tail_y));
    int cuttingAmountAvailable = distanceToTail - snake.growth_length - 3 /* Allow a small buffer */;
    const int numEmptySquaresOnBoard = ARENA_SIZE - snake.drawn_length - snake.growth_length - food.value;
    // If we don't have much space (i.e. snake is 75% of board) then don't take any shortcuts */
    if (numEmptySquaresOnBoard < ARENA_SIZE / 2)
        cuttingAmountAvailable = 0;
    else if (distanceToFood < distanceToTail) { /* We will eat the food on the way to the tail, so take that into account */
        cuttingAmountAvailable -= food.value;
        /* Once we ate that food, we might end up with another food suddenly appearing in front of us */
        if ((distanceToTail - distanceToFood) * 4 > numEmptySquaresOnBoard) /* 25% chance of another number appearing */
            cuttingAmountAvailable -= 10;
    }
    int cuttingAmountDesired = distanceToFood;
    if (cuttingAmountDesired < cuttingAmountAvailable)
        cuttingAmountAvailable = cuttingAmountDesired;
    if (cuttingAmountAvailable < 0)
        cuttingAmountAvailable = 0;
    // cuttingAmountAvailable is now the maximum amount that we can cut by

    bool canGoRight = !check_for_collision(x + 1, y);
    bool canGoLeft = !check_for_collision(x - 1, y);
    bool canGoDown = !check_for_collision(x, y + 1);
    bool canGoUp = !check_for_collision(x, y - 1);

    SnakeDirection bestDir;
    int bestDist = -1;
    if (canGoRight) {
        int dist = path_distance(pathNumber, getPathNumber(x + 1, y));
        if (dist <= cuttingAmountAvailable && dist > bestDist) {
            bestDir = Right;
            bestDist = dist;
        }
    }
    if (canGoLeft) {
        int dist = path_distance(pathNumber, getPathNumber(x - 1, y));
        if (dist <= cuttingAmountAvailable && dist > bestDist) {
            bestDir = Left;
            bestDist = dist;
        }
    }
    if (canGoDown) {
        int dist = path_distance(pathNumber, getPathNumber(x, y + 1));
        if (dist <= cuttingAmountAvailable && dist > bestDist) {
            bestDir = Down;
            bestDist = dist;
        }
    }
    if (canGoUp) {
        int dist = path_distance(pathNumber, getPathNumber(x, y - 1));
        if (dist <= cuttingAmountAvailable && dist > bestDist) {
            bestDir = Up;
            bestDist = dist;
        }
    }
    if (bestDist >= 0)
        return bestDir;

    if (canGoUp)
        return Up;
    if (canGoLeft)
        return Left;
    if (canGoDown)
        return Down;
    if (canGoRight)
        return Right;
    return None;
}

Maze maze;

void draw() {
    system("clear");
    for (int y = -1; y < ARENA_HEIGHT+1; ++y) {
        for (int x = -1; x < ARENA_WIDTH+1; ++x) {
            if (is_outside_maze(x,y))
                printf("▒");
            else if(check_for_collision(x,y))
                printf("%s", snake.get_snake_body_glyph_at(x,y));
            else if (x == food.x && y == food.y)
                printf("%d", food.value % 10);
            else
                printf(" ");
        }
        printf("\n");
    }
}

bool doTick() {
    SnakeDirection new_dir = aiGetNewSnakeDirection();
    if (new_dir == None)
        return false;
    snake.move_snake_head(new_dir);
    return true;
}

int main()
{
    srand(time(NULL));
    maze.generate();
    //maze.debug_print_maze_path();
    bool success = true;
    while(success) {
        draw();
        success = doTick();
        usleep(100000);
    }
}
