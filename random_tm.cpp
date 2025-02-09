#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <bitset>
#include <cstring>
#include <cassert>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>

using namespace std;

const int NUM_TAPES = 5, LOG_N = 3;
const int N = (1 << LOG_N), T = 32;
const int PAD_LENGTH = 10, INP_LENGTH = 3, OUTPUT_LENGTH = 1 << 15;
const int KEY_LENGTH = N * (1 << NUM_TAPES) * (NUM_TAPES * 2 + LOG_N) + PAD_LENGTH * NUM_TAPES;

int TM[N][1 << NUM_TAPES], freq[N][1 << NUM_TAPES], tot;
int tapes[NUM_TAPES][T * 2 + INP_LENGTH * 4];
int random_pad[NUM_TAPES][PAD_LENGTH];


void gen_random_key(ofstream &keyout)
{
    std::mt19937 gen(time(0));
    std::uniform_int_distribution<int> bin_dist(0, 1);
    
    for (int i = 0; i < KEY_LENGTH; ++i)
    	keyout << bin_dist(gen);
    keyout << endl;
}
int load_from_key(int key[], int &pos, int cnt)
{
	int value = 0;
	for (int i = 0; i < cnt; ++i)
		value = (value << 1) | key[pos + i];
	pos += cnt;
	return value;
}
void load_TM(int TM[][1 << NUM_TAPES], string& key_string)
{
	int key[KEY_LENGTH];
	
	for (int i = 0; i < KEY_LENGTH; ++i)
		key[i] = key_string[i] - '0';
	
//	for (int i = 0; i < 128; ++i)
//		key[i] = 0;
//	key[KEY_LENGTH - 1] ^= 1;
	
	int pos = 0;
	for (int node = 0; node < N; ++node)
		for (int idx = 0; idx < (1 << NUM_TAPES); ++idx)
		{
			int next_node = 0, vals = 0, dirs = 0;
			
			next_node = load_from_key(key, pos, LOG_N);
			vals = load_from_key(key, pos, NUM_TAPES);
			dirs = load_from_key(key, pos, NUM_TAPES);
			
//			cout << next_node << " " << vals << " " << dirs << endl;
			
            TM[node][idx] = (next_node << (NUM_TAPES << 1)) | (vals << NUM_TAPES) | dirs;
		}
	
	for (int i = 0; i < NUM_TAPES; ++i)
		for (int j = 0; j < PAD_LENGTH; ++j)
		{
			random_pad[i][j] = key[pos];
			++pos;
		}
	/*
	for (int node = 0; node < N; ++node)
		for (int idx = 0; idx < (1 << NUM_TAPES); ++idx)
			cout << node << " " << idx << " " << TM[node][idx] << endl;
	cout << pos << " " << KEY_LENGTH << endl;
	*/
}
void load_tape(int tape[], uint64_t inp, int pos)
{
    for (int i = pos; i < pos + INP_LENGTH; ++i, inp >>= 1)
        tape[i] = inp & 1;
}
int read_from_tapes(int heads[])
{
    int head_content = 0;
    for (int i = 0; i < NUM_TAPES; ++i)
        head_content |= tapes[i][heads[i]] << i;
    return head_content;
}
int edit_tape(int tape[], int head, int val, int dir)
{
    tape[head] = val;
    if (dir == 0) 
        dir = -1;
    head += dir;
    if (head < 0)
        head = 0;
    return head;
}
int emulate_TM(const int TM[][1 << NUM_TAPES], int64_t inp) 
{
    memset(tapes, 0, sizeof(tapes));
    for (int i = 0; i < NUM_TAPES; ++i)
    {
        load_tape(tapes[i], inp >> (i * INP_LENGTH), T);
        load_tape(tapes[i], inp >> (i * INP_LENGTH), T + INP_LENGTH);
        memcpy(tapes[i] + T - PAD_LENGTH, random_pad[i], sizeof(random_pad[i]));
        memcpy(tapes[i] + T + INP_LENGTH * 2, random_pad[i], sizeof(random_pad[i]));
    }
    int heads[NUM_TAPES], state = 0;
    for (int i = 0; i < NUM_TAPES; ++i)
    	heads[i] = T + INP_LENGTH;
    
    for (int _ = 0; _ < T; ++_)
    {
        int head_content = read_from_tapes(heads);
        
        ++freq[state][head_content];
    	++tot;
        
        int next_state_tuple = TM[state][head_content];
        int next_state = next_state_tuple >> (NUM_TAPES << 1),
            vals = (next_state_tuple >> (NUM_TAPES)) & ((1 << NUM_TAPES) - 1),
            dirs = next_state_tuple & ((1 << NUM_TAPES) - 1);
        
        state = next_state;
        for (int j = 0; j < NUM_TAPES; ++j)
            heads[j] = edit_tape(tapes[j], heads[j], (vals >> j) & 1, (dirs >> j) & 1);
    }
    return state & 1;
}

int main() {
//	freopen("key.txt", "r", stdin);
	freopen("output.txt", "w", stdout);
    
    ifstream keyin("key.txt");
    ofstream fout("truth_table.txt");
    
//    ofstream keyout("key.txt");
//    gen_random_key(keyout);
//    keyout.close();
    
    string key_string;
    keyin >> key_string;
    
    assert(key_string.size() >= KEY_LENGTH);
    load_TM(TM, key_string);

    vector<int> output;
    for (int64_t inp = 0; inp < OUTPUT_LENGTH; ++inp)
        output.push_back(emulate_TM(TM, inp));
    for (int val : output)
        fout << val;
    fout << endl;
    
    /*
    int sum = 0;
    for (int val : output) {
        sum += val;
    }
    std::cout << static_cast<double>(sum) / output.size() << std::endl;
	
	vector< pair< double, pair<int, int>>> stats;
	
	for (int node = 0; node < N; ++node)
		for (int idx = 0; idx < (1 << NUM_TAPES); ++idx)    
//			cout << node << " " << idx << " " << (double)freq[node][idx] / tot << endl;
			stats.push_back(make_pair((double)freq[node][idx] / tot, make_pair(node, idx)));
	sort(stats.begin(), stats.end());
	reverse(stats.begin(), stats.end());
	for (int i = 0; i < stats.size(); ++i)
		cout << stats[i].first << " " << stats[i].second.first << " " << stats[i].second.second << endl;
	*/

    return 0;
}
