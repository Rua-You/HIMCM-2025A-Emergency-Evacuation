 
#include <iostream>
#include <cstring>
#include <cmath>
#include <queue>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>

// all time is in seconds
// all distance is in meters

// avg adult brisk walking speed = 3.5 mph = 1.56 m/s
const float AVG_WALKING_SPD = 1.56;
const bool DEBUG = false;

const int N = 5e3 + 5, M = 3e4 + 5, SIM_CNT = 100;
const int ROOM_CLEARED_CNT = 1; // how many times a room sweeped to be considered cleared
const float INIT_FIRE_AREA = 0.5; // in meters
const float UPSTAIRS_MULT = 1.5; // upstair dist multiplier
const float IDLH_MULT = 1.5; // how much times more time repsonders will take 
const double PI = 3.14159265358979323846;

// number of nodes, rooms, responders, teams, corridors, stairwell
int n = 0, n_r = 0, n_rsp = 0, n_t = 0, n_c = 0, n_s = 0;
int fire_start_at, edge_cnt = 0, head[N * 2];
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
	Responder rsp[4];

	bool operator < (const Team& other) const {
		return t > other.t;
	}
};
std::vector<Team> init_t_pq;
std::priority_queue<Team> t_pq;

void ResetTimeData(){
	sum_t = 0;
	min_t = 2e9;
}

// --- init.cpp implementation ---

void Input(){
	/* initialize the graph (a simplification of building layout) */

	int num_fl = 0, fl_num = 1, fl_cnt = 0;
	std::cin >> n >> num_fl >> fire_start_at;
    
	for (int i = 1; i <= num_fl; i++){
		std::cin >> fl_h[i];
	}

	std::cin >> fl_cnt;
	for (int i = 1; i <= n; i++){
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

	int m; // number of edges
	std::cin >> m;
	for (int i = 1; i <= m; i++){
		int u, v;
		float w;
		std::cin >> u >> v >> w;
		add(u, v, w);
		add(v, u, w);
	}

	std::cin >> n_t;
	for (int i = 1; i <= n_t; i++){
		Team t;

		std::cin >> t.n >> t.init_pos; // number of responders in team j, starting position
		n_rsp += t.n;
		t.id = i, t.pos = t.init_pos, t.t = 0;

		for (int j = 0; j < t.n; j++){
			Responder responder;
			std::cin >> responder.is_man >> responder.height;
			t.rsp[j] = responder;
		}

		init_t_pq.push_back(t);
		t_pq.push(t);
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

float FireSpreadTimeForNode(const int u, const float t){
	const float MAX_SPREAD_SPD = 1.2f;
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

	float size = PI * INIT_FIRE_AREA * std::pow(MULT, (t - 30) / 30);

	float new_t = 30 * (std::log((size + area) / (PI * INIT_FIRE_AREA)) / std::log(MULT) + 1);
	float slope = INIT_FIRE_AREA * PI * std::log(MULT) / 30.0 * std::pow(MULT, new_t / 30.0 - 1.0);
	if (slope > MAX_SPREAD_SPD) {
		new_t = t + area / MAX_SPREAD_SPD;
	}

	return new_t - t;
}

void DijkstraFireSpread(){
	std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> q;
	bool visited[N * 2] = {0};
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

// --- model1.cpp implementation ---

float GetResponderSpd(){
	return AVG_WALKING_SPD;
}

bool InIDLH(int time, const Node &x){
	return time >= x.on_fire_at || time >= x.has_smoke_at;
}

float GetSweepRoomTime(const Team &t, const Node &x){
	float tm = r[x.type_id].perimeter() / GetResponderSpd() * sqrt(r[x.type_id].level_furnuture);
	tm = std::max(tm, 4.0f);
	return tm;
}

void Reset(){
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
	std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> q;
	bool visited[N * 2] = {0};
	q.push({0, t.pos});

	const int fl = a[t.pos].fl;

	while (!q.empty()){
		auto [time, u] = q.top();
		q.pop();
        
		if (visited[u]) continue;
		visited[u] = 1;
        
		if (!vis[t.id][u]){
			t.t += time;
			t.pos = u;
			return true;
		}

		for (int i = head[u]; i; i = e[i].nxt){
			int v = e[i].to;
			if (a[v].type == Node::EXIT) {
				continue;
			}
			if (!visited[v] &&
				((a[v].type == Node::ROOM && a[v].sweeped_cnt < ROOM_CLEARED_CNT) || (1)) && 
				((onSameFloor && a[v].fl == fl) || (!onSameFloor))){
				float mult = (a[v].fl - a[u].fl == 1) ? UPSTAIRS_MULT : 1;
				float new_t = time + e[i].len * mult;
				mult *= (InIDLH(time, a[u]) || InIDLH(new_t, a[v])) ? IDLH_MULT : 1;
				new_t = time + e[i].len * mult;
				q.push({new_t, v});
			}
		}
	}

	return false;
}

float Simulation(){
	while(cleared_cnt < n_r){
		Team t = t_pq.top();
		t_pq.pop();

		int u = t.pos, i = t.id;
		bool moved = false;
		vis[i][u] = true;
		a[u].sweeped_cnt ++;

		for (int j = head[u]; j; j = e[j].nxt){
			int v = e[j].to;
			if (a[v].type == Node::ROOM && !vis[i][v] && a[v].sweeped_cnt < ROOM_CLEARED_CNT){
				vis[i][v] = true;
				int room_id = a[v].type_id;
				float time_taken = GetSweepRoomTime(t, a[v]) + 2 * e[j].len / GetResponderSpd();
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

		if (!moved){
			int cnt_tmp = 0;
			int nxt_nd[50][2] = {0};
			for (int j = head[u]; j; j = e[j].nxt){
				int v = e[j].to;
				if (a[v].type == Node::CORRIDOR && !vis[i][v]){
					nxt_nd[++cnt_tmp][0] = v;
					nxt_nd[cnt_tmp][1] = e[j].len;
				}
			}

			if (cnt_tmp > 0){
				int nxt_id = 1 + rand() % cnt_tmp;
				t.t += nxt_nd[nxt_id][1] / GetResponderSpd();
				t.pos = nxt_nd[nxt_id][0];
			}
			else {
				bool found = DijkstraToNearestUnvisitedNode(t, 1);
				if (!found){
					found = DijkstraToNearestUnvisitedNode(t, 0);
					if (!found){
						t.t += a[u].len_to_exit / GetResponderSpd();
						t.pos = -1;
					}
				}
			}
		}

		if (t.pos != -1)
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
	return sum_t / SIM_CNT;
}

int GetMinClearTime(){
	return min_t;
}

int RunSimulations(){
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

// --- optimal_birth_place.cpp (main) ---

int st[1005][1005];

struct InitPos{
	int st[1005];
	float avg_t = 2e9, min_t = 2e9;
	bool operator < (const InitPos& other) const {
		return avg_t > other.avg_t;
	}
};
std::priority_queue<InitPos> q;

void Score(InitPos &p){
	ResetTimeData();
	for (int j = 0; j < n_t; j++){
		init_t_pq[j].init_pos = p.st[j + 1];
	}
	RunSimulations();
	p.avg_t = GetAvgClearTime();
	p.min_t = GetMinClearTime();
}

void GetRoot(){
	InitPos p;
	for (int i = 0; i < n_t; i++){
		p.st[i + 1] = init_t_pq[i].init_pos;
	}
	Score(p);
	q.push(p);
}

InitPos ChgBirthPoint(InitPos p){
	int rd = rand() % n_t;
	int new_init_pos = 1 + rand() % n;

	p.st[rd + 1] = new_init_pos;
	return p;
}

void PopUntil(int k){
	int sz = q.size();
	while (sz-- > k){
		q.pop();
	}
}

void Generate(){
	InitPos p = q.top();
	q.pop();
    
	for (int i = 0; i < 20; i++){
		InitPos child_p = ChgBirthPoint(p);
		Score(child_p);
		q.push(child_p);
	}
    
	PopUntil(1000);
}

void GetTop(){
    InitPos p = q.top();
    std::cout << "Top scored starting set: ";
    std::cout << "Average clear time: " << p.avg_t << "\n";
    std::cout << "Min clear time: " << p.min_t << "\n";
    for (int i = 0; i < n_t; i++){
        std::cout << p.st[i + 1] << " ";
    }
    std::cout << std::endl;
    std::cout.flush();
    fflush(stdout);
}

int main(){
	freopen("office.in", "r", stdin);
	freopen("bp.out", "w", stdout);

	Init();
	GetRoot();

	for (int i = 0; i < 10; i++){
        std::cout << "On trial " << i << std::endl;
		Generate();
	}

	GetTop();
	return 0;
}

