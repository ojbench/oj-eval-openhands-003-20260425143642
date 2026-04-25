#include <bits/stdc++.h>
using namespace std;

struct Submission {
    int teamId;
    int prob; // 0..M-1
    int time;
    int is_ac; // 1=AC, 0=not AC
    string status_str; // original status string
    int epoch; // freeze epoch when submitted
};

struct ProblemState {
    bool solved = false;
    int ac_time = 0;
    int wrong_before_ac = 0;

    // Freeze related
    // We support multiple freeze/scroll cycles by using an epoch counter.
    // When frozen==true, current_freeze_epoch holds the epoch number.
    bool frozen = false;
    int current_freeze_epoch = 0;
    int wrong_before_freeze = 0; // wrong count at the moment of first post-freeze submit in this epoch
    int submits_during_freeze = 0; // count of submissions in this epoch for this problem
    // Per-epoch live tracking to avoid scanning
    int epoch_wrongs_before_ac = 0;
    int epoch_ac_time = 0;
    bool epoch_seen_ac = false;
};

struct Team {
    string name;
    vector<ProblemState> probs;
    int solved = 0;
    long long penalty = 0;
    vector<int> solve_times; // descending order maintained
    int last_rank = 0; // rank after last flush
};

struct Cmp {
    bool operator()(const Team* a, const Team* b) const {
        if (a->solved != b->solved) return a->solved > b->solved;
        if (a->penalty != b->penalty) return a->penalty < b->penalty;
        size_t n = min(a->solve_times.size(), b->solve_times.size());
        for (size_t i=0;i<n;i++) if (a->solve_times[i] != b->solve_times[i]) return a->solve_times[i] < b->solve_times[i];
        if (a->solve_times.size() != b->solve_times.size()) return a->solve_times.size() < b->solve_times.size();
        return a->name < b->name;
    }
};

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Team> teams;
    unordered_map<string,int> name2id;
    vector<vector<Submission>> subs_by_team;

    bool started = false;
    int duration = 0;
    int M = 0;

    bool frozen = false;
    int freeze_epoch = 0; // increases each FREEZE

    bool has_flushed_once = false;

    string cmd;
    while (cin >> cmd){
        if (cmd == "ADDTEAM"){
            string team_name; cin >> team_name;
            if (started){ cout << "[Error]Add failed: competition has started.\n"; continue; }
            if (name2id.count(team_name)){ cout << "[Error]Add failed: duplicated team name.\n"; continue; }
            Team t; t.name = team_name; name2id[team_name] = (int)teams.size(); teams.push_back(move(t)); subs_by_team.emplace_back();
            cout << "[Info]Add successfully.\n";
        } else if (cmd == "START"){
            string D; int dur; string P; int pc;
            cin >> D >> dur >> P >> pc;
            if (started){ cout << "[Error]Start failed: competition has started.\n"; continue; }
            started = true; duration = dur; M = pc;
            for (auto &t : teams){ t.probs.assign(M, ProblemState()); t.solved=0; t.penalty=0; t.solve_times.clear(); t.last_rank=0; }
            // initialize ranking before first flush as lexicographic by team name
            {
                vector<int> idx(teams.size()); iota(idx.begin(), idx.end(), 0);
                sort(idx.begin(), idx.end(), [&](int a,int b){ return teams[a].name < teams[b].name; });
                for (size_t i=0;i<idx.size();++i) teams[idx[i]].last_rank = (int)i+1;
            }
            cout << "[Info]Competition starts.\n";
        } else if (cmd == "SUBMIT"){
            string prob_name; string BY; string team_name; string WITH; string status; string AT; int tm;
            cin >> prob_name >> BY >> team_name >> WITH >> status >> AT >> tm;
            int prob = prob_name[0]-'A';
            int tid = name2id[team_name];
            int st = (status=="Accepted")?1:0;
            Submission s{tid, prob, tm, st, status, frozen?freeze_epoch:0};
            subs_by_team[tid].push_back(s);
            Team &t = teams[tid];
            ProblemState &ps = t.probs[prob];
            if (!frozen){
                if (!ps.solved){
                    if (st){
                        ps.solved = true; ps.ac_time = tm; t.solved++; t.penalty += 20LL*ps.wrong_before_ac + tm; t.solve_times.push_back(tm); sort(t.solve_times.begin(), t.solve_times.end(), greater<int>());
                    } else {
                        ps.wrong_before_ac++;
                    }
                }
            } else {
                if (!ps.solved){
                    if (!ps.frozen){ ps.frozen = true; ps.current_freeze_epoch = freeze_epoch; ps.wrong_before_freeze = ps.wrong_before_ac; ps.submits_during_freeze = 0; ps.epoch_wrongs_before_ac=0; ps.epoch_ac_time=0; ps.epoch_seen_ac=false; }
                    ps.submits_during_freeze++;
                    if (!ps.epoch_seen_ac){ if (st){ ps.epoch_seen_ac=true; ps.epoch_ac_time=tm; } else { ps.epoch_wrongs_before_ac++; } }
                }
            }
        } else if (cmd == "FLUSH"){
            cout << "[Info]Flush scoreboard.\n";
            vector<int> idx(teams.size()); iota(idx.begin(), idx.end(), 0);
            if (!has_flushed_once) sort(idx.begin(), idx.end(), [&](int a,int b){ return teams[a].name < teams[b].name; });
            else sort(idx.begin(), idx.end(), [&](int a,int b){ Cmp cmp; return cmp(&teams[a], &teams[b]); });
            for (size_t i=0;i<idx.size();++i) teams[idx[i]].last_rank = (int)i+1;
            has_flushed_once = true;
        } else if (cmd == "FREEZE"){
            if (frozen){ cout << "[Error]Freeze failed: scoreboard has been frozen.\n"; }
            else { frozen = true; freeze_epoch++; cout << "[Info]Freeze scoreboard.\n"; }
        } else if (cmd == "SCROLL"){
            if (!frozen){ cout << "[Error]Scroll failed: scoreboard has not been frozen.\n"; continue; }
            cout << "[Info]Scroll scoreboard.\n";
            // Flush before scrolling
            vector<int> idx(teams.size()); iota(idx.begin(), idx.end(), 0);
            sort(idx.begin(), idx.end(), [&](int a,int b){ Cmp cmp; return cmp(&teams[a], &teams[b]); });
            for (size_t i=0;i<idx.size();++i) teams[idx[i]].last_rank = (int)i+1;
            auto print_board = [&](const vector<int>& order, bool after){
                for (int id : order){
                    Team &t = teams[id];
                    cout << t.name << ' ' << t.last_rank << ' ' << t.solved << ' ' << t.penalty;
                    for (int p=0;p<M;p++){
                        const ProblemState &ps = t.probs[p];
                        string cell;
                        if (ps.solved){ int x = ps.wrong_before_ac; cell = (x==0?"+":"+"+to_string(x)); }
                        else if (!after && ps.frozen && ps.current_freeze_epoch==freeze_epoch){
                            int x = ps.wrong_before_freeze; int y = ps.submits_during_freeze; cell = (x==0?"0/"+to_string(y):"-"+to_string(x)+"/"+to_string(y));
                        } else {
                            int x = ps.wrong_before_ac; cell = (x==0?".":"-"+to_string(x));
                        }
                        cout << ' ' << cell;
                    }
                    cout << '\n';
                }
            };
            print_board(idx, false);

            auto hasFrozen = [&](int id){ for (int p=0;p<M;p++){ auto &ps=teams[id].probs[p]; if (ps.frozen && ps.current_freeze_epoch==freeze_epoch) return true; } return false; };
            auto nextFrozenProblem = [&](int id){ for (int p=0;p<M;p++){ auto &ps=teams[id].probs[p]; if (ps.frozen && ps.current_freeze_epoch==freeze_epoch) return p; } return -1; };

            // position map for efficient swaps
            vector<int> pos_of(teams.size()); for (size_t i=0;i<idx.size();++i) pos_of[idx[i]]=(int)i;

            while (true){
                // select lowest-ranked team with frozen problems: scan from end of idx
                int candidate = -1;
                for (int i=(int)idx.size()-1; i>=0; --i){ int id = idx[i]; if (hasFrozen(id)){ candidate = id; break; } }
                if (candidate==-1) break;
                int p = nextFrozenProblem(candidate);
                ProblemState &ps = teams[candidate].probs[p];
                // Reveal submissions in this epoch for this team-problem using per-epoch counters
                int ac_time = ps.epoch_seen_ac ? ps.epoch_ac_time : -1;
                int wrongs_before_ac = ps.epoch_wrongs_before_ac;
                int total_y = ps.submits_during_freeze;
                // Unfreeze effects
                ps.frozen = false; ps.current_freeze_epoch = 0;
                if (ac_time!=-1){
                    ps.solved = true; ps.ac_time = ac_time; ps.wrong_before_ac = ps.wrong_before_freeze + wrongs_before_ac;
                    Team &t = teams[candidate]; t.solved++; t.penalty += 20LL*ps.wrong_before_ac + ps.ac_time; t.solve_times.push_back(ps.ac_time); sort(t.solve_times.begin(), t.solve_times.end(), greater<int>());
                } else {
                    ps.wrong_before_ac = ps.wrong_before_freeze + total_y;
                }
                ps.submits_during_freeze = 0; ps.epoch_wrongs_before_ac=0; ps.epoch_ac_time=0; ps.epoch_seen_ac=false;
                // move candidate up in idx while needed, printing swaps
                int pos = pos_of[candidate];
                while (pos > 0){
                    int other = idx[pos-1];
                    Cmp cmp; if (cmp(&teams[candidate], &teams[other])){
                        // candidate should be ahead -> swap and print
                        cout << teams[candidate].name << ' ' << teams[other].name << ' ' << teams[candidate].solved << ' ' << teams[candidate].penalty << "\n";
                        swap(idx[pos], idx[pos-1]); pos_of[idx[pos]] = pos; pos_of[idx[pos-1]] = pos-1; pos--;
                    } else break;
                }
                // update last_rank for the two moved teams only for next selection view
                teams[candidate].last_rank = pos+1;
                if (pos+1 < (int)idx.size()) teams[idx[pos+1]].last_rank = pos+2; // neighbor pushed down by one on last swap
            }
            // After scrolling ends, print final board (no frozen cells)
            for (size_t i=0;i<idx.size();++i) teams[idx[i]].last_rank=(int)i+1;
            print_board(idx, true);
            frozen = false; // end scroll lifts frozen state
        } else if (cmd == "QUERY_RANKING"){
            string team_name; cin >> team_name;
            if (!name2id.count(team_name)) cout << "[Error]Query ranking failed: cannot find the team.\n";
            else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                int id = name2id[team_name]; cout << teams[id].name << " NOW AT RANKING " << teams[id].last_rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION"){
            string team_name; string WHERE; string prob_tok; string AND; string status_tok;
            cin >> team_name >> WHERE >> prob_tok >> AND >> status_tok;
            if (!name2id.count(team_name)){
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                int id = name2id[team_name];
                auto get_val = [](const string &tok){ auto pos = tok.find('='); return pos==string::npos? string("") : tok.substr(pos+1); };
                string prob_name = get_val(prob_tok);
                string st = get_val(status_tok);
                int prob_filter = -1; if (prob_name != "ALL" && !prob_name.empty()) prob_filter = prob_name[0]-'A';
                string status_filter = st;
                int found=-1;
                for (int i=(int)subs_by_team[id].size()-1;i>=0;i--){ auto &s=subs_by_team[id][i]; if ((prob_filter==-1 || s.prob==prob_filter) && (status_filter=="ALL" || s.status_str==status_filter)){ found=i; break; } }
                if (found==-1) cout << "Cannot find any submission.\n";
                else {
                    auto &s = subs_by_team[id][found]; char pc = char('A'+s.prob);
                    string status_out = s.status_str; // output original status
                    cout << teams[id].name << ' ' << pc << ' ' << status_out << ' ' << s.time << "\n";
                }
            }
        } else if (cmd == "END"){
            cout << "[Info]Competition ends.\n"; break;
        }
    }
    return 0;
}
