/**************************************************************************/
// FILE NAME: 310510221.cpp
// VERSRION: 1.0
// DATE: Oct 25, 2022
// AUTHOR: Kuan-Wei Chen / NYCU IEE Oasis Lab / 310510221
// CODE TYPE: CPP
// DESCRIPTION: 2022 Fall Computer Aided Design (CAD) / Homework1
// MODIFICATION HISTORY: 
// Date                 Description
// 2022/10/26           Implement Quine McCluskey Algorithm
//
/**************************************************************************/
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <math.h>

using namespace std;

bool debug = true; // for debug
int MAX_PI_NUM = 15;


int inVar; // input variables
vector<int> onSet; // on-set
vector<int> dcSet; // don't care set

vector<string> imp; // implicant
vector<string> pi;  // prime implicant
vector<string> essential_pi; // essential prime implicant

vector<string> sorted_pi; // sorted prime implicant
vector<string> sorted_essential_pi; // sorted essential prime implicant
int literal = 0;

class Quine_McCluskey{	
	public:		
		// Covert dec(int) to binary(string) //
		string dec2bin(int dec){
			// if(debug) cout << "dec: " << dec << endl;
			string str_bin;
			
			for(int i = 0; i < inVar; i++){
				if(dec == 0){
					str_bin += '0';
				}
				else{
					str_bin += '0' + dec % 2;
					dec /= 2;
				}
			}
			reverse(str_bin.begin(), str_bin.end());
			// printf("str_bin: %s\n", str_bin.c_str());
			return str_bin;
		}
	
		// Get binary implicant from onSet and dcSet, and generate implication table //
		vector<string> gen_implication_table(){
			vector<string> imp; // implicant
			for(int i = 0; i < onSet.size(); i++){
				imp.push_back(dec2bin(onSet[i])); // push onSet into implicant table
			}
			for(int i = 0; i < dcSet.size(); i++){
				imp.push_back(dec2bin(dcSet[i]));
			}
			
			// Print implicant
			if(debug){
				for(int i = 0; i < imp.size(); i++){
					cout << imp[i] << endl;
				}
			}
			
			return imp;
		}
		
		// Grouping //
		vector< vector<string> > group_imp(){
			vector<vector <string> > group;
			group.resize(inVar + 1); // 0, 1, 2, ..., inVar
			// Example: group[groupidx]
			// group[0] = {"0000"}
			// group[1] = {"0001", "0010", "0100", "1000"}
			// group[2] = {"0011", "0101", "0110", "1001", "1010", "1100"}
			// ...
			// group[4] = {"1111"}
			
			vector<string> tmp_imp;
			int group_idx;
			int curr_imp;
			
			// Grouping
			for(int i = 0; i < imp.size(); i++){
				group_idx = 0;
				for(int j = 0; j < inVar; j++){
					if(imp[i][j] == '1'){
						group_idx++;
					}
				}
				group[group_idx].push_back(imp[i]);
			}
			
			// Sorting
			for(int i = 0; i < group.size(); i++){
				for(int j = 0; j < group[i].size(); j++){
					sort(group[i].begin(), group[i].end());
				}
			}
			
			// Show group
			if(debug){
				for(int i = 0; i <= inVar; i++){
					cout << "[Group " << i << "]" << endl;
					
					for(int j = 0; j < group[i].size(); j++){
						cout << group[i][j] << endl;
					}
					cout << "---------" << endl;	
				}
			}
			
			// Same order with the group
			for(int i = 0; i < inVar; i++){
				for(int j = 0; j < group[i].size(); j++){
					tmp_imp.push_back(group[i][j]);
				}
			}
			
			imp.clear(); // clear imp vector
			
			for(int i = 0; i < tmp_imp.size(); i++){
				imp.push_back(tmp_imp[i]);
			}
			return group;
		}
		
		// Merge //
		string merge(string str_1, string str_2){
			int cnt = 0;
			for(int i = 0; i < str_1.length(); i++){
				if(str_1[i] != str_2[i]){
					str_1[i] = '-';
					cnt ++;
				}
			}
			if(cnt == 1){
				// if(debug) cout << str_1 << endl;
				return str_1;
			}
			
			return "x";
		}
		
		// Generate Prime Implicant //
		void find_PI(vector< vector<string>> group){	
			string merge_imp;
			vector<bool> prime_flag(imp.size(), true); // 1: can merge(IMP), 0: cannot merge(PI)
			vector<string> tmp_imp;
			tmp_imp.clear();
			
			int cur_group_pos = 0;
			int nxt_group_pos = 0;
			
			if(debug){
				cout << "Initialize PI flag" << endl;
				for(int i = 0; i < imp.size(); i++){
					cout << prime_flag[i] << " ";
				}
				cout << endl << endl;
			}
			
			if(debug) cout << "after merge" << endl;
			for(int i = 0; i < inVar; i++){ // group index
				if(debug) cout << "[Group " << i << "]" << endl;
				nxt_group_pos += group[i].size();
				// if(debug) cout << "current pos: " << cur_group_pos << endl;
				for(int j = 0; j < group[i].size(); j++){
					for(int k = 0; k < group[i+1].size(); k++){
						merge_imp = merge(group[i][j], group[i+1][k]);
						if(merge_imp != "x"){ // is not a prime implicant
							// if merg_imp is not "x", it means that it can merge, so it is not a prime implicant
							
							if(debug) cout << merge_imp << endl;
							prime_flag[cur_group_pos+j] = false;
							prime_flag[nxt_group_pos+k] = false;
							
							tmp_imp.push_back(merge_imp);
						}
					}
				}
				cur_group_pos += group[i].size();
				if(debug) cout << "---------" << endl;
			}
			
			if(debug) cout << endl;
			
			if(debug){
				cout << "PI flag" << endl;
				for(int i = 0; i < imp.size(); i++){
					cout << prime_flag[i] << " ";
				}
				cout << endl;
				cout << "------------------------------" << endl << endl;
			}
			
			// find prime implicnat
			for(int i = 0; i < imp.size(); i++){
				if(prime_flag[i] == true){
					
					pi.push_back(imp[i]);
				}
			}
			
			// clear imp vector
			imp.clear();
			
			// remove duplicates elements (tmp_imp)
			sort(tmp_imp.begin(), tmp_imp.end());
			tmp_imp.erase(unique(tmp_imp.begin(), tmp_imp.end()), tmp_imp.end());
			
			// push new imp
			for(int i = 0; i < tmp_imp.size(); i++){
				imp.push_back(tmp_imp[i]);
			}
		}
		
		// Prime Implicant Generation //
		void PI_generation(){
			if(debug) cout << "####### PI Generation Start #######" << endl;
			if(debug) cout << "number of input imp: " << imp.size() << endl << endl;
			vector< vector<string> > group;
			int round = 1;
			
			while(!imp.empty()){
				if(debug) cout << "////////////// Round " << round << " /////////////" << endl << endl;
				
				///////////////////////////////////////////////////////////////////////////////
				// Grouping
				if(debug) cout << "============ Grouping =============" << endl;
				group = group_imp();
				///////////////////////////////////////////////////////////////////////////////
				// Merge and Generate PI
				if(debug) cout << "============= Merging =============" << endl;
				find_PI(group);
				///////////////////////////////////////////////////////////////////////////////
				round++;
				
				if(debug){
					cout << "Prime Implicants (pi): " << endl;
					for(int i = 0; i < pi.size(); i++){
						cout << pi[i] << endl;
					}
					cout << endl;
					cout << "Implicants (imp): " << endl;
					for(int i = 0; i < imp.size(); i++){
						cout << imp[i] << endl;
					}
					cout << endl;
				}
			}
			if(debug) cout << "####### PI Generation Finish ######" << endl << endl;
		}
		
		// Column Covering //
		void column_covering(){
			vector< vector<bool> > column_coverage_table(onSet.size()); // 2-D table
			vector< vector <int> > pi_dec(pi.size());
			// ex: 
			// pi[0] = 0-00   / pi_dec[0] = {0,4}
			// pi[1] = -000   / pi_dec[0] = {0,8}
			// ...
			// pi[6] = -1-1  / pi_dec[0] = {5,7,13,15}
			vector<int> dc_value_set;
			
			int on_dec;
			int dc_dec;
			int exp;
			
			
			// Initialize column coverage table
			for(int i = 0; i < onSet.size(); i++){
				for(int j = 0; j < pi.size(); j++){
					column_coverage_table[i].push_back(false);
				}
			}
			
			if(debug){
				cout << "-Initialize Column Coveraging Table-" << endl;
				cout << "      ";
				for(int i = 0; i < onSet.size(); i++){
					if(onSet[i] > 9){
						cout << onSet[i] << " ";
					}
					else{
						cout << " " << onSet[i] << " ";
					}	
				}
				cout << endl << "----------------------------------" << endl;
				for(int j = 0; j < pi.size(); j++){
					cout << pi[j] << " | ";
					for(int i = 0; i < onSet.size(); i++){
						cout << column_coverage_table[i][j] << "  ";
					}
					cout << endl;
				}
				cout << endl;
			}
			
			// Read PI string (reverse)
			for(int i = 0; i < pi.size(); i++){
				on_dec = 0;
				exp = 0;
				
				dc_value_set.clear();
				dc_value_set.push_back(0);
				for(int j = inVar - 1; j >= 0; j--){
					dc_dec = 0;
					if(pi[i][j] == '1'){
						on_dec += pow(2, exp);
					}
					if(pi[i][j] == '-'){
						dc_dec = pow(2, exp);
						
						// Permuation all possible don't care value
						int dc_value_set_size = dc_value_set.size();
						for(int k = 0; k < dc_value_set_size; k++){
							dc_value_set.push_back(dc_value_set[k] + dc_dec);
						}	
					}
					exp++;
				}
				
				// Show DC possible value
				// if(debug){
				// 	cout << "DC possible value: " << endl;
				// 	for(int i = 0; i < dc_value_set.size(); i++){
				// 		cout << dc_value_set[i] << " ";
				// 	}
				// 	cout << endl;
				// }
				
				// remove same element
				sort(dc_value_set.begin(), dc_value_set.end());
				dc_value_set.erase(unique(dc_value_set.begin(), dc_value_set.end()), dc_value_set.end());
				
				// Push PI possible dec value
				for(int j = 0; j < dc_value_set.size(); j++){
					pi_dec[i].push_back(on_dec + dc_value_set[j]);
				}
				
				// Print possible PI
				if(debug){
					cout << "PI possible value" << endl;
					cout << pi[i] << endl;
					for(int j = 0; j < pi_dec[i].size(); j++){
						cout << pi_dec[i][j] << " ";
					}
					cout << endl << "----------------------------------" << endl;
				}	
			}
			
			// modify table
			for(int k = 0; k < onSet.size(); k++){
				for(int i = 0; i < pi.size(); i++){
					for(int j = 0; j < pi_dec[i].size(); j++){
						if(pi_dec[i][j] == onSet[k]){
							column_coverage_table[k][i] = 'x';
						}
					}	
				}
			}
			
			if(debug){
				cout << "    - Column Coveraging Table -    " << endl;
				cout << "      ";
				for(int i = 0; i < onSet.size(); i++){
					if(onSet[i] > 9){
						cout << onSet[i] << " ";
					}
					else{
						cout << " " << onSet[i] << " ";
					}	
				}
				cout << endl << "----------------------------------" << endl;
				for(int j = 0; j < pi.size(); j++){
					cout << pi[j] << " | ";
					for(int i = 0; i < onSet.size(); i++){
						cout << column_coverage_table[i][j] << "  ";
					}
					cout << endl;
				}
			}	
		
			find_essential_PI(column_coverage_table, pi_dec);
		}
		
		// Find Essential Prime Implicant //
		void find_essential_PI(vector< vector<bool> > column_coverage_table, vector< vector <int> > pi_dec){
			vector<bool> essential_pi_flag;
			vector<bool> col_sel_flag;
			vector<int> col_pi_cnt;
			
			///////////////////////////////////////////////////////////////////////////////
			// Step 0: Initialize
			
			// Initialize essential_pi_flag
			for(int i = 0; i < pi.size(); i++){
				essential_pi_flag.push_back(false);
			}
			
			// Initialize col_sel_flag
			for(int i = 0; i < onSet.size(); i++){
				col_sel_flag.push_back(false);
			}
			
			// Initialize col_pi_cnt
			for(int i = 0; i < onSet.size(); i++){
				col_pi_cnt.push_back(0);
			}
			
			// Print essential_pi_flag & col_pi_cnt
			// if(debug){
			// 	for(int i = 0; i < pi.size(); i++){
			// 		cout << essential_pi_flag[i] << " ";
			// 	}
			// 	cout << endl;
			// 	for(int i = 0; i < onSet.size(); i++){
			// 		cout << col_pi_cnt[i] << " ";
			// 	}
			// 	cout << endl;
			// }
			
			///////////////////////////////////////////////////////////////////////////////
			// Step 2: Find essential prime implicant.
			
			// If column has a single 'x', then the implicant associated with the row is essential.
			for(int i = 0; i < onSet.size(); i++){ // Column index
				for(int j = 0; j < pi.size(); j++){ // Row index
					if(column_coverage_table[i][j] == true){
						col_pi_cnt[i] += 1; 
					}	
				}
				if(col_pi_cnt[i] == 1){
					for(int j = 0; j < pi.size(); j++){
						if(column_coverage_table[i][j] == true){
							essential_pi_flag[j] = true; 
						}	
					}
				}
			}
			
			// Push Essential PI
			for(int i = 0; i < essential_pi_flag.size(); i++){
				if(essential_pi_flag[i] == true){
					essential_pi.push_back(pi[i]);
				}
			}
			
			// Show col_pi_cnt
			if(debug){
				cout << "----------------------------------" << endl << "Total: ";
				for(int i = 0; i < onSet.size(); i++){
					cout << col_pi_cnt[i] << "  ";
				}
				cout << endl;
			}
			
			// Show Essential PI
			if(debug){
				cout << "----------------------------------" << endl;
				cout << "Essential PI: " << endl;
				for(int i = 0; i < essential_pi.size(); i++){
					cout << essential_pi[i] << endl;
				}
				cout << endl;
			}
			
			// Eliminate all columns coveraged by essential primes
			for(int i = 0; i < onSet.size(); i++){ // Column index
				for(int j = 0; j < pi.size(); j++){ // Row index
					if(essential_pi_flag[j] == true){
						if(column_coverage_table[i][j] == true){
							col_sel_flag[i] = true; 
						}
					}
				}
			}
			
			// Show col_sel_flag
			if(debug){
				cout << "----------------------------------" << endl;
				cout << "Column selected: " << endl;
				for(int i = 0; i < col_sel_flag.size(); i++){
					cout << col_sel_flag[i] << " ";
				}
				cout << endl << endl;
			}	
			
			
			///////////////////////////////////////////////////////////////////////////////
			// Step 3: Find minimum set of row that cover the remaining columns
			int remain_column_cnt;
			int max_cover;
			int round_min_coverage;
			bool selected;
			vector<int> column_coverage_cnt;
			
			round_min_coverage = 1;
			
			
			remain_column_cnt = 0;
			for(int i = 0; i < col_sel_flag.size(); i++){
				if(col_sel_flag[i] == false){
					remain_column_cnt++;
				}
			}
			
			while(remain_column_cnt > 0){
				
				if(debug) cout << "---- Round (min coverage): " << round_min_coverage << " ----" << endl << endl;
				
				max_cover = 0;
				for(int i = 0; i < pi.size(); i++){
					column_coverage_cnt.push_back(0);
				}
				
				// Botton-Up (count which pi cover the most column)
				for(int i = 0; i < col_sel_flag.size(); i++){
					if(col_sel_flag[i] == false){
						for(int j = pi.size()-1; j >= 0; j--){
							if(essential_pi_flag[j] == false && column_coverage_table[i][j] == true){
								column_coverage_cnt[j]++;
							}
							if(column_coverage_cnt[j] > max_cover){
								max_cover = column_coverage_cnt[j];
							}
						}
					}	
				}
				
				// Show column coverage cnt
				if(debug){
					cout << "Column coverage cnt: " << endl;
					for(int i = 0; i < pi.size(); i++){
						cout << pi[i] << ": " << column_coverage_cnt[i] << endl;
					}
					cout << endl;
				}
				
				
				selected = false;
				// Set essential pi
				for(int i = 0; i < col_sel_flag.size(); i++){
					if(selected == true){
						break;
					}
					if(col_sel_flag[i] == false){
						for(int j = pi.size()-1; j >= 0; j--){
							// Set essential_pi flag & push essential_pi
							if(essential_pi_flag[j] == false && column_coverage_cnt[j] == max_cover){
								selected = true;
								essential_pi_flag[j] = true;
								essential_pi.push_back(pi[j]);
								for(int k = 0; k < col_sel_flag.size(); k++){
									if(column_coverage_table[k][j] == true){
										col_sel_flag[k] = true;
									}	
								}
								break;
							}
							
						}
					}
				}
				
				// Show Essential PI
				if(debug){
					cout << "----------------------------------" << endl;
					cout << "Essential PI: " << endl;
					for(int i = 0; i < essential_pi.size(); i++){
						cout << essential_pi[i] << endl;
					}
					cout << endl;
				}
				
				
				// Show col_sel_flag
				if(debug){
					cout << "----------------------------------" << endl;
					cout << "Column selected: " << endl;
					for(int i = 0; i < col_sel_flag.size(); i++){
						cout << col_sel_flag[i] << " ";
					}
					cout << endl;
				}
				
				remain_column_cnt = 0;
				for(int i = 0; i < col_sel_flag.size(); i++){
					if(col_sel_flag[i] == false){
						remain_column_cnt++;
					}
				}
				
				round_min_coverage++;				
			}
		}
};


// Sort Result //
vector<string> sort_result(vector<string> imp){
	vector<string> sorted_imp;
	sort(imp.begin(), imp.end());
	
	for(int i = 0; i < imp.size(); i++){
		sorted_imp.push_back(imp[i]);
	}
	// Show sorted result
	if(debug){
		for(int i = 0; i < sorted_imp.size(); i++){
			cout << sorted_imp[i] << endl;
		}
		cout << endl;
	}
	return sorted_imp;
}

// Count Literal //
int count_literal(vector<string> str){
	int literal = 0;
	for(int i = 0; i < str.size(); i++){
		for(int j = 0; j < str[i].size(); j++){
			if(str[i][j] == '0' || str[i][j] == '1'){
				literal++;
			}
		}
	}
	
	if(debug) cout << "literal: " << literal << endl;
	
	return literal;
}



// Write Output File //
void write_ouput(ofstream &fout, vector<string> pi, vector<string> essential_pi, int literal){
	int pi_num = 0;
	
	fout << ".p " << pi.size() << endl;
	for(int i = 0; i < pi.size(); i++){
		pi_num ++;
		fout << pi[i] << endl;
		if(pi_num >= 15){
			break;
		}
	}
	
	fout << endl;
	
	fout << ".mc " << essential_pi.size() << endl;
	for(int i = 0; i < essential_pi.size(); i++){
		fout << essential_pi[i] << endl;
	}
	fout << "literal=" << literal;
}



int main(int argc, char *argv[]){
	string line;
	string str, onset, dcset;
	
	ifstream fin(argv[1]);
	ofstream fout(argv[2]);
	
	if(debug) cout << "Open file" << endl;
	
	// Read Input
	while(getline(fin, line)){
		istringstream sin(line);
		sin >> str;
		// if(debug) cout << str << endl;
		if(str == ".i"){
			getline(fin, line);
			stringstream var(line);
			var >> inVar;
		}
		if(str == ".m"){
			getline(fin, line);
			stringstream onset_in(line);
			while(onset_in >> onset){
				onSet.push_back(stoi(onset)); // string to int
				// if(debug) if(debug) cout << stoi(onset) << endl;
			}
		}
		if(str == ".d"){
			getline(fin, line);
			stringstream dcset_in(line);
			while(dcset_in >> dcset){
				dcSet.push_back(stoi(dcset)); // string to int
				// if(debug) cout << stoi(dcset) << endl;
			}
		}
	}
	
	Quine_McCluskey QM;
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug){
		cout << "============ Input Info ============" << endl;
		cout << "inVar: " << inVar << endl << endl;
		
		cout << "onSet size: " << onSet.size() << endl;
		cout << "onSet: " << endl;
		for(int i = 0; i < onSet.size(); i++){
			cout << QM.dec2bin(onSet[i]) << " ";
		}
		
		cout << endl << endl;
		
		cout << "dcSet size: " << dcSet.size() << endl;
		cout << "dcSet: " << endl;
		for(int i = 0; i < dcSet.size(); i++){
			cout << QM.dec2bin(dcSet[i]) << " ";
		}
		cout << endl << endl;;
	}
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug) cout << "===== Quine McCluskey Algorithm ====" << endl << endl;
	if(debug) cout << "========== Generate Table ==========" << endl;
	imp = QM.gen_implication_table();
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug) cout << "=========== PI Generation ==========" << endl;
	QM.PI_generation();
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug) cout << "========== Column Covering =========" << endl;
	QM.column_covering();
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug) cout << "============== Sort PI =============" << endl;
	if(debug) cout << "p = " << pi.size() << endl;
	sorted_pi = sort_result(pi);
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug) cout << "========= Sort Essential PI ========" << endl;
	if(debug) cout << "mc = " << essential_pi.size() << endl;
	sorted_essential_pi = sort_result(essential_pi);
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug) cout << "=========== Count Literal ==========" << endl;
	literal = count_literal(sorted_essential_pi);
	
	///////////////////////////////////////////////////////////////////////////////
	if(debug) cout << "========= Write Ouput File =========" << endl;
	write_ouput(fout, sorted_pi, sorted_essential_pi, literal);
	
	if(debug) cout << "Write Ouput File Finished !" << endl;
	
	
	fin.close();
	fout.close();
	if(debug) cout << "Close file" << endl;
	
	return 0;
}
