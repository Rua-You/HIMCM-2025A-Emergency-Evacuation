#ifndef MAIN_H 
#define MAIN_H


#include <iostream>
#include <cstring>
#include <cmath>
#include <queue>

// all time is in seconds
// all distance is in meters

// for q2 we assum each room is 6m*10m

// avg adult brisk walking speed = 3.5 mph = 1.56 m/s
const float AVG_WALKING_SPD = 1.56;
const bool DEBUG = false;

const int N = 5e3 + 5, M = 3e4 + 5, SIM_CNT = 100;
const int ROOM_CLEARED_CNT = 2; // how many times a room sweeped to be considered cleared
const float INIT_FIRE_AREA = 0.5; // in meters
const float UPSTAIRS_MULT = 1.5; // upstair dist multiplier
const float IDLH_MULT = 1.5; // how much times more time repsonders will take 
const double PI = 3.14159265358979323846;

// number of nodes, rooms, responders, teams, corridors, stairwell
int n = 0, n_r = 0, n_rsp = 0, n_t = 0, n_c = 0, n_s = 0;
int fire_start_at, edge_cnt, head[N * 2];
int cleared_cnt = 0; // we define a room as cleared if it has been sweeped twice

float sum_t = 0, min_t = 2e9;

struct Room{
    float length, width;
    float level_furnuture;
    // int node_id; // refers to the node id; for debugging purposes
    float area() { return length * width; }
    float perimeter() { return 2 * (length + width); }
}r[N];
bool vis[M][N * 2]; // visited node or no

struct Corridor{
    float area;
}c[N];

// base floor height for each floor
float fl_h[205]; // the highest building in the world has 163 floors.

struct Node{
    int nxt, to;

    enum Type { ROOM, CORRIDOR, STAIRWELL, EXIT } type;
    int fl; // floor number
    int type_id; // links Node to the type-specific struct
    int sweeped_cnt; // number of time the node has been sweeped/walked
    
    float len_to_exit = 2e9; // length to nearest exit, initialized to infinity
    float on_fire_at = 2e9; // the time the node catchs fire
    float has_smoke_at = 2e9; // the time smoke spreads to the node
}a[N * 2];

struct Edge{
    int nxt, to;
    float len;
}e[N * 4];

void add(int u, int v, float w){
    e[++edge_cnt] = {head[u], v, w};
    head[u] = edge_cnt;
}

struct Responder{
    bool is_man;
    float height;
};

struct Team{
    int n, id; // number of responders, team id
    float t; // total time spent
    int init_pos, pos; // initial position, current position
    Responder rsp[4]; // there would almost never be a team more than 4, according to our research

    bool operator < (const Team& other) const {
        return t > other.t;
    }
};
extern std::vector<Team> init_t_pq;
extern std::priority_queue<Team> t_pq;

void ResetTimeData(){
    sum_t = 0;
    min_t = 2e9;
}

#endif