#ifndef INIT_CPP
#define INIT_CPP

#include "main.hpp"

void Input(){
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
            // r[n_r].node_id = i;
            a[i].type = Node::ROOM;
            a[i].type_id = n_r;
        }
        else if (op == 1){ // corridor
            a[i].type = Node::CORRIDOR;
            std::cin >> c[++n_c].area;
            a[i].type_id = n_c;
        }
        else if (op == 2){ // stairwell
            a[i].type = Node::STAIRWELL;
        }
        else if (op == 3){ // exit
            a[i].type = Node::EXIT;
            a[i].len_to_exit = 0;
        }
        else {
            throw std::invalid_argument("Room type invalid");
        }

        if (--fl_cnt <= 0) {
            if (fl_num >= num_fl) {
                break;
            }
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
    // returns the time taken for the node to be engulfed in fire after catching fire at time t
    
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
        area = (fl_h[a[u].fl + 1] - fl_h[a[u].fl]) * 15;
    }
    // exit is not considered

    // plug function in
    float size = PI * INIT_FIRE_AREA * std::pow(MULT, (t - 30) / 30);

    // calculate the time at which the fire spreads over the node
    float new_t = 30 * (std::log((size + area) / (PI * INIT_FIRE_AREA)) / std::log(MULT) + 1);
    // slope = rate of which the area of the fire is expanding at time t
    float slope = INIT_FIRE_AREA * PI * std::log(MULT) / 30.0 * std::pow(MULT, new_t / 30.0 - 1.0);
    if (slope > MAX_SPREAD_SPD) {
        new_t = t + area / MAX_SPREAD_SPD;
    }
    // area = pi * spd ^ 2, so spd = sqrt(slope)
    // float spd = sqrt(slope);

    return new_t - t;
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
        float time = FireSpreadTimeForNode(u, tm);

        for (int i = head[u]; i; i = e[i].nxt){
            int v = e[i].to;
            if (!visited[v]){
                // once a[u] is completely burnt, all connected nodes catch fire
                if (a[u].type == Node::STAIRWELL && a[v].fl == a[u].fl - 1) {
                    a[v].on_fire_at = tm + time * 5; 
                }
                else {
                    a[v].on_fire_at = tm + time;
                }
                q.push({a[v].on_fire_at, v});
            }
        }
    }
}

void Debug(){
    // when each node catches fire
    for (int i = 1; i <= n; i++){
        std::cout << "Node " << i << " catches fire at " << a[i].on_fire_at << std::endl;
    }
    std::cout << std::endl;
}

void Init(){
    Input();
    DijkstraLenToExit();
    DijkstraFireSpread();
    if (DEBUG) {
        Debug();
    }
}

#endif