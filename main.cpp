

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <cctype>

using namespace std;

// Structure to store submission history
struct Submission {
    string problem_name;
    string status;
    int time;
    
    Submission(const string& prob, const string& stat, int t) : problem_name(prob), status(stat), time(t) {}
};

// Structure to represent a team
struct Team {
    string name;
    int solved_count = 0;
    int total_penalty = 0;
    map<char, int> incorrect_attempts; // Number of incorrect attempts for each problem
    map<char, int> solve_time; // Time when the problem was solved
    map<char, int> freeze_submissions; // Number of submissions after freeze for each problem
    set<char> frozen_problems; // Problems that are frozen for this team
    bool has_started_competition = false;
    
    // Default constructor
    Team() {}
    
    Team(const string& team_name) : name(team_name) {
        // Initialize incorrect attempts for all possible problems (A-Z)
        for (char c = 'A'; c <= 'Z'; ++c) {
            incorrect_attempts[c] = 0;
            solve_time[c] = -1; // -1 means not solved
            freeze_submissions[c] = 0;
        }
    }
};

// Global variables
map<string, Team> teams;
int duration_time = 0;
int problem_count = 0;
bool competition_started = false;
bool scoreboard_frozen = false;
set<string> frozen_teams; // Teams that have frozen problems
map<string, vector<Submission>> submission_history; // Submission history for each team

// Function to get problem ID from character
char get_problem_id(int idx) {
    return 'A' + idx;
}

// Function to check if a team exists
bool team_exists(const string& team_name) {
    return teams.find(team_name) != teams.end();
}

// Function to add a team
void add_team(const string& team_name) {
    if (competition_started) {
        cout << "[Error]Add failed: competition has started." << endl;
        return;
    }
    
    if (team_exists(team_name)) {
        cout << "[Error]Add failed: duplicated team name." << endl;
        return;
    }
    
    teams[team_name] = Team(team_name);
    cout << "[Info]Add successfully." << endl;
}

// Function to start the competition
void start_competition(int dur_time, int prob_count) {
    if (competition_started) {
        cout << "[Error]Start failed: competition has started." << endl;
        return;
    }
    
    duration_time = dur_time;
    problem_count = prob_count;
    competition_started = true;
    
    cout << "[Info]Competition starts." << endl;
}

// Function to process a submission
void submit_problem(const string& problem_name, const string& team_name, 
                   const string& submit_status, int time) {
    if (!team_exists(team_name)) {
        // According to the problem, team should exist when submission happens
        return;
    }
    
    Team& team = teams[team_name];
    char problem_id = problem_name[0];
    
    // Record the submission in history
    submission_history[team_name].push_back(Submission(problem_name, submit_status, time));
    
    // If the problem is already solved, ignore further submissions
    auto it = team.solve_time.find(problem_id);
    if (it != team.solve_time.end() && it->second != -1) {
        return;
    }
    
    if (submit_status == "Accepted") {
        // Calculate penalty: 20 * incorrect attempts + time
        auto it_incorrect = team.incorrect_attempts.find(problem_id);
        int incorrect_count = (it_incorrect != team.incorrect_attempts.end()) ? it_incorrect->second : 0;
        int penalty = 20 * incorrect_count + time;
        team.solve_time[problem_id] = time;
        team.solved_count++;
        team.total_penalty += penalty;
    } else {
        // Increment incorrect attempts
        team.incorrect_attempts[problem_id]++;
        
        // If scoreboard is frozen, increment freeze submissions
        if (scoreboard_frozen && team.frozen_problems.find(problem_id) != team.frozen_problems.end()) {
            team.freeze_submissions[problem_id]++;
        }
    }
}

// Function to flush the scoreboard
void flush_scoreboard() {
    cout << "[Info]Flush scoreboard." << endl;
}

// Function to freeze the scoreboard
void freeze_scoreboard() {
    if (scoreboard_frozen) {
        cout << "[Error]Freeze failed: scoreboard has been frozen." << endl;
        return;
    }
    
    // Mark all problems that have incorrect submissions as frozen
    for (auto& [team_name, team] : teams) {
        for (int i = 0; i < problem_count; ++i) {
            char problem_id = get_problem_id(i);
            // A problem is frozen if it has at least one submission after freeze
            // and was not solved before freeze
            if (team.solve_time[problem_id] == -1 && 
                team.incorrect_attempts[problem_id] > 0) {
                team.frozen_problems.insert(problem_id);
                frozen_teams.insert(team_name);
            }
        }
    }
    
    scoreboard_frozen = true;
    cout << "[Info]Freeze scoreboard." << endl;
}

// Function to compare teams for ranking
bool compare_teams(const string& team1_name, const string& team2_name) {
    const Team& team1 = teams[team1_name];
    const Team& team2 = teams[team2_name];
    
    // First, compare by solved count (more is better)
    if (team1.solved_count != team2.solved_count) {
        return team1.solved_count > team2.solved_count;
    }
    
    // Then, compare by penalty time (less is better)
    if (team1.total_penalty != team2.total_penalty) {
        return team1.total_penalty < team2.total_penalty;
    }
    
    // Then, compare by maximum solve time, then second largest, etc.
    vector<int> times1, times2;
    for (int i = 0; i < problem_count; ++i) {
        char problem_id = get_problem_id(i);
        // Check if the problem is solved (exists in solve_time map and value is not -1)
        auto it1 = team1.solve_time.find(problem_id);
        if (it1 != team1.solve_time.end() && it1->second != -1) {
            times1.push_back(it1->second);
        }
        auto it2 = team2.solve_time.find(problem_id);
        if (it2 != team2.solve_time.end() && it2->second != -1) {
            times2.push_back(it2->second);
        }
    }
    
    // Sort in descending order
    sort(times1.rbegin(), times1.rend());
    sort(times2.rbegin(), times2.rend());
    
    // Compare solve times from largest to smallest
    int min_size = min(times1.size(), times2.size());
    for (int i = 0; i < min_size; ++i) {
        if (times1[i] != times2[i]) {
            return times1[i] < times2[i];
        }
    }
    
    // If all solve times are equal, compare by lexicographical order of team names
    return team1_name < team2_name;
}

// Function to get the status string for a problem
string get_problem_status(const Team& team, char problem_id) {
    // If the problem is frozen
    if (team.frozen_problems.find(problem_id) != team.frozen_problems.end()) {
        auto it_incorrect = team.incorrect_attempts.find(problem_id);
        auto it_freeze = team.freeze_submissions.find(problem_id);
        int incorrect_count = (it_incorrect != team.incorrect_attempts.end()) ? it_incorrect->second : 0;
        int freeze_count = (it_freeze != team.freeze_submissions.end()) ? it_freeze->second : 0;
        int before_freeze = incorrect_count - freeze_count;
        if (before_freeze == 0) {
            return "0/" + to_string(freeze_count);
        } else {
            return "-" + to_string(before_freeze) + "/" + to_string(freeze_count);
        }
    }
    
    // If the problem is solved
    auto it_solve = team.solve_time.find(problem_id);
    if (it_solve != team.solve_time.end() && it_solve->second != -1) {
        auto it_incorrect = team.incorrect_attempts.find(problem_id);
        int incorrect_count = (it_incorrect != team.incorrect_attempts.end()) ? it_incorrect->second : 0;
        if (incorrect_count == 0) {
            return "+";
        } else {
            return "+" + to_string(incorrect_count);
        }
    }
    
    // If the problem is not solved and not frozen
    auto it_incorrect = team.incorrect_attempts.find(problem_id);
    int incorrect_count = (it_incorrect != team.incorrect_attempts.end()) ? it_incorrect->second : 0;
    if (incorrect_count == 0) {
        return ".";
    } else {
        return "-" + to_string(incorrect_count);
    }
}

// Function to print the scoreboard
void print_scoreboard() {
    // Create a list of team names
    vector<string> team_names;
    for (const auto& [name, _] : teams) {
        team_names.push_back(name);
    }
    
    // Sort teams by ranking
    sort(team_names.begin(), team_names.end(), compare_teams);
    
    // Print each team's status
    for (int i = 0; i < team_names.size(); ++i) {
        const Team& team = teams[team_names[i]];
        cout << team.name << " " << (i + 1) << " " << team.solved_count << " " << team.total_penalty;
        
        // Print status for each problem
        for (int j = 0; j < problem_count; ++j) {
            char problem_id = get_problem_id(j);
            cout << " " << get_problem_status(team, problem_id);
        }
        cout << endl;
    }
}

// Function to scroll the scoreboard
void scroll_scoreboard() {
    if (!scoreboard_frozen) {
        cout << "[Error]Scroll failed: scoreboard has not been frozen." << endl;
        return;
    }
    
    cout << "[Info]Scroll scoreboard." << endl;
    
    // Print the scoreboard before scrolling
    print_scoreboard();
    
    // Process scrolling
    bool changes_occurred = false;
    
    // Continue until no team has frozen problems
    while (!frozen_teams.empty()) {
        // Find the lowest-ranked team with frozen problems
        vector<string> team_names;
        for (const auto& [name, _] : teams) {
            team_names.push_back(name);
        }
        
        // Sort teams by ranking
        sort(team_names.begin(), team_names.end(), compare_teams);
        
        string target_team;
        char target_problem = 0;
        
        // Find the lowest-ranked team with frozen problems
        for (int i = team_names.size() - 1; i >= 0; --i) {
            const string& team_name = team_names[i];
            const Team& team = teams[team_name];
            
            if (!team.frozen_problems.empty()) {
                target_team = team_name;
                
                // Find the problem with the smallest ID among frozen problems
                target_problem = *team.frozen_problems.begin();
                for (char p : team.frozen_problems) {
                    if (p < target_problem) {
                        target_problem = p;
                    }
                }
                break;
            }
        }
        
        // If no team with frozen problems found, break
        if (target_team.empty()) {
            break;
        }
        
        // Unfreeze the problem
        Team& team = teams[target_team];
        team.frozen_problems.erase(target_problem);
        
        // If this team has no more frozen problems, remove from frozen_teams
        if (team.frozen_problems.empty()) {
            frozen_teams.erase(target_team);
        }
        
        // Check if this unfreezing caused a ranking change
        // We need to compare rankings before and after
        vector<string> old_ranking = team_names;
        vector<string> new_ranking = team_names;
        sort(new_ranking.begin(), new_ranking.end(), compare_teams);
        
        // Check if rankings changed
        bool ranking_changed = false;
        string team_name1, team_name2;
        int new_solved_count = 0;
        int new_penalty_time = 0;
        
        for (int i = 0; i < old_ranking.size(); ++i) {
            if (old_ranking[i] != new_ranking[i]) {
                ranking_changed = true;
                // Find which team moved up
                for (int j = 0; j < new_ranking.size(); ++j) {
                    if (new_ranking[j] == old_ranking[i] && j < i) {
                        // old_ranking[i] moved up from position i to position j
                        team_name1 = new_ranking[j];
                        team_name2 = new_ranking[j + 1];
                        new_solved_count = teams[team_name1].solved_count;
                        new_penalty_time = teams[team_name1].total_penalty;
                        break;
                    }
                }
                break;
            }
        }
        
        if (ranking_changed) {
            cout << team_name1 << " " << team_name2 << " " << new_solved_count << " " << new_penalty_time << endl;
            changes_occurred = true;
        }
    }
    
    // Print the scoreboard after scrolling
    print_scoreboard();
    
    // Lift the frozen state
    scoreboard_frozen = false;
}

// Function to query team ranking
void query_ranking(const string& team_name) {
    if (!team_exists(team_name)) {
        cout << "[Error]Query ranking failed: cannot find the team." << endl;
        return;
    }
    
    cout << "[Info]Complete query ranking." << endl;
    
    if (scoreboard_frozen) {
        cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
    }
    
    // Get the ranking of the team
    vector<string> team_names;
    for (const auto& [name, _] : teams) {
        team_names.push_back(name);
    }
    
    // Sort teams by ranking
    sort(team_names.begin(), team_names.end(), compare_teams);
    
    // Find the team's ranking
    int ranking = 0;
    for (int i = 0; i < team_names.size(); ++i) {
        if (team_names[i] == team_name) {
            ranking = i + 1;
            break;
        }
    }
    
    cout << team_name << " NOW AT RANKING " << ranking << endl;
}

// Function to query team submission
void query_submission(const string& team_name, const string& problem_name, const string& status) {
    if (!team_exists(team_name)) {
        cout << "[Error]Query submission failed: cannot find the team." << endl;
        return;
    }
    
    cout << "[Info]Complete query submission." << endl;
    
    // Find the last submission that matches the criteria
    string last_problem = problem_name;
    string last_status = status;
    
    // If problem_name is "ALL", we match any problem
    // If status is "ALL", we match any status
    bool found = false;
    Submission latest_submission("", "", -1);
    
    // Check if the team has any submission history
    if (submission_history.find(team_name) != submission_history.end()) {
        for (const auto& submission : submission_history[team_name]) {
            bool problem_match = (last_problem == "ALL" || submission.problem_name == last_problem);
            bool status_match = (last_status == "ALL" || submission.status == last_status);
            
            if (problem_match && status_match) {
                if (!found || submission.time > latest_submission.time) {
                    latest_submission = submission;
                    found = true;
                }
            }
        }
    }
    
    if (!found) {
        cout << "Cannot find any submission." << endl;
    } else {
        cout << team_name << " " << latest_submission.problem_name << " " 
             << latest_submission.status << " " << latest_submission.time << endl;
    }
}

// Function to end the competition
void end_competition() {
    cout << "[Info]Competition ends." << endl;
}

int main() {
    string line;
    
    while (getline(cin, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            add_team(team_name);
        }
        else if (command == "START") {
            string duration_str, problem_str;
            int duration_val, problem_val;
            iss >> duration_str >> duration_val >> problem_str >> problem_val;
            start_competition(duration_val, problem_val);
        }
        else if (command == "SUBMIT") {
            string problem_name, by_str, team_name, with_str, submit_status, at_str;
            int time;
            iss >> problem_name >> by_str >> team_name >> with_str >> submit_status >> at_str >> time;
            submit_problem(problem_name, team_name, submit_status, time);
        }
        else if (command == "FLUSH") {
            flush_scoreboard();
        }
        else if (command == "FREEZE") {
            freeze_scoreboard();
        }
        else if (command == "SCROLL") {
            scroll_scoreboard();
        }
        else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            query_ranking(team_name);
        }
        else if (command == "QUERY_SUBMISSION") {
            string team_name, where_str, problem_str, and_str, status_str;
            string problem_val = "ALL", status_val = "ALL";
            iss >> team_name >> where_str >> problem_str >> problem_val >> and_str >> status_str >> status_val;
            
            // Extract problem name and status from the strings
            // Format: PROBLEM=[problem_name] and STATUS=[status]
            if (problem_str.length() > 8) {
                problem_val = problem_str.substr(8);
            }
            if (status_str.length() > 7) {
                status_val = status_str.substr(7);
            }
            
            query_submission(team_name, problem_val, status_val);
        }
        else if (command == "END") {
            end_competition();
            break;
        }
    }
    
    return 0;
}


