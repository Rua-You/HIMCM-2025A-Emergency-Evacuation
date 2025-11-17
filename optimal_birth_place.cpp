#ifndef OPTIMAL_BIRTH_PLACE_CPP
#define OPTIMAL_BIRTH_PLACE_CPP

#include "main.hpp"
#include "model1.cpp"
#include "init.cpp"

int st[1005][1005];

struct InitPos{
    int st[1005];
    float avg_t = 2e9, min_t = 2e9;
    bool operator < (const InitPos& other) const {
        // smallest first
        return avg_t > other.avg_t;
    }
};
std::priority_queue<InitPos> q;

void Score(InitPos &p){
    // scores a set starting position
    ResetTimeData();
    for (int j = 0; j < n_t; j++){
        init_t_pq[j].init_pos = p.st[j + 1];
    }
    RunSimulations();
    p.avg_t = GetAvgClearTime();
    p.min_t = GetMinClearTime();
}

void GetRoot(){
    // get the root of all possible starting positions
    InitPos p;
    for (int i = 0; i < n_t; i++){
        p.st[i + 1] = init_t_pq[i].init_pos;
    }
    Score(p);
    q.push(p);
}

InitPos ChgBirthPoint(InitPos p){
    // changes the starting position of one team only

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
    // generate 20 kids for q.top()

    InitPos p = q.top(); // p is already scored
    q.pop();
    
    for (int i = 0; i < 20; i++){
        // iterate 20 kids for each parent
        InitPos child_p = ChgBirthPoint(p);
        Score(child_p);
        q.push(child_p);
    }
    
    // only keep the top 1000 best sets of starting positions
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
}

int main(){
    
    freopen("office.in", "r", stdin);
    freopen("bp.out", "w", stdout);

    Init();
    GetRoot();

    for (int i = 0; i < 1000; i++){
        Generate();
    }

    GetTop();
    return 0;
}

#endif