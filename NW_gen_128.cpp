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
#include <thread>
using namespace std;

const bool MULTI_THREADING = 1;
const int THREAD_NUM = 8;
const int P = 607, Q = 1217, L = P * 2, CHUNK_SIZE = 60, CHUNK_NUM = P / CHUNK_SIZE + 1;
/* OUTPUT_LENGTH <= Q ** 3 */
/* Truth_table_length >= OUTPUT_LENGTH * (2 ** 3) */
const int OUTPUT_LENGTH = 6760 * 2, NW_SEEDLENGTH = Q * L;

struct Big_Integer
{
	int64_t a[CHUNK_NUM];

	Big_Integer() {
		memset(a, 0, sizeof(a));
	}
	
	Big_Integer(int b) {
		memset(a, 0, sizeof(a));
		
		a[0] = b;
	}
	
	inline Big_Integer(const vector<int>& b) {
		assert( b.size() <= P );
		
		memset(a, 0, sizeof(a));
		for (int i = 0; i < b.size(); ++i)
			a[i / CHUNK_SIZE] |= ((int64_t) (b[i] & 1)) << (i % CHUNK_SIZE);
	}
	
	inline vector<int> to_vector() const {
		vector<int> output;
		
		for (int i = 0; i < P; ++i)
			output.push_back( (a[i / CHUNK_SIZE] >> (i % CHUNK_SIZE)) & 1 );
		
		return output;
	}
	
	inline Big_Integer operator +(const Big_Integer& b) const {
		int64_t c[CHUNK_NUM];
		
		for (int i = 0; i < CHUNK_NUM; ++i)
			c[i] = (int64_t) a[i] + b.a[i];
		
		Big_Integer output;
		for (int i = 0; i < CHUNK_NUM - 1; ++i)
		{
			c[i + 1] += c[i] >> CHUNK_SIZE;
			output.a[i] = c[i] & ((((int64_t) 1) << CHUNK_SIZE) - 1);
		}
		output.a[CHUNK_NUM - 1] = c[CHUNK_NUM - 1] & ((((int64_t) 1) << (P % CHUNK_SIZE)) - 1);
		return output;
	}
	
	inline Big_Integer operator *(const Big_Integer& b) const {
		__uint128_t c[CHUNK_NUM];
		
		memset(c, 0, sizeof(c));
		for (int i = 0; i < CHUNK_NUM; ++i)
			for (int j = 0; j < CHUNK_NUM - i; ++j)
				c[i + j] += (__uint128_t) a[i] * b.a[j];
		
		Big_Integer output;
		for (int i = 0; i < CHUNK_NUM - 1; ++i)
		{
			c[i + 1] += (int64_t) (c[i] >> CHUNK_SIZE);
			output.a[i] = c[i] & ((((__uint128_t) 1) << CHUNK_SIZE) - 1);
		}
		output.a[CHUNK_NUM - 1] = c[CHUNK_NUM - 1] & ((((__uint128_t) 1) << (P % CHUNK_SIZE)) - 1);
		return output;
	}
};

struct Polynomial
{
	vector<Big_Integer> coeffs;
	
	Polynomial() {}
	inline Polynomial(const string& message) {
		for (int i = 0; i * P < message.size(); ++i)
		{
			vector<int> number;
			for (int j = i * P; j < min(i * P + P, (int) message.size()); ++j)
				number.push_back( message[j] - '0' );
			coeffs.push_back( Big_Integer(number) );
		}
	}
	
	inline Big_Integer eval(Big_Integer x) const {
		Big_Integer fx = 0;
		
		for (Big_Integer coeff : coeffs)
		{
//			fx = fx + (coeff * y);
//			y = y * x;
			fx = fx * x;
			fx = fx + coeff;
		}
		
		return fx;
	}
};

vector<int> get_design(int idx)
{
	int a = idx / Q, b = idx % Q;
	
	vector<int> design;
	for (int i = 0; i < L; ++i)
		design.push_back( i * Q + (a * i + b) % Q );
	
	return design;
}
int read_from_seed(const string& NW_seed, int pos)
{
	char ch = NW_seed[ pos / 4 ];
	int val = (ch >= '0' && ch <= '9') ? (ch - '0') : (ch - 'a' + 10);
	
	return (val >> (pos & 3)) & 1;
}
vector<int> slice(const string& NW_seed, const vector<int>& design)
{
	vector<int> input;
	for (int pos : design)
		input.push_back( read_from_seed(NW_seed, pos) );
	
	return input;
}
Big_Integer ReedSolomon_eval(const Polynomial& truth_table_poly, const Big_Integer& idx)
{
	return truth_table_poly.eval(idx);
}
int Hadamard_eval(const vector<int>& a, const vector<int>& b)
{
	assert( (a.size() == P && b.size() == P) );
	
	int z = 0;
	for (int i = 0; i < a.size(); ++i)
		z ^= a[i] & b[i];
	
	return z;
}
int eval_NW(int idx, const string& NW_seed, const Polynomial& truth_table_poly)
{
	vector<int> design = get_design(idx);
	vector<int> input = slice(NW_seed, design);
	
	vector<int> ReedSolomon_idx_v, Hadamard_idx_v;
	for (int i = 0; i < P; ++i)
		ReedSolomon_idx_v.push_back( input[i] ),
		Hadamard_idx_v.push_back( input[i + P] );
	
	Big_Integer ReedSolomon_idx( ReedSolomon_idx_v );
	Big_Integer codeword_block = ReedSolomon_eval( truth_table_poly, ReedSolomon_idx );
	
	vector<int> codeword_block_v = codeword_block.to_vector();
	
	return Hadamard_eval( codeword_block_v, Hadamard_idx_v );
}
void eval_NW_multi_threading( int start, int end, int output[], const string& NW_seed, const Polynomial& truth_table_poly)
{
//	vector<int> output;
	for (int idx = start; idx < end; ++idx)
		output[idx] = eval_NW(idx, NW_seed, truth_table_poly);
//	return output;
}
int main()
{
//	freopen("output1.txt", "w", stdout);
//	__uint128_t aaa;

	ifstream truth_table_in("truth_table.txt");	
    ifstream NW_seed_in("randomness1_raw.txt");
    ofstream fout("prg_output.txt");
    
	string NW_seed, truth_table;
    
    NW_seed_in >> NW_seed;
    
    assert( NW_seed.size() * 4 >= NW_SEEDLENGTH );
    
    truth_table_in >> truth_table;
    Polynomial truth_table_poly = Polynomial(truth_table);
    
    assert( OUTPUT_LENGTH <= Q * Q );
    
//    vector<int> output;
	int output[OUTPUT_LENGTH];
	memset(output, 0, sizeof(output));
    
    if (not MULTI_THREADING)
    {
    	for (int64_t idx = 0; idx < OUTPUT_LENGTH; ++idx)
//        	output.push_back(eval_NW(idx, NW_seed, truth_table_poly));
			output[idx] = eval_NW(idx, NW_seed, truth_table_poly);
    }
    else
    {
    	vector<thread> t;
    	for (int i = 0; i < THREAD_NUM; ++i)
    		t.push_back( thread(eval_NW_multi_threading, OUTPUT_LENGTH / THREAD_NUM * i, min(OUTPUT_LENGTH / THREAD_NUM * (i + 1), OUTPUT_LENGTH), output, NW_seed, truth_table_poly ) );
    	for (int i = 0; i < THREAD_NUM; ++i)
    		t[i].join();
    }
        
    
    for (int i = 0; i < OUTPUT_LENGTH; ++i)
        fout << output[i];
    fout << endl;

	return 0;
}
