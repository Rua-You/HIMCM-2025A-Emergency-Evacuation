// TODO: spread of smoke
// TODO: different movement rules under IDLH conditions
// TODO: rules for fire spreading to different floors
// TODO: best inital position

#ifndef MODEL_CPP
#define MODEL_CPP

#include "main.hpp"
#include "init.cpp"

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


float Exit(){
    // all teams exit the building, return the time taken for the last team to exit
    float max_t = 2e9;

    while (!t_pq.empty()){
        Team t = t_pq.top();
        t_pq.pop();
        int pos = t.pos;
        t.t += a[pos].len_to_exit / GetResponderSpd();
        max_t = std::min(max_t, t.t);

        if (DEBUG) {
            std::cout << "Team " << t.id << " exits with time " << t.t << std::endl;
        }
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
            // skip exits
            if (a[v].type == Node::EXIT) {
                continue;
            }
            // the node is dijed, hasn't been sweeped, and check if on the same floor
            if (!visited[v] &&
                ((a[v].type == Node::ROOM && a[v].sweeped_cnt < ROOM_CLEARED_CNT) || (1)) && 
                ((onSameFloor && a[v].fl == fl) || (!onSameFloor))){
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
                if (DEBUG) {
                    std::cout << "Team " << i << " sweeped room " << v << " at time " << t.t << std::endl; 
                }
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
        if (DEBUG) {
            std::cout << "NEW MIN TIME: ";
        }
        min_t = time;
    }
    if (DEBUG) {
        std::cout << time << "\n\n";
    }
    return time;
}

int GetAvgClearTime(){
    // only works properly after all simulations have been run
    return sum_t / SIM_CNT;
}

int GetMinClearTime(){
    // only works properly after all simulations have been run
    return min_t;
}

int RunSimulations(){
    // freopen("office.in", "r", stdin);
    // freopen("office.out", "w", stdout);

    for (int i = 1; i <= SIM_CNT; i++){
        Simulation();
        Reset();
    }

    float avg_t = sum_t / SIM_CNT;

    if (DEBUG) {
        std::cout << "Average clear time: " << avg_t << std::endl;
    }
    return 0;
}

#endif