// developed up to the point where responders are grouped into pairs

#include<iostream>
#include<cstring>
#include<cmath>
#include<queue>

// all time is in seconds
// all distance is in meters

// for q2 we assum each room is 6m*10m

// avg adult brisk walking speed = 3.5 mph = 1.56 m/s
const float AVG_WALKING_SPD = 1.56;

const int N = 5e3 + 5, M = 3e4 + 5;

int n = 0, n_r = 0, n_rsp = 0; // number of nodes, number of rooms, number of responders
int edge_cnt, head[N * 2];
int cleared_cnt = 0; // we define a room as cleared if it has been sweeped twice

float sum_t = 0;

struct Room{
    float length, width;
    float level_furnuture;
    int sweeped_cnt; // number of times the room has been sweeped
    float area() { return length * width; }
    float perimeter() { return 2 * (length + width); }
}r[N];
bool vis[M][N * 2];

struct Node{
    int nxt, to;

    enum Type { ROOM, CORRIDOR, STAIRWELL, EXIT } type;
    int type_id; // empty if node is not a room
    float len_to_exit = 2e9; // length to nearest exit, initialized to infinity
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
    float height, t = 0; // t: total time spent
    int id, init_pos, pos; // node index, initalized to starting position
    
    bool operator < (const Responder& other) const {
        return t > other.t;
    }
};
std::vector<Responder> init_t_pq;
std::priority_queue<Responder> t_pq;

// TODO
float GetResponderSpd(){
    return AVG_WALKING_SPD;
}

float GetSweepRoomTime(Room r){
    float t = r.area() / 4.0 * sqrt(r.level_furnuture);
    // base time = 4.0 if room is empty (level_of_furnuture = 0)
    return std::max(t, 4.0f);
}

void Init(){
    // initialize the graph (simplification of building layout)
    std::cin >> n;

    for (int i = 1; i <= n; i++){
        // read node info
        int t;
        std::cin >> t;
        if (t == 0){ // room
            float l, w;
            int f;
            std::cin >> l >> w >> f;
            r[++n_r].length = l, r[n_r].width = w, r[n_r].level_furnuture = f;
            r[n_r].sweeped_cnt = 0, r[n_r];
            a[i].type = Node::ROOM;
            a[i].type_id = n_r;
        }
        else if (t == 1){ // corridor
            a[i].type = Node::CORRIDOR;
        }
        else if (t == 2){ // stairwell
            // not considered in problem 2
        }
        else if (t == 3){ // exit
            a[i].type = Node::EXIT;
            a[i].len_to_exit = 0;
        }
    }

    int m; // number of edges
    std::cin >> m;
    for (int i = 1; i <= m; i++){
        int u, v;
        float w;
        std::cin >> u >> v >> w;
        add(u, v, w);
        add(v, u, w);
    }

    std::cin >> n_rsp;
    for (int i = 1; i <= n_rsp; i++){
        // read responder info
        int g, sp;
        float h;
        std::cin >> g >> h >> sp;
        Responder responder;
        responder.id = i, responder.is_man = g, responder.height = h;
        responder.init_pos = responder.pos = sp, responder.t = 0;
        init_t_pq.push_back(responder);
        t_pq.push(responder);
    }
}

void Reset(){
    cleared_cnt = 0;
    std::memset(vis, 0, sizeof(vis));
    for (int i = 1; i <= n_r; i++){
        r[i].sweeped_cnt = 0;
    }

    while (!t_pq.empty()) t_pq.pop();
    for (int i = 0; i < n_rsp; i++){
        t_pq.push(init_t_pq[i]);
    }
}

void DijkstraLenToExit(){
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

float Exit(){
    // all responders exit the building, return the time taken for the last responder to exit
    float max_t = 2e9;

    while (!t_pq.empty()){
        Responder rsp = t_pq.top();
        t_pq.pop();
        int pos = rsp.pos;
        rsp.t += a[pos].len_to_exit / GetResponderSpd();
        max_t = std::min(max_t, rsp.t);

        std::cout << "Responder " << rsp.id << " exits with time " << rsp.t << std::endl;
    }

    sum_t += max_t;
    return max_t;
}

bool DijkstraToNearestUnvisitedNode(Responder& rsp){
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> q;
    bool visited[N * 2] = {0};
    q.push({0, rsp.pos});

    while (!q.empty()){
        auto [dist, u] = q.top();
        q.pop();
        
        if (visited[u]) continue;
        visited[u] = 1;
        
        if (!vis[rsp.id][u] && a[u].type == Node::CORRIDOR){
            // found the nearest unvisited corridor node
            rsp.t += dist / GetResponderSpd();
            rsp.pos = u;
            return true;
        }

        for (int i = head[u]; i; i = e[i].nxt){
            int v = e[i].to;
            if (!visited[v]){
                q.push({dist + e[i].len, v});
            }
        }
    }

    // if no unvisited corridor node found, exit the building
    return false;
}

void Simulation(){
    // each room needs to be sweeped twice
    int score = 0;

    // when building not cleared
    while(cleared_cnt < n_r){
        // move the next responder with least time spent
        Responder rsp = t_pq.top();
        t_pq.pop();

        int pos = rsp.pos, i = rsp.id;
        bool moved = false;
        vis[i][pos] = true;
        
        // check all connected nodes. If there is an uncleared room that hasn't been sweeped by this responder, go sweep it
        for (int j = head[pos]; j; j = e[j].nxt){
            int v = e[j].to;
            if (a[v].type == Node::ROOM && !vis[i][v] && r[a[v].type_id].sweeped_cnt < 2){
                vis[i][v] = true;
                int room_idx = a[v].type_id;
                float sweep_time = GetSweepRoomTime(r[room_idx]);
                rsp.t += sweep_time + 2 * e[j].len / GetResponderSpd(); // time to sweep + time to enter and exit
                r[room_idx].sweeped_cnt++;
                if (r[room_idx].sweeped_cnt == 2)
                    cleared_cnt++;
                moved = true;
                break;
            }
        }

        // continue sweeping (move towards a new node)
        if (!moved){
            int cnt_tmp = 0;
            int nxt_nd[50][2] = {0}; // store possible next nodes
            for (int j = head[pos]; j; j = e[j].nxt){
                int v = e[j].to;
                if (a[v].type == Node::CORRIDOR && !vis[i][v]){
                    nxt_nd[++cnt_tmp][0] = v;
                    nxt_nd[cnt_tmp][1] = e[j].len; // store length to next node
                }
            }

            if (cnt_tmp > 0){
                // randomly choose one node to move to
                int nxt_id = 1 + rand() % cnt_tmp;
                rsp.t += nxt_nd[nxt_id][1] / GetResponderSpd(); // time to move
                rsp.pos = nxt_nd[nxt_id][0];
            }
            else {
                // go to nearest unvisited node
                bool found = DijkstraToNearestUnvisitedNode(rsp);
                if (!found){
                    // all nodes have been visited, exit the building
                    rsp.t += a[pos].len_to_exit / GetResponderSpd();
                    rsp.pos = -1; // indicate exited
                }
            }
        }

        std::cout << "Responder " << i << " at node " << rsp.pos << " with time " << rsp.t << std::endl;
        if (rsp.pos != -1) // still in building
            t_pq.push(rsp);
    }

    float time = Exit();

    std::cout << time << "\n\n";
    std::cout.flush();
}

int main(){
    freopen("2.in", "r", stdin);
    freopen("2.out", "w", stdout);

    Init();
    DijkstraLenToExit();

    for (int i = 1 ; i <= 100; i++){
        Simulation();
        Reset();
    }

    std::cout << "Average clear time: " << sum_t / 100 << std::endl;

    return 0;
}