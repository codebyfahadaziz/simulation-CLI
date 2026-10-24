// CPU Scheduling Simulator - OS Lab PBL (Project 1)
// Algorithms: FCFS, SJF (NP), SRTF, Priority (NP & P), Round Robin, MLFQ (bonus)
// Tie-breaking rule everywhere: earlier arrival first, then smaller PID.
// Lower priority number = higher priority.

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <queue>
#include <deque>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
using namespace std;

const int IDLE = -1;

// ---------------------------------------------------------------- data types
struct Process {
    int pid = 0, at = 0, bt = 0, pr = 0;      // input
    int rem = 0, ct = 0, tat = 0, wt = 0, rt = -1;  // computed
};

struct Seg { int pid, s, e; };  // one Gantt block

struct Result {
    string name;
    vector<Process> procs;
    vector<Seg> gantt;
    double avgWT = 0, avgTAT = 0, avgRT = 0, util = 0;
};

// ------------------------------------------------------------------ helpers
// Add a block to the Gantt chart; merge with previous if same process & contiguous.
void addSeg(vector<Seg>& g, int pid, int s, int e) {
    if (s >= e) return;
    if (!g.empty() && g.back().pid == pid && g.back().e == s) g.back().e = e;
    else g.push_back({pid, s, e});
}

// Fresh copy of the input with run-time fields reset.
vector<Process> freshCopy(const vector<Process>& in) {
    vector<Process> v = in;
    for (auto& p : v) { p.rem = p.bt; p.ct = p.tat = p.wt = 0; p.rt = -1; }
    return v;
}

// Compute TAT, WT, averages and CPU utilization from CT and RT.
void finalize(Result& r) {
    double sw = 0, st = 0, sr = 0;
    for (auto& p : r.procs) {
        p.tat = p.ct - p.at;
        p.wt = p.tat - p.bt;
        sw += p.wt; st += p.tat; sr += p.rt;
    }
    double n = r.procs.size();
    r.avgWT = sw / n; r.avgTAT = st / n; r.avgRT = sr / n;
    int busy = 0;
    for (auto& s : r.gantt) if (s.pid != IDLE) busy += s.e - s.s;
    int total = r.gantt.empty() ? 1 : r.gantt.back().e;
    r.util = 100.0 * busy / total;
}

// ----------------------------------------------------------------- FCFS
Result fcfs(const vector<Process>& in) {
    Result r; r.name = "FCFS"; r.procs = freshCopy(in);
    vector<int> ord(r.procs.size());
    for (size_t i = 0; i < ord.size(); i++) ord[i] = i;
    sort(ord.begin(), ord.end(), [&](int a, int b) {
        return make_tuple(r.procs[a].at, r.procs[a].pid) < make_tuple(r.procs[b].at, r.procs[b].pid);
    });
    int t = 0;
    for (int i : ord) {
        Process& p = r.procs[i];
        if (t < p.at) { addSeg(r.gantt, IDLE, t, p.at); t = p.at; }  // idle gap
        p.rt = t - p.at;
        addSeg(r.gantt, p.pid, t, t + p.bt);
        t += p.bt;
        p.ct = t;
    }
    finalize(r);
    return r;
}

// ------------------------------------- generic NON-preemptive (SJF, Priority)
typedef function<tuple<int, int, int>(const Process&)> KeyFn;

Result nonPreemptive(const string& name, const vector<Process>& in, KeyFn key) {
    Result r; r.name = name; r.procs = freshCopy(in);
    int n = r.procs.size(), done = 0, t = 0;
    vector<bool> fin(n, false);
    while (done < n) {
        int best = -1;
        for (int i = 0; i < n; i++)
            if (!fin[i] && r.procs[i].at <= t)
                if (best == -1 || key(r.procs[i]) < key(r.procs[best])) best = i;
        if (best == -1) {  // nobody ready -> jump to next arrival (idle)
            int nxt = 1 << 30;
            for (int i = 0; i < n; i++) if (!fin[i]) nxt = min(nxt, r.procs[i].at);
            addSeg(r.gantt, IDLE, t, nxt);
            t = nxt;
            continue;
        }
        Process& p = r.procs[best];
        p.rt = t - p.at;
        addSeg(r.gantt, p.pid, t, t + p.bt);
        t += p.bt;
        p.ct = t; p.rem = 0; fin[best] = true; done++;
    }
    finalize(r);
    return r;
}

// ----------------------------------- generic PREEMPTIVE (SRTF, Priority)
// Re-decides every time unit, so a newly arrived better process preempts.
Result preemptive(const string& name, const vector<Process>& in, KeyFn key) {
    Result r; r.name = name; r.procs = freshCopy(in);
    int n = r.procs.size(), done = 0, t = 0;
    while (done < n) {
        int best = -1;
        for (int i = 0; i < n; i++)
            if (r.procs[i].rem > 0 && r.procs[i].at <= t)
                if (best == -1 || key(r.procs[i]) < key(r.procs[best])) best = i;
        if (best == -1) { addSeg(r.gantt, IDLE, t, t + 1); t++; continue; }
        Process& p = r.procs[best];
        if (p.rt == -1) p.rt = t - p.at;
        addSeg(r.gantt, p.pid, t, t + 1);
        t++; p.rem--;
        if (p.rem == 0) { p.ct = t; done++; }
    }
    finalize(r);
    return r;
}

// ---------------------------------------------------------- Round Robin
Result roundRobin(const vector<Process>& in, int q) {
    Result r; r.name = "RR(q=" + to_string(q) + ")"; r.procs = freshCopy(in);
    int n = r.procs.size();
    vector<int> ord(n);
    for (int i = 0; i < n; i++) ord[i] = i;
    sort(ord.begin(), ord.end(), [&](int a, int b) {
        return make_tuple(r.procs[a].at, r.procs[a].pid) < make_tuple(r.procs[b].at, r.procs[b].pid);
    });
    queue<int> rq;
    int nxt = 0, t = 0, done = 0;
    auto admit = [&]() { while (nxt < n && r.procs[ord[nxt]].at <= t) rq.push(ord[nxt++]); };
    admit();
    while (done < n) {
        if (rq.empty()) {  // idle until next arrival
            int na = r.procs[ord[nxt]].at;
            addSeg(r.gantt, IDLE, t, na);
            t = na; admit();
            continue;
        }
        int i = rq.front(); rq.pop();
        Process& p = r.procs[i];
        if (p.rt == -1) p.rt = t - p.at;
        int run = min(q, p.rem);
        addSeg(r.gantt, p.pid, t, t + run);
        t += run; p.rem -= run;
        admit();                       // new arrivals enter BEFORE the preempted process
        if (p.rem > 0) rq.push(i);
        else { p.ct = t; done++; }
    }
    finalize(r);
    return r;
}

// ------------------------------------------- BONUS: Multilevel Feedback Queue
// 3 queues: Q0 (quantum 4), Q1 (quantum 8), Q2 (FCFS). New process -> Q0.
// Using up the quantum demotes a process. A process in a higher queue preempts a lower one.
Result mlfq(const vector<Process>& in) {
    Result r; r.name = "MLFQ(4,8,FCFS)"; r.procs = freshCopy(in);
    const int Q = 3;
    const int quantum[Q] = {4, 8, 1 << 30};
    int n = r.procs.size(), t = 0, done = 0;
    vector<deque<int>> qs(Q);
    vector<bool> admitted(n, false);
    int cur = -1, level = 0, used = 0;
    while (done < n) {
        // admit arrivals (in pid order for ties)
        vector<int> arr;
        for (int i = 0; i < n; i++)
            if (!admitted[i] && r.procs[i].at <= t) arr.push_back(i);
        sort(arr.begin(), arr.end(), [&](int a, int b) {
            return make_tuple(r.procs[a].at, r.procs[a].pid) < make_tuple(r.procs[b].at, r.procs[b].pid);
        });
        for (int i : arr) { qs[0].push_back(i); admitted[i] = true; }

        // preemption by a higher-priority queue
        if (cur != -1)
            for (int l = 0; l < level; l++)
                if (!qs[l].empty()) { qs[level].push_back(cur); cur = -1; break; }

        if (cur == -1) {
            int l = 0;
            while (l < Q && qs[l].empty()) l++;
            if (l == Q) { addSeg(r.gantt, IDLE, t, t + 1); t++; continue; }
            cur = qs[l].front(); qs[l].pop_front();
            level = l; used = 0;
        }
        Process& p = r.procs[cur];
        if (p.rt == -1) p.rt = t - p.at;
        addSeg(r.gantt, p.pid, t, t + 1);
        t++; p.rem--; used++;
        if (p.rem == 0) { p.ct = t; done++; cur = -1; }
        else if (used >= quantum[level]) {       // quantum expired -> demote
            qs[min(level + 1, Q - 1)].push_back(cur);
            cur = -1;
        }
    }
    finalize(r);
    return r;
}

// ------------------------------------------------------------------ output
string center(const string& s, int w) {
    int pad = max(0, w - (int)s.size()), l = pad / 2;
    return string(l, ' ') + s + string(pad - l, ' ');
}

void printGantt(const vector<Seg>& g) {
    string l1, l2;
    for (auto& s : g) {
        string label = (s.pid == IDLE) ? "IDLE" : "P" + to_string(s.pid);
        int w = max((int)label.size() + 2, 6);
        l1 += "|" + center(label, w);
        string tm = to_string(s.s);
        l2 += tm + string(max(0, w + 1 - (int)tm.size()), ' ');
    }
    l1 += "|";
    l2 += to_string(g.back().e);
    cout << "\nGantt Chart:\n" << l1 << "\n" << l2 << "\n";
}

void printResult(const Result& r) {
    cout << "\n=============== " << r.name << " ===============\n";
    printGantt(r.gantt);
    cout << "\nPID   AT   BT   PR   CT   TAT   WT   RT\n";
    vector<Process> v = r.procs;
    sort(v.begin(), v.end(), [](const Process& a, const Process& b) { return a.pid < b.pid; });
    for (auto& p : v)
        cout << left << "P" << setw(4) << p.pid << setw(5) << p.at << setw(5) << p.bt
             << setw(5) << p.pr << setw(5) << p.ct << setw(6) << p.tat
             << setw(5) << p.wt << p.rt << "\n";
    cout << fixed << setprecision(2)
         << "\nAvg Waiting Time    : " << r.avgWT
         << "\nAvg Turnaround Time : " << r.avgTAT
         << "\nAvg Response Time   : " << r.avgRT
         << "\nCPU Utilization     : " << r.util << " %\n";
}

void printComparison(const vector<Result>& rs) {
    cout << "\n=========================== COMPARISON SUMMARY ===========================\n";
    cout << left << setw(18) << "Algorithm" << setw(10) << "Avg WT" << setw(10) << "Avg TAT"
         << setw(10) << "Avg RT" << setw(12) << "CPU Util %" << "\n";
    cout << string(60, '-') << "\n" << fixed << setprecision(2);
    int best = 0;
    for (size_t i = 0; i < rs.size(); i++) {
        cout << left << setw(18) << rs[i].name << setw(10) << rs[i].avgWT << setw(10) << rs[i].avgTAT
             << setw(10) << rs[i].avgRT << setw(12) << rs[i].util << "\n";
        if (rs[i].avgWT < rs[best].avgWT) best = i;
    }
    cout << "\nBest algorithm (lowest average waiting time): " << rs[best].name
         << " (" << rs[best].avgWT << ")\n";

    // BONUS: text bar graph of average waiting time
    double mx = 0;
    for (auto& r : rs) mx = max(mx, r.avgWT);
    cout << "\nAverage Waiting Time graph:\n";
    for (auto& r : rs) {
        int bars = mx > 0 ? (int)(r.avgWT / mx * 40) : 0;
        cout << left << setw(18) << r.name << "| " << string(bars, '#') << " " << r.avgWT << "\n";
    }
}

// Export results for the matplotlib script (plot_results.py)
void exportCSV(const vector<Result>& rs) {
    ofstream f("results.csv");
    f << "algorithm,avg_wt,avg_tat,avg_rt,cpu_util\n";
    for (auto& r : rs)
        f << r.name << "," << r.avgWT << "," << r.avgTAT << "," << r.avgRT << "," << r.util << "\n";
    ofstream g("gantt.csv");
    g << "algorithm,pid,start,end\n";
    for (auto& r : rs)
        for (auto& s : r.gantt) g << r.name << "," << s.pid << "," << s.s << "," << s.e << "\n";
    cout << "\nExported results.csv and gantt.csv  (run: python3 plot_results.py)\n";
}

// ------------------------------------------------------------------- input
bool validate(const vector<Process>& v, string& err) {
    if (v.empty()) { err = "no processes"; return false; }
    set<int> ids;
    for (auto& p : v) {
        if (!ids.insert(p.pid).second) { err = "duplicate PID " + to_string(p.pid); return false; }
        if (p.at < 0 || p.bt <= 0 || p.pr < 0) { err = "invalid values for PID " + to_string(p.pid); return false; }
    }
    return true;
}

bool loadCSV(const string& path, vector<Process>& out) {
    ifstream f(path);
    if (!f) { cout << "Cannot open file: " << path << "\n"; return false; }
    out.clear();
    string line;
    while (getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || !(isdigit(line[0]) || line[0] == '-')) continue;  // skip header/blank
        for (char& c : line) if (c == ',') c = ' ';
        istringstream ss(line);
        Process p;
        if (!(ss >> p.pid >> p.at >> p.bt >> p.pr)) { cout << "Bad line: " << line << "\n"; return false; }
        out.push_back(p);
    }
    string err;
    if (!validate(out, err)) { cout << "Invalid input: " << err << "\n"; return false; }
    return true;
}

bool readInt(const string& prompt, int& x) {
    cout << prompt;
    if (!(cin >> x)) { cin.clear(); cin.ignore(10000, '\n'); return false; }
    return true;
}

bool loadKeyboard(vector<Process>& out) {
    int n;
    if (!readInt("Number of processes: ", n) || n <= 0) { cout << "Invalid number.\n"; return false; }
    out.clear();
    for (int i = 0; i < n; i++) {
        Process p;
        cout << "--- Process " << i + 1 << " ---\n";
        if (!readInt("PID: ", p.pid) || !readInt("Arrival time: ", p.at) ||
            !readInt("Burst time: ", p.bt) || !readInt("Priority (lower = higher): ", p.pr)) {
            cout << "Invalid input.\n"; return false;
        }
        out.push_back(p);
    }
    string err;
    if (!validate(out, err)) { cout << "Invalid input: " << err << "\n"; return false; }
    return true;
}

// -------------------------------------------------------------------- main
KeyFn sjfKey  = [](const Process& p) { return make_tuple(p.bt, p.at, p.pid); };
KeyFn srtfKey = [](const Process& p) { return make_tuple(p.rem, p.at, p.pid); };
KeyFn prioKey = [](const Process& p) { return make_tuple(p.pr, p.at, p.pid); };

int main(int argc, char** argv) {
    vector<Process> procs;
    if (argc > 1 && loadCSV(argv[1], procs)) cout << "Loaded " << procs.size() << " processes from " << argv[1] << "\n";

    while (true) {
        cout << "\n========== CPU SCHEDULING SIMULATOR ==========\n"
             << "1. Enter processes manually\n2. Load processes from CSV file\n"
             << "3. Run ONE algorithm\n4. Compare ALL algorithms\n0. Exit\nChoice: ";
        int c;
        if (!(cin >> c)) { if (cin.eof()) break; cin.clear(); cin.ignore(10000, '\n'); continue; }
        if (c == 0) break;
        if (c == 1) loadKeyboard(procs);
        else if (c == 2) { string path; cout << "CSV file name: "; cin >> path; loadCSV(path, procs); }
        else if (c == 3 || c == 4) {
            if (procs.empty()) { cout << "Load processes first (option 1 or 2).\n"; continue; }
            int q = 2;
            if (c == 3) {
                cout << "1.FCFS 2.SJF(NP) 3.SRTF 4.Priority(NP) 5.Priority(P) 6.RoundRobin 7.MLFQ\nAlgorithm: ";
                int a; cin >> a;
                if (a == 6) { if (!readInt("Time quantum: ", q) || q <= 0) { cout << "Invalid quantum.\n"; continue; } }
                Result r;
                switch (a) {
                    case 1: r = fcfs(procs); break;
                    case 2: r = nonPreemptive("SJF (non-preemptive)", procs, sjfKey); break;
                    case 3: r = preemptive("SRTF", procs, srtfKey); break;
                    case 4: r = nonPreemptive("Priority (non-preemptive)", procs, prioKey); break;
                    case 5: r = preemptive("Priority (preemptive)", procs, prioKey); break;
                    case 6: r = roundRobin(procs, q); break;
                    case 7: r = mlfq(procs); break;
                    default: cout << "Invalid algorithm.\n"; continue;
                }
                printResult(r);
            } else {
                if (!readInt("Time quantum for Round Robin: ", q) || q <= 0) { cout << "Invalid quantum.\n"; continue; }
                vector<Result> rs = {
                    fcfs(procs),
                    nonPreemptive("SJF (NP)", procs, sjfKey),
                    preemptive("SRTF", procs, srtfKey),
                    nonPreemptive("Priority (NP)", procs, prioKey),
                    preemptive("Priority (P)", procs, prioKey),
                    roundRobin(procs, q),
                    mlfq(procs)};
                for (auto& r : rs) printResult(r);
                printComparison(rs);
                exportCSV(rs);
            }
        }
    }
    cout << "Goodbye.\n";
    return 0;
}
