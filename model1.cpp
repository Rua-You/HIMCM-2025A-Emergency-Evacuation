// TODO: spread of smoke
// TODO: different movement rules under IDLH conditions
// TODO: rules for fire spreading to different floors
// TODO: best inital position

#include<iostream>
#include<cstring>
#include<cmath>
#include<queue>

// all time is in seconds
// all distance is in meters

// for q2 we assum each room is 6m*10m

// avg adult brisk walking speed = 3.5 mph = 1.56 m/s
const float AVG_WALKING_SPD = 1.56;

const int N = 5e3 + 5, M = 3e4 + 5, SIM_CNT = 100;
const int ROOM_CLEARED_CNT = 1; // how many times a room sweeped to be considered cleared
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
std::vector<Team> init_t_pq;
std::priority_queue<Team> t_pq;

// TODO
float GetResponderSpd(){
    return AVG_WALKING_SPD;
}

bool InIDLH(int time, const Node &x){
    /* 
     * if the node at that time is under immediate dangerous to life and health conditions
     * this includes fire and smoke spread.
     */
    
    return time >= x.on_fire_at || time >= x.has_smoke_at;
}

float GetSweepRoomTime(const Team &t, const Node &x){
    float tm = r[x.type_id].perimeter() / GetResponderSpd() * sqrt(r[x.type_id].level_furnuture);
    // base time = 4.0 if room is empty (level_of_furnuture = 0)
    tm = std::max(tm, 4.0f);
    return tm;
}

void Init(){
    /* initialize the graph (a simplification of building layout) */

    // number of floors, floor number, number of rooms per floor
    int num_fl = 0, fl_num = 1, fl_cnt = 0;
    std::cin >> n >> num_fl >> fire_start_at;
    
    // base height of each floor
    for (int i = 1; i <= num_fl; i++){
        std::cin >> fl_h[i];
    }

    std::cin >> fl_cnt;
    // nodes information
    for (int i = 1; i <= n; i++){        
        // read node info
        int op;
        std::cin >> op;
        a[i].fl = fl_num;
        if (op == 0){ // room
            std::cin >> r[++n_r].length >> r[n_r].width >> r[n_r].level_furnuture;
            a[i].type = Node::ROOM;
            a[i].type_id = n_r;
        }
        else if (op == 1){ // corridor
            a[i].type = Node::CORRIDOR;
            std::cin >> c[++n_c].area;
            a[i].type_id = n_c;
        }
        else if (op == 2){ // stairwell
            // not considered in problem 2
        }
        else if (op == 3){ // exit
            a[i].type = Node::EXIT;
            a[i].len_to_exit = 0;
        }
        else {
            throw std::invalid_argument("Room type invalid");
        }

        if (--fl_cnt == 0) {
            std::cin >> fl_cnt;
            fl_num++;
        }
    }

    // How the nodes are connected
    int m; // number of edges
    std::cin >> m;
    for (int i = 1; i <= m; i++){
        int u, v;
        float w;
        std::cin >> u >> v >> w;
        add(u, v, w);
        add(v, u, w);
    }

    // responders & teams information
    std::cin >> n_t;
    for (int i = 1; i <= n_t; i++){
        Team t;

        std::cin >> t.n >> t.init_pos; // number of responders in team j, starting position
        n_rsp += t.n;
        t.id = i, t.pos = t.init_pos, t.t = 0;

        for (int j = 0; j < t.n; j++){
            // individual responder info
            Responder responder;
            std::cin >> responder.is_man >> responder.height;
            t.rsp[j] = responder;
        }

        init_t_pq.push_back(t);
        t_pq.push(t);
    }
}

void Reset(){
    // reset to initial state for another round of simulation
    cleared_cnt = 0;
    std::memset(vis, 0, sizeof(vis));
    for (int i = 1; i <= n; i++){
        a[i].sweeped_cnt = 0;
    }

    while (!t_pq.empty()) t_pq.pop();
    for (int i = 0; i < n_t; i++){
        t_pq.push(init_t_pq[i]);
    }
}

void DijkstraLenToExit(){
    /* 
     * precalculates the length to the nearest exit for each node using dijkstra algorithm
     * going down staircases doesn't take much time, so it's safe to count them as flat ground
     */
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> q;
    bool visited[N * 2] = {0};
    for (int i = 1; i <= n; i++){
        if (a[i].type == Node::EXIT){
            q.push({0, i});
        }
    }

    while (!q.empty()){
        auto [dist, u] = q.top();
        q.pop();
        
        if (visited[u]) continue;
        visited[u] = 1;
        a[u].len_to_exit = dist;

        for (int i = head[u]; i; i = e[i].nxt){
            int v = e[i].to;
            float len = e[i].len;
            if (!visited[v] && a[v].len_to_exit > a[u].len_to_exit + len){
                a[v].len_to_exit = a[u].len_to_exit + len;
                q.push({a[v].len_to_exit, v});
            }
        }
    }
}

float FireSpreadTimeForNode(const int u, const float t){
    // returns the time at which the node will be engulfed in fire after catching fire at time t
    
    const float MAX_SPREAD_SPD = 1.2f;
    // how much the fire area multiplies per 30s (for small fires)
    const float MULT = 2.0f;

    float area = 0.0;
    if (a[u].type == Node::ROOM) {
        area = r[a[u].type_id].area();
    }
    else if (a[u].type == Node::CORRIDOR) {
        area = c[a[u].type_id].area;
    }
    else if (a[u].type == Node::STAIRWELL) {
        // TODO.
    }
    // exit is not considered

    // plug function in
    float size = PI * INIT_FIRE_AREA * std::pow(MULT, (t - 30) / 30);

    // calculate the time at which the fire spreads over the node
    float new_t = 30 * (std::log((size + area) / (PI * INIT_FIRE_AREA)) / std::log(MULT) + 1);
    // slope = rate of which the area of the fire is expanding at time t
    float slope = INIT_FIRE_AREA * PI * std::log(MULT) / 30.0 * std::pow(MULT, new_t / 30.0 - 1.0);
    if (slope < MAX_SPREAD_SPD) {
    }
    else {
        new_t = t + area / MAX_SPREAD_SPD;
    }
    // area = pi * spd ^ 2, so spd = sqrt(slope)
    // float spd = sqrt(slope);

    return new_t;
    // TODO: this inheritenly returns a value larger than the actual value because it doesn't consider increasing fire spread speed when the node is on fire
}

void DijkstraFireSpread(){
    // tags each node of the time that fire would spread to it.
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> q;
    bool visited[N * 2] = {0};
    // it takes 30s for a small fire to become a major one
    q.push({30, fire_start_at});

    while (!q.empty()){
        auto [tm, u] = q.top();
        q.pop();
        
        if (visited[u]) continue;
        visited[u] = 1;
        a[u].on_fire_at = tm;
        float new_t = FireSpreadTimeForNode(u, tm);

        for (int i = head[u]; i; i = e[i].nxt){
            int v = e[i].to;
            if (!visited[v]){
                // once a[u] is completely burnt, all connected nodes catch fire
                a[v].on_fire_at = new_t;
                q.push({a[v].on_fire_at, v});
            }
        }
    }
}

float Exit(){
    // all teams exit the building, return the time taken for the last team to exit
    float max_t = 2e9;

    while (!t_pq.empty()){
        Team t = t_pq.top();
        t_pq.pop();
        int pos = t.pos;
        t.t += a[pos].len_to_exit / GetResponderSpd();
        max_t = std::min(max_t, t.t);

        std::cout << "Team " << t.id << " exits with time " << t.t << std::endl;
    }

    sum_t += max_t;
    return max_t;
}

bool DijkstraToNearestUnvisitedNode(Team &t, bool onSameFloor){
    // return true if there is a node to go to, otherwise false
    // the nodes are compared using time spent to get there, not nearest distance
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> q;
    bool visited[N * 2] = {0};
    q.push({0, t.pos});

    const int fl = a[t.pos].fl; // the floor they must stay on

    while (!q.empty()){
        auto [time, u] = q.top();
        q.pop();
        
        if (visited[u]) continue;
        visited[u] = 1;
        
        if (!vis[t.id][u]){
            // found the nearest unvisited node
            t.t += time;
            t.pos = u;
            return true;
        }

        for (int i = head[u]; i; i = e[i].nxt){
            int v = e[i].to;
            // the node is unvisted, hasn't been sweeped, and check if on the same floor
            if (!visited[v] && ((a[v].type == Node::ROOM && a[v].sweeped_cnt < ROOM_CLEARED_CNT) || 
                (a[v].sweeped_cnt < 1)) && ((onSameFloor && a[v].fl == fl) || (!onSameFloor))){
                // multiplier if this is going upstairs or in IDLH.
                float mult = (a[v].fl - a[u].fl == 1) ? UPSTAIRS_MULT : 1;
                float new_t = time + e[i].len * mult;
                mult *= (InIDLH(time, a[u]) || InIDLH(new_t, a[v])) ? IDLH_MULT : 1;
                new_t = time + e[i].len * mult;
                q.push({new_t, v});
            }
        }
    }

    // if no unvisited corridor node found, exit the building
    return false;
}   

float Simulation(){
    /* do 1 simulation, returns the time taken to clear & evacuate */

    // when building not cleared
    while(cleared_cnt < n_r){
        // move the next responder with least time spent
        Team t = t_pq.top();
        t_pq.pop();

        int u = t.pos, i = t.id;
        bool moved = false;
        vis[i][u] = true;
        a[u].sweeped_cnt ++;

        // check all connected nodes. If there is an uncleared room that hasn't been sweeped by this team, go sweep it
        for (int j = head[u]; j; j = e[j].nxt){
            int v = e[j].to;
            if (a[v].type == Node::ROOM && !vis[i][v] && a[v].sweeped_cnt < ROOM_CLEARED_CNT){
                vis[i][v] = true;
                int room_id = a[v].type_id;
                // time to sweep + time to enter and exit
                float time_taken = GetSweepRoomTime(t, a[v]) + 2 * e[j].len / GetResponderSpd();
                // if in IDLH, take more time
                time_taken *= (InIDLH(t.t, a[v]) || InIDLH(t.t, a[u])) ? IDLH_MULT : 1;
                t.t += time_taken;
                a[v].sweeped_cnt++;
                if (a[v].sweeped_cnt == ROOM_CLEARED_CNT)
                    cleared_cnt++;
                moved = true;
                std::cout << "Team " << i << " sweeped room " << room_id << " at time " << t.t << std::endl; 
                break;
            }
        }

        // continue sweeping (move towards a new node)
        if (!moved){
            int cnt_tmp = 0;
            int nxt_nd[50][2] = {0}; // store possible next nodes
            for (int j = head[u]; j; j = e[j].nxt){
                int v = e[j].to;
                if (a[v].type == Node::CORRIDOR && !vis[i][v]){
                    // the node being a corridor imples same floor
                    nxt_nd[++cnt_tmp][0] = v;
                    nxt_nd[cnt_tmp][1] = e[j].len; // store length to next node
                }
            }

            if (cnt_tmp > 0){
                // randomly choose one node to move to
                int nxt_id = 1 + rand() % cnt_tmp;
                t.t += nxt_nd[nxt_id][1] / GetResponderSpd(); // time to move
                t.pos = nxt_nd[nxt_id][0];
            }
            else {
                // if there's no adjacent node to move to, go to nearest unvisited node on the same floor
                bool found = DijkstraToNearestUnvisitedNode(t, 1);
                if (!found){
                    // all nodes on this floor have been visited, try other floors
                    found = DijkstraToNearestUnvisitedNode(t, 0);
                    if (!found){
                        // all nodes visited, exit the building
                        t.t += a[u].len_to_exit / GetResponderSpd();
                        t.pos = -1; // indicate exited
                    }
                }
            }
        }

        // std::cout << "Team " << i << " at node " << t.pos << " at time " << t.t << std::endl;
        if (t.pos != -1) // still in building
            t_pq.push(t);
    }

    float time = Exit();

    if (min_t > time){
        std::cout << "NEW MIN TIME: ";
        min_t = time;
    }
    std::cout << time << "\n\n";
    return time;
}

void Debug(){
    // when each node catches fire
    for (int i = 1; i <= n; i++){
        std::cout << "Node " << i << " catches fire at " << a[i].on_fire_at << std::endl;
    }
    std::cout << std::endl;
}

int main(){
    freopen("2.in", "r", stdin);
    freopen("2.out", "w", stdout);

    Init();
    DijkstraLenToExit();
    DijkstraFireSpread();
    
    Debug();

    for (int i = 1; i <= SIM_CNT; i++){
        Simulation();
        Reset();
    }

    float avg_t = sum_t / SIM_CNT;

    std::cout << "Average clear time: " << avg_t << std::endl;

    return 0;
}