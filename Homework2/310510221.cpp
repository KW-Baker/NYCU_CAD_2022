/**************************************************************************/
// FILE NAME: 310510221.cpp
// VERSRION: 1.0
// DATE: Nov 18, 2022
// AUTHOR: Kuan-Wei Chen / NYCU IEE Oasis Lab / 310510221
// CODE TYPE: CPP
// DESCRIPTION: 2022 Fall Computer Aided Design (CAD) / Homework2
// MODIFICATION HISTORY: 
// Date                 Description
// 2022/11/18           Implement Timing Analyzer with False Path Detection
//
// ReadMe
// g++ -std=c++11 310510221.cpp -o 310510221.o
// ./310510221.o <netlist_file> -p <input.pat> -l <testlib.lib>
/**************************************************************************/
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <float.h>

#define DEBUG false
#define ID "310510221"
#define TABLE10_ROW_NUM 7
#define OUTPUT_LOADING 0.03

using namespace std;

vector<string> input_node;
vector<string> output_node;
vector<string> wire;

vector<double> total_output_cap;
vector<double> input_transition;

vector<int> cells_sort;

// table
vector< vector<double> > nor_cell_rise,  nor_cell_fall; // cell delay when output is rising & falling
vector< vector<double> > nor_rise_tran,  nor_fall_tran; // output rising & falling time
vector< vector<double> > inv_cell_rise,  inv_cell_fall; 
vector< vector<double> > inv_rise_tran,  inv_fall_tran;
vector< vector<double> > nand_cell_rise, nand_cell_fall;
vector< vector<double> > nand_rise_tran, nand_fall_tran;

string case_name, case_name_check;
string output_load_filePath;
string output_delay_filePath;
string output_path_filePath;

double nor2x1_a1_cap, nor2x1_a2_cap;
double invx1_i_cap;
double nandx1_a1_cap, nandx1_a2_cap;

int pattern_num = 0;

typedef struct STEP1{
	string name;
	double load;
} STEP1;


typedef struct STEP2{
	string name;
	bool logic_output;
	double delay;
	double trans_time;
} STEP2;


typedef struct STEP3{
	double longest_delay;
	double shortest_delay;
	vector<string> longest_path;
	vector<string> shortest_path;
} STEP3;


typedef struct CELL{
	string name;
	string gatetype;
	string in1;
	string in2;
	string out;
	double in1_cap;
	double in2_cap;
	double in_trans;
	double out_cap;
	bool in1_logic;
	bool in2_logic;
	bool out_logic;
	double delay;
	double total_delay;
	double trans;
	vector<int> pre_idx;
	vector<int> nxt_idx;
	vector<string> pre_input;
	vector<string> nxt_input;
	vector<string> long_path;
} CELL;


typedef struct PATTERN{
	vector<string> inputs;
	vector< vector<bool> > values;
} PATTERN;


vector<STEP1> step1;
vector< vector<STEP2> > step2;
vector<STEP3> step3;
STEP3 output_paths; 

vector<CELL> cells;
PATTERN pattern;


class Graph{
	private:
		int num_vertex;
		vector <list<int> > AdjList;
		int *flag;
		int *predecessor;
		int *discover;
		int *finish;

	public:
		Graph():num_vertex(0){};
		Graph(int N):num_vertex(N){
			AdjList.resize(num_vertex);
		}

		void AddEdgeList(int from, int to);
		void DFS();
		void DFSVisit(int vertex, int &time);
		void TopologicalSort();

		friend void QuickSort(int *vec, int front, int end, int *vec2);
		friend int Partition(int *vec, int front , int end, int *vec2);
		friend void swap(int *x, int *y);
};


void Graph::AddEdgeList(int from, int to){
	AdjList[from].push_back(to);
}


void Graph::DFS(){
	flag = new int[num_vertex];
	discover = new int[num_vertex];
	finish = new int[num_vertex];
	predecessor = new int[num_vertex];

	// Initialize
	int time = 0;
	for(auto i = 0; i < num_vertex; i++){
		flag[i] = 0;
		discover[i] = 0;
		finish[i] = 0;
		predecessor[i] = -1 ;
	}

	for(auto i = 0; i < num_vertex; i++){
		if(flag[i] == 0)
			DFSVisit(i, time);
	}
}


void Graph::DFSVisit(int vertex, int &time){
	flag[vertex] = 1;
	discover[vertex] = ++time;
	for(list<int>::iterator itr = AdjList[vertex].begin(); itr != AdjList[vertex].end(); itr++){
		if(flag[*itr] == 0){
			predecessor[*itr] = vertex;
			DFSVisit(*itr, time);
		}
	}
	flag[vertex] = 2;
	finish[vertex] = ++time;
}


void swap(int *x, int *y){
	int tmp = *x;
	*x = *y;
	*y = tmp;
}


int Partition(int *vec, int front, int end, int *vec2){
	int pivot = vec[end];
	int i = front - 1;
	for(auto j = front; j < end; j++){
		if(vec[j] > pivot){
			i++;
			swap(&vec[i], &vec[j]);
			swap(&vec2[i], &vec2[j]);
		}
	}
	swap(&vec[i + 1], &vec[end]);
	swap(&vec2[i + 1], &vec2[end]);

	return i + 1;
}


void QuickSort(int *vec, int front, int end, int *vec2){
	if(front < end){
		int pivot = Partition(vec, front, end, vec2);
		QuickSort(vec, front, pivot - 1, vec2);
		QuickSort(vec, pivot + 1, end, vec2);
	}
}


void Graph::TopologicalSort(){
	DFS();

	int finishLargetoSmall[num_vertex];
    for(auto i = 0; i < num_vertex; i++)
        finishLargetoSmall[i] = i;
    
    QuickSort(finish, 0, num_vertex - 1, finishLargetoSmall);

    for(auto i = 0; i < num_vertex; i++)
    	cells_sort.push_back(finishLargetoSmall[i]);
}


// Remove comments
string removeComments(string prgm){
	string res;

	// Flags to indicate the single line and multiple lines comments
	bool s_cmt = false;
	bool m_cmt = false;

	// Traverse the given program
	for(auto i = 0; i < prgm.length(); i++){
		// If single line comment flag is on, then check for end for it
		if(s_cmt == true && prgm[i] == '\n')
			s_cmt = false;

		// If multiple line comment is on, them check for end of it
		else if(m_cmt == true && prgm[i] == '*' && prgm[i+1] == '/'){
			m_cmt = false;
			i++;
		}

		// If this character is in a comment, ignore it
		else if(s_cmt || m_cmt)
			continue;

		// Check for beggining of comments and set the corresponding flag
		else if(prgm[i] == '/' && prgm[i+1] == '/'){
			s_cmt = true;
			i++;
		}
		else if(prgm[i] == '/' && prgm[i+1] == '*'){
			m_cmt = true;
			i++;
		}

		// If current character is a non-comment character, append it to res
		else
			res += prgm[i];
	}
	return res;
}


// Remove Substring
string remove_substr(string s, string p){
	s.erase(s.find(p), p.length());
  	return s;
}


// Parse Netlist
void parse_netlist(const char* filePath){
	string org_netlist; // original netlist
	string mdf_netlist; // modified netlist
	string line, str;

	string input_line;
	string output_line;
	string wire_line;
	string tmp_str;

	string str_name;
	string str_A1;
	string str_A2;
	string str_I;
	string str_ZN;

	size_t foundNOR2X1;
	size_t foundINVX1;
	size_t foundNANDX1;

	size_t foundA1;
	size_t foundA2;
	size_t foundI;
	size_t foundZN;

	size_t found_left;
	size_t found_right;

	stringstream ss;
	ss.clear();

	ifstream fin(filePath);

	#if DEBUG
		cout << "[Open netlist file]" << endl;
	#endif
     
	if(!fin){
		cout << "Error: Cannot open the netlist file!" << endl;
		return;
	}

	// Read netlist file
	while(getline(fin, line)){
		org_netlist += line;
		org_netlist += '\n';
	}
	// cout << org_netlist;

	// Remove Comment
	mdf_netlist = removeComments(org_netlist);
	
	/*
	#if DEBUG
		cout << "=============== Remove Netlist Comments ===============" << endl;
		cout << "Original netlist:" << endl;
		for(auto i = 0; i < org_netlist.size(); i++){
			cout << org_netlist[i];
			if(org_netlist[i] == '\n')
				cout << endl;
		}

		cout << endl << endl;

		cout << "Modified netlist:" << endl;
		for(auto i = 0; i < mdf_netlist.size(); i++){
			cout << mdf_netlist[i];
			if(mdf_netlist[i] == '\n')
				cout << endl;
		}
	#endif
	*/

	size_t found_input  = mdf_netlist.find("input");
	size_t found_output = mdf_netlist.find("output");
	size_t found_wire   = mdf_netlist.find("wire");

	for(auto i = 0; i < mdf_netlist.size(); i++){
		
		// input node
		if(i == found_input){
			i += strlen("input"); // skip the "input" string
			while(mdf_netlist[i] != ';'){
				input_line += mdf_netlist[i];
				i++;
			}
			input_line += ';';
		}

		// output node
		if(i == found_output){
			i += strlen("output"); // skip the "output" string
			while(mdf_netlist[i] != ';'){
				output_line += mdf_netlist[i];
				i++;
			}
			output_line += ';';
		}

		// wire
		if(i == found_wire){
			i += strlen("wire"); // skip the "wire" string
			while(mdf_netlist[i] != ';'){
				wire_line += mdf_netlist[i];
				i++;
			}
			wire_line += ';';
		}
	}

	tmp_str = "";
	for(auto i = 0; i < input_line.length(); i++){
		if(input_line[i] == ',' || input_line[i] == ';'){
			input_node.push_back(tmp_str);
			tmp_str = "";
		}
		// skip space, tap and newline
		else if(input_line[i] == ' ' || input_line[i] == '	' || input_line[i] == '\n')
			continue;
		else
			tmp_str += input_line[i];
	}

	tmp_str = "";
	for(auto i = 0; i < output_line.length(); i++){
		if(output_line[i] == ',' || output_line[i] == ';'){
			output_node.push_back(tmp_str);
			tmp_str = "";
		}
		// skip space, tap and newline
		else if(output_line[i] == ' ' || output_line[i] == '	' || output_line[i] == '\n')
			continue;
		else
			tmp_str += output_line[i];
	}
	
	tmp_str = "";
	for(auto i = 0; i < wire_line.length(); i++){
		if(wire_line[i] == ',' || wire_line[i] == ';'){
			wire.push_back(tmp_str);
			tmp_str = "";
		}
		// skip space, tap and newline
		else if(wire_line[i] == ' ' || wire_line[i] == '	' || wire_line[i] == '\n')
			continue;
		else
			tmp_str += wire_line[i];
	}
	
	ss << mdf_netlist;
	while(getline(ss, line, ';')){
		// NOR2X1
		if(line.find("NOR2X1") != string::npos){
			str_name = line;
			str_A1 = line;
			str_A2 = line;
			str_ZN = line;

			// get name
			foundNOR2X1 = str_name.find("NOR2X1");
			if(str_name.find("(") != string::npos)
				found_left = str_name.find("(");
			str_name.assign(str_name, foundNOR2X1 + strlen("NOR2X1") + 1, found_left - foundNOR2X1 - strlen("NOR2X1") - 1);
			str_name.erase(remove(str_name.begin(), str_name.end(), ' '), str_name.end()); // remove space

			// get str_A1 (in1)
			if(line.find(".A1") != string::npos){
				foundA1 = str_A1.find(".A1");
				str_A1.erase(str_A1.begin(), str_A1.begin() + foundA1);
			}
			if(str_A1.find("(") != string::npos)
				found_left = str_A1.find("(");
			if(str_A1.find(")") != string::npos)
				found_right = str_A1.find(")");
			str_A1.assign(str_A1, found_left + 1, found_right - found_left - 1);
			str_A1.erase(remove(str_A1.begin(), str_A1.end(), ' '), str_A1.end()); // remove space


			// get str_A2 (in2)
			if(line.find(".A2") != string::npos){
				foundA2 = str_A2.find(".A2");
				str_A2.erase(str_A2.begin(), str_A2.begin() + foundA2);
			}
			if(str_A2.find("(") != string::npos)
				found_left = str_A2.find("(");
			if(str_A2.find(")") != string::npos)
				found_right = str_A2.find(")");
			str_A2.assign(str_A2, found_left + 1, found_right - found_left - 1);
			// remove space
			str_A2.erase(remove(str_A2.begin(), str_A2.end(), ' '), str_A2.end());

			// get str_ZN (out)
			if(line.find(".ZN") != string::npos){
				foundZN = str_ZN.find(".ZN");
				str_ZN.erase(str_ZN.begin(), str_ZN.begin() + foundZN);
			}
			if(str_ZN.find("(") != string::npos)
				found_left = str_ZN.find("(");
			if(str_ZN.find(")") != string::npos)
				found_right = str_ZN.find(")");
			str_ZN.assign(str_ZN, found_left + 1, found_right - found_left - 1);
			str_ZN.erase(remove(str_ZN.begin(), str_ZN.end(), ' '), str_ZN.end()); // remove space

			cells.push_back({str_name, "NOR", str_A1, str_A2, str_ZN, nor2x1_a1_cap, nor2x1_a2_cap, 0, 0, 0, 0, 0, 0});
		}

		// INVX1
		else if(line.find("INVX1") != string::npos){
			str_name = line;
			str_I = line;
			str_ZN = line;
			
			// get name
			foundINVX1 = str_name.find("INVX1");
			if(str_name.find("(") != string::npos)
				found_left = str_name.find("(");
			str_name.assign(str_name, foundINVX1 + strlen("INVX1") + 1, found_left - foundINVX1 - strlen("INVX1") - 1);
			str_name.erase(remove(str_name.begin(), str_name.end(), ' '), str_name.end()); // remove space
			
			// get str_I (in)
			if(line.find(".I") != string::npos){
				foundI = str_I.find(".I");
				str_I.erase(str_I.begin(), str_I.begin() + foundI);
			}
			if(str_I.find("(") != string::npos)
				found_left = str_I.find("(");
			if(str_I.find(")") != string::npos)
				found_right = str_I.find(")");
			str_I.assign(str_I, found_left + 1, found_right - found_left - 1);
			// remove space
			str_I.erase(remove(str_I.begin(), str_I.end(), ' '), str_I.end());


			// get str_ZN (out)
			if(line.find(".ZN") != string::npos){
				foundZN = str_ZN.find(".ZN");
				str_ZN.erase(str_ZN.begin(), str_ZN.begin() + foundZN);
			}
			if(str_ZN.find("(") != string::npos)
				found_left = str_ZN.find("(");
			if(str_ZN.find(")") != string::npos)
				found_right = str_ZN.find(")");
			str_ZN.assign(str_ZN, found_left + 1, found_right - found_left - 1);
			str_ZN.erase(remove(str_ZN.begin(), str_ZN.end(), ' '), str_ZN.end()); // remove space
		
			cells.push_back({str_name, "INV", str_I, "None", str_ZN, invx1_i_cap, 0, 0, 0, 0, 0, 0, 0});
		}

		// NAND2X1
		else if(line.find("NANDX1") != string::npos){
			str_name = line;
			str_A1 = line;
			str_A2 = line;
			str_ZN = line;

			// get name
			foundNANDX1 = str_name.find("NANDX1");
			if(str_name.find("(") != string::npos)
				found_left = str_name.find("(");
			str_name.assign(str_name, foundNANDX1 + strlen("NANDX1") + 1, found_left - foundNANDX1 - strlen("NANDX1") - 1);
			str_name.erase(remove(str_name.begin(), str_name.end(), ' '), str_name.end()); // remove space

			// get str_A1 (in1)
			if(line.find(".A1") != string::npos){
				foundA1 = str_A1.find(".A1");
				str_A1.erase(str_A1.begin(), str_A1.begin() + foundA1);
			}
			if(str_A1.find("(") != string::npos)
				found_left = str_A1.find("(");
			if(str_A1.find(")") != string::npos)
				found_right = str_A1.find(")");
			str_A1.assign(str_A1, found_left + 1, found_right - found_left - 1);
			// remove space
			str_A1.erase(remove(str_A1.begin(), str_A1.end(), ' '), str_A1.end());


			// get str_A2 (in2)
			if(line.find(".A2") != string::npos){
				foundA2 = str_A2.find(".A2");
				str_A2.erase(str_A2.begin(), str_A2.begin() + foundA2);
			}
			if(str_A2.find("(") != string::npos)
				found_left = str_A2.find("(");
			if(str_A2.find(")") != string::npos)
				found_right = str_A2.find(")");
			str_A2.assign(str_A2, found_left + 1, found_right - found_left - 1);
			// remove space
			str_A2.erase(remove(str_A2.begin(), str_A2.end(), ' '), str_A2.end());


			// get str_ZN (out)
			if(line.find(".ZN") != string::npos){
				foundZN = str_ZN.find(".ZN");
				str_ZN.erase(str_ZN.begin(), str_ZN.begin() + foundZN);
			}
			if(str_ZN.find("(") != string::npos)
				found_left = str_ZN.find("(");
			if(str_ZN.find(")") != string::npos)
				found_right = str_ZN.find(")");
			str_ZN.assign(str_ZN, found_left + 1, found_right - found_left - 1);
			// remove space
			str_ZN.erase(remove(str_ZN.begin(), str_ZN.end(), ' '), str_ZN.end());
		
			cells.push_back({str_name ,"NAND", str_A1, str_A2, str_ZN, nandx1_a1_cap, nandx1_a2_cap, 0, 0, 0, 0, 0, 0});
		}
		else if(line.find("endmodule") != string::npos)
			break;
		else
			continue;
	}
}


// Connect Cells
void connect_cells(){
	for(auto i = 0; i < cells.size(); i++){
		for(auto j = 0; j < cells.size(); j++){
			if(cells[i].in1 == cells[j].out){
				cells[i].pre_idx.push_back(j);
				cells[i].pre_input.push_back(cells[j].out);
				cells[j].nxt_idx.push_back(i);
				cells[j].nxt_input.push_back("in1");
			}
			if(cells[i].in2 == cells[j].out){
				cells[i].pre_idx.push_back(j);
				cells[i].pre_input.push_back(cells[j].out);
				cells[j].nxt_idx.push_back(i);
				cells[j].nxt_input.push_back("in2");
			}
		}
	}
}


// Remove Pat File Comments
string removePatComments(string prgm){
	string res;

	// Flags to indicate the single line and multiple lines comments
	bool s_cmt = false;
	bool m_cmt = false;

	// Traverse the given program
	for(auto i = 0; i < prgm.length(); i++){
		// If single line comment flag is on, then check for end for it
		if(s_cmt == true && prgm[i] == '\n'){
			s_cmt = false;
		}
		// If multiple line comment is on, them check for end of it
		else if(m_cmt == true && prgm[i] == '*' && prgm[i+1] == '/'){
			m_cmt = false;
			i++;
		}

		// If this character is in a comment, ignore it
		else if(s_cmt || m_cmt)
			continue;

		// Check for beggining of comments and set the corresponding flag
		else if(prgm[i] == '/' && prgm[i+1] == '/'){
			s_cmt = true;
			i++;
			res += '\n';
		}
		else if(prgm[i] == '/' && prgm[i+1] == '*'){
			m_cmt = true;
			i++;
		}

		else if(prgm[i] == '\n')
			res += '\n';

		// If current character is a non-comment character, append it to res
		else
			res += prgm[i];
	}
	return res;
}


// Parse pat
void parse_pat(const char* filePath){
	string line;
	string str;
	string org_pat, mdf_pat;
	string str_pat;
	stringstream mdf_pat_ss, ss, ss_tmp;

	int input_num = 0;
	int pattern_idx = 0;
	ss.clear();

	ifstream fin(filePath);

	#if DEBUG
		cout << "[Open pat file]" << endl;
	#endif

	if(!fin){
		cout << "Error: Cannot open the pat file!" << endl;
		return;
	}

	while(getline(fin, line)){
		org_pat += line;
		org_pat += '\n';
	}
	
	mdf_pat = removePatComments(org_pat);
	mdf_pat_ss << mdf_pat;

	while(mdf_pat_ss >> str){
		// load input
		if(str == "input"){
			while(getline(mdf_pat_ss, line)){
				line.erase(remove(line.begin(), line.end(), ' '), line.end());
				ss << line;
				while(getline(ss, str, ',')){
					pattern.inputs.push_back(str);
					input_num ++;
					if(input_num == input_node.size())
						break;
				}
				ss.clear();

				if(input_num == input_node.size())
					break;
			}
		}
		
		str = "";
		// load pattern
		while(getline(mdf_pat_ss, line)){
			if(line != ".end"){
				ss << line;
				str_pat += line;
				str_pat += '\n';
			}
			else
				break;
		}
		str_pat.erase(remove(str_pat.begin(), str_pat.end(), '	'), str_pat.end());
		str_pat.erase(remove(str_pat.begin(), str_pat.end(), ' '), str_pat.end());
	}
	ss.clear();

	// cout << str_pat;
	
	// count pattern num
	ss << str_pat;
	pattern_num = 0;
	while(getline(ss, line))
		pattern_num += 1;

	ss.clear();

	// push back pattern
	ss << str_pat;
	pattern_idx = 0;
	pattern.values.resize(pattern_num);

	while(getline(ss, line)){
		for(auto i = 0; i < line.length(); i++){
			if(line[i] == '0')
				pattern.values[pattern_idx].push_back(0);
			else if(line[i] == '1')
				pattern.values[pattern_idx].push_back(1);
		}
		pattern_idx += 1;
		if(pattern_idx == pattern_num)
			break;
	}

	#if DEBUG
		cout << "org_pat: " << endl << org_pat << endl;
		cout << "mdf_pat: " << endl << mdf_pat << endl;
		cout << "str_pat: " << endl << str_pat << endl;
		cout << "pattern num: " << pattern_num << endl;
	#endif
}


// Write Table
vector< vector<double>> write_table(ifstream &fin, vector< vector<double>> table){
	string str, tmp_str = "";

	for(auto i = 0; i < TABLE10_ROW_NUM; i++){
		fin >> str;
		str.erase(remove(str.begin(), str.end(), '('), str.end());
		str.erase(remove(str.begin(), str.end(), '"'), str.end());
		str.erase(remove(str.begin(), str.end(), '\\'), str.end());
		str.erase(remove(str.begin(), str.end(), ';'), str.end());
		tmp_str = "";
		for(auto j = 0; j < str.length(); j++){
			if(str[j] == ',' || str[j] == ')'){
				table[i].push_back(atof(tmp_str.c_str()));
				tmp_str = "";
			}
			else
				tmp_str += str[j];
		}
	}
	return table;
}


// Parse lib file
void parse_lib(const char* filePath){
	string line;
	string str, tmp_str = "";
	stringstream ss;
	ss.clear();

	// resize tables
	nor_cell_rise.resize(TABLE10_ROW_NUM);
	nor_cell_fall.resize(TABLE10_ROW_NUM);
	nor_rise_tran.resize(TABLE10_ROW_NUM);
	nor_fall_tran.resize(TABLE10_ROW_NUM);

	inv_cell_rise.resize(TABLE10_ROW_NUM);
	inv_cell_fall.resize(TABLE10_ROW_NUM);
	inv_rise_tran.resize(TABLE10_ROW_NUM);
	inv_fall_tran.resize(TABLE10_ROW_NUM);

	nand_cell_rise.resize(TABLE10_ROW_NUM);
	nand_cell_fall.resize(TABLE10_ROW_NUM);
	nand_rise_tran.resize(TABLE10_ROW_NUM);
	nand_fall_tran.resize(TABLE10_ROW_NUM);

	ifstream fin(filePath);

	#if DEBUG
		cout << "[Open lib file]" << endl;
	#endif

	if(!fin){
		cout << "Error: Cannot open the lib file!" << endl;
		return;
	}

	while(fin >> line){
		// index_1 (total output capacitance)
		if(line == "index_1"){
			fin >> line;
			line.erase(line.begin(), line.begin() + 2); 
			line.erase(line.end() - 3, line.end()); 
			ss << line;
			while(getline(ss, str, ',')){
				total_output_cap.push_back(atof(str.c_str()));
			}
			ss.clear();
		}
		// index_2 (input transition)
		else if(line == "index_2"){
			fin >> line;
			line.erase(line.begin(), line.begin() + 2); 
			line.erase(line.end() - 3, line.end()); 
			ss << line;
			while(getline(ss, str, ',')){
				input_transition.push_back(atof(str.c_str()));
			}
			ss.clear();
		}
		// NOR2X1
		else if(line == "(NOR2X1)"){
			while(fin >> line){
				if(line == "pin(A1)"){
					while(fin >> line){
						if(line == "capacitance"){
							fin >> str;
							fin >> str;
							str.erase(str.end() - 1);
							nor2x1_a1_cap = atof(str.c_str());
							break;
						}
					}
				}
				else if(line == "pin(A2)"){
					while(fin >> line){
						if(line == "capacitance"){
							fin >> str;
							fin >> str;
							str.erase(str.end() - 1);
							nor2x1_a2_cap = atof(str.c_str());
							break;
						}
					}
				}
				else if(line == "timing()"){
					while(fin >> line){
						if(line == "cell_rise(table10){"){
							fin >> str;
							nor_cell_rise = write_table(fin, nor_cell_rise);
						}
						else if(line == "cell_fall(table10){"){
							fin >> str;
							nor_cell_fall = write_table(fin, nor_cell_fall);
						}
						else if(line == "rise_transition(table10){"){
							fin >> str;
							nor_rise_tran = write_table(fin, nor_rise_tran);
						}
						else if(line == "fall_transition(table10){"){
							fin >> str;
							nor_fall_tran = write_table(fin, nor_fall_tran);
							break;
						}
					}
				}
				else if(line == "cell")
					break;
			}
		}

		// INVX1
		else if(line == "(INVX1)"){
			while(fin >> line){
				if(line == "pin(I)"){
					while(fin >> line){
						if(line == "capacitance"){
							fin >> str;
							fin >> str;
							str.erase(str.end() - 1);
							invx1_i_cap = atof(str.c_str());
							break;
						}
					}
				}
				else if(line == "timing()"){
					while(fin >> line){
						if(line == "cell_rise(table10){"){
							fin >> str;
							inv_cell_rise = write_table(fin, inv_cell_rise);
						}
						else if(line == "cell_fall(table10){"){
							fin >> str;
							inv_cell_fall = write_table(fin, inv_cell_fall);
						}
						else if(line == "rise_transition(table10){"){
							fin >> str;
							inv_rise_tran = write_table(fin, inv_rise_tran);
						}
						else if(line == "fall_transition(table10){"){
							fin >> str;
							inv_fall_tran = write_table(fin, inv_fall_tran);
							break;
						}
					}
				}
				else if(line == "cell")
					break;
			}
		}

		// NANDX1
		else if(line == "(NANDX1)"){
			while(fin >> line){
				if(line == "pin(A1)"){
					while(fin >> line){
						if(line == "capacitance"){
							fin >> str;
							fin >> str;
							str.erase(str.end() - 1);
							nandx1_a1_cap = atof(str.c_str());
							break;
						}
					}
				}
				else if(line == "pin(A2)"){
					while(fin >> line){
						if(line == "capacitance"){
							fin >> str;
							fin >> str;
							str.erase(str.end() - 1);
							nandx1_a2_cap = atof(str.c_str());
							break;
						}
					}
				}
				else if(line == "timing()"){
					while(fin >> line){
						if(line == "cell_rise(table10){"){
							fin >> str;
							nand_cell_rise = write_table(fin, nand_cell_rise);
						}
						else if(line == "cell_fall(table10){"){
							fin >> str;
							nand_cell_fall = write_table(fin, nand_cell_fall);
						}
						else if(line == "rise_transition(table10){"){
							fin >> str;
							nand_rise_tran = write_table(fin, nand_rise_tran);
						}
						else if(line == "fall_transition(table10){"){
							fin >> str;
							nand_fall_tran = write_table(fin, nand_fall_tran);
							break;
						}
					}
				}
				else if(line == "cell")
					break;
			}
		}
	}
}


// Step 1. Check the output loading (out_cap) of each cell
void cal_loading(){
	double loading;

	// Determine if the cell coneect to ouput node (set output loading = 0.03 pf)
	for(auto i = 0; i < cells.size(); i++)
		for(auto j = 0; j < output_node.size(); j++)
			if(cells[i].out == output_node[j])
				cells[i].out_cap = OUTPUT_LOADING;
			
	// Calculate fanout loading
	for(auto i = 0; i < cells.size(); i++){
		loading = 0.;
		for(auto j = 0; j < cells.size(); j++){
			if(cells[i].out == cells[j].in1)
				loading += cells[j].in1_cap;
			if(cells[i].out == cells[j].in2)
				loading += cells[j].in2_cap;
		}
		cells[i].out_cap += loading;
	}
}


// Sort the results in descending order.
// If the output loading are the same, sort it accroding to instance number in ascending order
bool sortByload_name(const STEP1 &a, const STEP1 &b){
	if(a.load != b.load)
		return a.load > b.load;
	else
		if(a.name.length() > b.name.length())
			return a.name.length() < b.name.length();
		else
			return a.name < b.name;
}


// Sort Output Loading
void sort_output_lodaing(){
	for(auto i = 0; i < cells.size(); i++)
		step1.push_back({cells[i].name, cells[i].out_cap});
	
	sort(step1.begin(), step1.end(), sortByload_name);
}


// Write output_load_file
void write_output_load_file(const string filePath){
	ofstream fout(filePath);
	for(auto i = 0; i < step1.size(); i++)
		fout << step1[i].name << " " << step1[i].load << endl;
	
	fout.close();
}


// Step 2. Determine the delay and transition time of given input patterns
// Topological Sort
void cell_sorting(){
	Graph circuit(cells.size());
	int start;
	for(auto i = 0; i < cells.size(); ++i){
		if(cells[i].pre_idx.empty() == true)
			start = i;
		for(auto j = 0; j < cells[i].nxt_idx.size(); ++j){
			circuit.AddEdgeList(i, cells[i].nxt_idx[j]);
			// #if DEBUG
			// 	cout << "connect: " << i << ", " << cells[i].nxt_idx[j] << endl;
			// #endif
		}
	}
	circuit.TopologicalSort();
}


// Bilinear Interpolation
double bilinear_interpolation(	double x1,
							 	double x2,
  							 	double y1,
								double y2,
    							double x,
    							double y,
    							double q11,
    							double q21,
    							double q12,
    							double q22){
	double t1 = ((x2 - x)*(y2 - y)/((x2 - x1)*(y2 - y1)))*q11;
	double t2 = ((x - x1)*(y2 - y)/((x2 - x1)*(y2 - y1)))*q21;
	double t3 = ((x2 - x)*(y - y1)/((x2 - x1)*(y2 - y1)))*q12;
	double t4 = ((x - x1)*(y - y1)/((x2 - x1)*(y2 - y1)))*q22;

	return t1 + t2 + t3 + t4;
}


// Lookup Table
double lookup_table(double x, double y, vector< vector<double> > table){
	int x_end = 0;
	int x_start = total_output_cap.size() - 1;

	int y_end = 0;
	int y_start = input_transition.size() - 1;

	for(int i = total_output_cap.size() - 1; i >= 0; --i){
		if(x < total_output_cap[i])
			x_start--;
	}
	
	for(int i = 0; i < total_output_cap.size(); ++i){
		if(x >= total_output_cap[i])
			x_end++;
	}

	for(int i = input_transition.size() - 1; i >= 0; --i){
		if(y < input_transition[i])
			y_start--;
	}
	
	for(int i = 0; i < input_transition.size(); ++i){
		if(y >= input_transition[i])
			y_end++;
	}

	if(x_start < 0){
		x_start++;
		x_end++;
	}
	if(x_end > total_output_cap.size() - 1){
		x_start--;
		x_end--;
	}
	if(y_start < 0){
		y_start++;
		y_end++;
	}
	if(y_end > input_transition.size() - 1){
		y_start--;
		y_end--;
	}
	return bilinear_interpolation(	total_output_cap[x_start],
									total_output_cap[x_end],
									input_transition[y_start],
									input_transition[y_end], 
									x, 
									y,
									table[y_start][x_start],
									table[y_start][x_end],
									table[y_end][x_start],
									table[y_end][x_end]);
}


// Calculate delay & transition time
void cal_delay_trans_time(){
	int curr_cell;
	double rise_delay, fall_delay = 0.;
	double rise_trans, fall_trans = 0.;
	double tmp_delay = 0.;
	double pre_total_delay = 0.;
	
	// tables
	vector< vector<double> > cell_rise_table, cell_fall_table;
	vector< vector<double> > rise_tran_table, fall_tran_table;

	// Clean long_path vector
	for(auto i = 0; i < cells.size(); i++){
		cells[i].long_path.clear();
	}


	for(auto i = 0; i < cells_sort.size(); i++){ // topological sorted
		curr_cell = cells_sort[i];
		tmp_delay = 0.;

		// input transition time (follow the sensitization rules to choose the timing arc)
		if(cells[curr_cell].gatetype == "NOR"){
			// Find smaller cumulative delay
			// 1, 1
			if(cells[curr_cell].in1_logic == 1 && cells[curr_cell].in2_logic == 1){
				if(cells[curr_cell].pre_idx.size() == 2){
					if(cells[cells[curr_cell].pre_idx[0]].total_delay < cells[cells[curr_cell].pre_idx[1]].total_delay){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
					}

					else{
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[1]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[1]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[1]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].out);
					}
				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;
					if(cells[curr_cell].pre_input[0] == cells[curr_cell].in1){
						cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
					}
					else{
						cells[curr_cell].long_path.push_back(cells[curr_cell].in1);
					}
				}
				else{
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;

					
					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}

			// 0, 1
			else if(cells[curr_cell].in1_logic == 0 && cells[curr_cell].in2_logic == 1){
				if(cells[curr_cell].pre_idx.size() == 2){
					cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[1]].trans;
					tmp_delay = cells[cells[curr_cell].pre_idx[1]].total_delay;

					// Push path
					for(auto p = 0; p < cells[cells[curr_cell].pre_idx[1]].long_path.size(); p++){
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].long_path[p]);
					}
					cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].out);
				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					if(cells[curr_cell].pre_input[0] == cells[curr_cell].in2){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
					}
					else{ // input node -> in2
						cells[curr_cell].in_trans = 0.;
						tmp_delay = 0.;

						// Push path
						cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
					}
				}
				else{
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;

					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}

			// 1, 0
			else if(cells[curr_cell].in1_logic == 1 && cells[curr_cell].in2_logic == 0){
				if(cells[curr_cell].pre_idx.size() == 2){
					cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
					tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

					// Push path
					for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
					}
					cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					if(cells[curr_cell].pre_input[0] == cells[curr_cell].in1){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[curr_cell].in1);
					}
					else{
						cells[curr_cell].in_trans = 0.;
						tmp_delay = 0.;

						// Push path
						cells[curr_cell].long_path.push_back(cells[curr_cell].in1);
					}
				}
				else{
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;

					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}

			// Find lager cumulative delay
			// 0, 0
			else{
				if(cells[curr_cell].pre_idx.size() == 2){
					if(cells[cells[curr_cell].pre_idx[0]].total_delay > cells[cells[curr_cell].pre_idx[1]].total_delay){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
					}
					else{
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[1]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[1]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[1]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].out);
					}
				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
					tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

					// Push path
					for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
					}
					cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
				}
				else{
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;

					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}

		}
		else if(cells[curr_cell].gatetype == "INV"){
			if(cells[curr_cell].pre_idx.size() == 1){
				cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
				tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

				// Push path
				for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
					cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
				}
				cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
			}
			else{
				cells[curr_cell].in_trans = 0.;
				tmp_delay = 0.;

				// Push path
				cells[curr_cell].long_path.push_back(cells[curr_cell].in1);
			}
			pre_total_delay = tmp_delay;
		}
		else if(cells[curr_cell].gatetype == "NAND"){
			// Find smaller cumulative delay
			// 0, 0
			if(cells[curr_cell].in1_logic == 0 && cells[curr_cell].in2_logic == 0){
				if(cells[curr_cell].pre_idx.size() == 2){
					if(cells[cells[curr_cell].pre_idx[0]].total_delay < cells[cells[curr_cell].pre_idx[1]].total_delay){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
					}
					else{
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[1]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[1]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[1]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].out);
					}
				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;
					if(cells[curr_cell].pre_input[0] == cells[curr_cell].in1){
						cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
					}
					else{
						cells[curr_cell].long_path.push_back(cells[curr_cell].in1);
					}
				}
				else{
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;

					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}

			// 0, 1
			else if(cells[curr_cell].in1_logic == 0 && cells[curr_cell].in2_logic == 1){
				if(cells[curr_cell].pre_idx.size() == 2){
					cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
					tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

					// Push path
					for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
					}
					cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					if(cells[curr_cell].pre_input[0] == cells[curr_cell].in1){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[curr_cell].in1);
					}
					else{
						cells[curr_cell].in_trans = 0.;
						tmp_delay = 0.;

						// Push path
						cells[curr_cell].long_path.push_back(cells[curr_cell].in1);
					}
				}
				else{
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;

					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}

			// 1, 0
			else if(cells[curr_cell].in1_logic == 1 && cells[curr_cell].in2_logic == 0){
				if(cells[curr_cell].pre_idx.size() == 2){
					cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[1]].trans;
					tmp_delay = cells[cells[curr_cell].pre_idx[1]].total_delay;
					
					// Push path
					for(auto p = 0; p < cells[cells[curr_cell].pre_idx[1]].long_path.size(); p++){
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].long_path[p]);
					}
					cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].out);
					//

				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					if(cells[curr_cell].pre_input[0] == cells[curr_cell].in2){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
					}
					else{
						cells[curr_cell].in_trans = 0.;
						tmp_delay = 0.;

						// Push path
						cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
					}
				}
				else{
					cells[curr_cell].in_trans = 0.;
					tmp_delay = 0.;

					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}

			// Find lager cumulative delay
			// 1, 1
			else{
				if(cells[curr_cell].pre_idx.size() == 2){
					if(cells[cells[curr_cell].pre_idx[0]].total_delay > cells[cells[curr_cell].pre_idx[1]].total_delay){
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
					}
					else{
						cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[1]].trans;
						tmp_delay = cells[cells[curr_cell].pre_idx[1]].total_delay;

						// Push path
						for(auto p = 0; p < cells[cells[curr_cell].pre_idx[1]].long_path.size(); p++){
							cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].long_path[p]);
						}
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[1]].out);
					}
				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					cells[curr_cell].in_trans = cells[cells[curr_cell].pre_idx[0]].trans;
					tmp_delay = cells[cells[curr_cell].pre_idx[0]].total_delay;

					// Push path
					for(auto p = 0; p < cells[cells[curr_cell].pre_idx[0]].long_path.size(); p++){
						cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].long_path[p]);
					}
					cells[curr_cell].long_path.push_back(cells[cells[curr_cell].pre_idx[0]].out);
				}
				else{
					cells[curr_cell].in_trans = 0;
					tmp_delay = 0.;

					// Push path
					cells[curr_cell].long_path.push_back(cells[curr_cell].in2);
				}
				pre_total_delay = tmp_delay;
			}
		}
		else{
			cout << "[Error] Gate type not found!" << endl;
		}

		// Select tables
		if(cells[curr_cell].gatetype == "NOR"){
			cell_rise_table = nor_cell_rise;
			cell_fall_table = nor_cell_fall;
			rise_tran_table = nor_rise_tran;
			fall_tran_table = nor_fall_tran;
		}
		else if(cells[curr_cell].gatetype == "INV"){
			cell_rise_table = inv_cell_rise;
			cell_fall_table = inv_cell_fall;
			rise_tran_table = inv_rise_tran;
			fall_tran_table = inv_fall_tran;
		}
		else if(cells[curr_cell].gatetype == "NAND"){
			cell_rise_table = nand_cell_rise;
			cell_fall_table = nand_cell_fall;
			rise_tran_table = nand_rise_tran;
			fall_tran_table = nand_fall_tran;
		}
		else{
			cout << "[Error] Gate type not found!" << endl;
		}

		// lookup table
		double rise_delay = lookup_table(cells[curr_cell].out_cap, cells[curr_cell].in_trans, cell_rise_table);
		double fall_delay = lookup_table(cells[curr_cell].out_cap, cells[curr_cell].in_trans, cell_fall_table);

		double rise_trans = lookup_table(cells[curr_cell].out_cap, cells[curr_cell].in_trans, rise_tran_table);
		double fall_trans = lookup_table(cells[curr_cell].out_cap, cells[curr_cell].in_trans, fall_tran_table);

		if(cells[curr_cell].out_logic == 1){
			cells[curr_cell].delay = rise_delay;
			cells[curr_cell].trans = rise_trans;
		}
		else{
			cells[curr_cell].delay = fall_delay;
			cells[curr_cell].trans = fall_trans;
		}

		// Calculate total delay
		cells[curr_cell].total_delay = pre_total_delay + cells[curr_cell].delay;
	}

	// Push output node
	for(auto i = 0; i < output_node.size(); i++){
		for(auto j = 0; j < cells.size(); j++){
			if(cells[j].out == output_node[i]){
				cells[j].long_path.push_back(cells[j].out);
			}
		}
	}
}


// Step3: Find Longest & shortest delay and path of output paths
void find_longest_shortest_paths(){
	int curr_cell;
	output_paths.longest_delay = 0.;
	output_paths.shortest_delay = DBL_MAX;
	output_paths.longest_path.clear();
	output_paths.shortest_path.clear();

	for(auto i = 0; i < output_node.size(); i++){
		for(auto j = 0; j < cells_sort.size(); j++){
			curr_cell = cells_sort[j];

			if(cells[curr_cell].out == output_node[i]){
				// longer
				if(cells[curr_cell].total_delay > output_paths.longest_delay){
					output_paths.longest_delay = cells[curr_cell].total_delay;
					output_paths.longest_path.clear();
					output_paths.longest_path.assign(cells[curr_cell].long_path.begin(), cells[curr_cell].long_path.end());
				}

				// shorter
				if(cells[curr_cell].total_delay < output_paths.shortest_delay){
					output_paths.shortest_delay = cells[curr_cell].total_delay;
					output_paths.shortest_path.clear();
					output_paths.shortest_path.assign(cells[curr_cell].long_path.begin(), cells[curr_cell].long_path.end());
				}
			}
		}
	}
}


// Determine the logic, delay, transition time, and path of given input patterns
void cal_logic_delay_trans_path(){
	step2.resize(pattern_num);
	step3.resize(pattern_num);

	vector<int> remain_cells_sort;
	remain_cells_sort.assign(cells_sort.begin(), cells_sort.end());

	int curr_cell;
	for(auto k = 0; k < pattern_num; k++){ // different pattern
		#if DEBUG
			cout << "Pattern " << k + 1 << "/" << pattern_num << ": " << endl;
		#endif

		// Determine if the cell connect to input node (assign (NOR, INV, NAND)input logic & (INV) output logic)  
		for(auto i = 0; i < cells_sort.size(); i++){ // By toplogical sorting
			curr_cell = cells_sort[i];
			for(auto j = 0; j < pattern.inputs.size(); j++){ // traverse 
				// INV
				if(cells[curr_cell].gatetype == "INV"){
					if(cells[curr_cell].in1 == pattern.inputs[j]){
						
						cells[curr_cell].in1_logic = pattern.values[k][j];
						cells[curr_cell].out_logic = !cells[curr_cell].in1_logic;
						
						// find & remove cells connect with input nodes
						auto itr = find(remain_cells_sort.begin(), remain_cells_sort.end(), cells_sort[i]);
						if(itr != remain_cells_sort.end()){
							int idx = distance(remain_cells_sort.begin(), itr);
							remain_cells_sort.erase(remain_cells_sort.begin() + idx);
						}

						#if DEBUG
							cout << cells[curr_cell].gatetype << " "<< cells[curr_cell].name << ": " << cells[curr_cell].in1 << ", in1_logic: " << cells[curr_cell].in1_logic << endl;
						#endif	

					}	
				}
				// NOR, NAND
				else{ 
					if(cells[curr_cell].in1 == pattern.inputs[j]){
						cells[curr_cell].in1_logic = pattern.values[k][j];

						if(cells[curr_cell].pre_idx.size() == 0){ // if 2 input nodes connect to the target gate assign output logic, then remove the target gate in remain_cells_sort
							if(cells[curr_cell].gatetype == "NOR")
								cells[curr_cell].out_logic = !(cells[curr_cell].in1_logic | cells[curr_cell].in2_logic);
							else if(cells[curr_cell].gatetype == "NAND")
								cells[curr_cell].out_logic = !(cells[curr_cell].in1_logic & cells[curr_cell].in2_logic);

							// find & remove cells connect with input nodes
							auto itr = find(remain_cells_sort.begin(), remain_cells_sort.end(), cells_sort[i]);
							if(itr != remain_cells_sort.end()){
								int idx = distance(remain_cells_sort.begin(), itr);
								remain_cells_sort.erase(remain_cells_sort.begin() + idx);
							}
						}

						#if DEBUG
							cout << cells[curr_cell].gatetype << " "<< cells[curr_cell].name << ": " << cells[curr_cell].in1 << ", in1_logic: " << cells[curr_cell].in1_logic << endl;
						#endif
					}
					else if(cells[curr_cell].in2 == pattern.inputs[j]){
						cells[curr_cell].in2_logic = pattern.values[k][j];

						if(cells[curr_cell].pre_idx.size() == 0){ // if 2 input nodes connect to the target gate, assign output logic, then remove the target gate in remain_cells_sort
							if(cells[curr_cell].gatetype == "NOR")
								cells[curr_cell].out_logic = !(cells[curr_cell].in1_logic | cells[curr_cell].in2_logic);
							else if(cells[curr_cell].gatetype == "NAND")
								cells[curr_cell].out_logic = !(cells[curr_cell].in1_logic & cells[curr_cell].in2_logic);

							// find & remove cells connect with input nodes
							auto itr = find(remain_cells_sort.begin(), remain_cells_sort.end(), cells_sort[i]);
							if(itr != remain_cells_sort.end()){
								int idx = distance(remain_cells_sort.begin(), itr);
								remain_cells_sort.erase(remain_cells_sort.begin() + idx);
							}
						}
						
						#if DEBUG
							cout << cells[curr_cell].gatetype <<" "<< cells[curr_cell].name << ": " << cells[curr_cell].in2 << ", in2_logic: " << cells[curr_cell].in2_logic << endl;
						#endif
					}
				}	
			}
		}
		#if DEBUG
			cout << endl;
		#endif
		
		// Calculate the internal gate input logic & output logic
		for(auto i = 0; i < remain_cells_sort.size(); i++){ // By toplogical sorting
			curr_cell = remain_cells_sort[i];
			if(cells[curr_cell].gatetype == "INV"){
				cells[curr_cell].in1_logic = cells[ cells[curr_cell].pre_idx[0] ].out_logic; // current cell input logic = previous cell output logic
				cells[curr_cell].out_logic = !cells[curr_cell].in1_logic; // current cell output logic
			}
			// NOR, NAND
			else{ 
				if(cells[curr_cell].pre_idx.size() == 2){
					cells[curr_cell].in1_logic = cells[ cells[curr_cell].pre_idx[0] ].out_logic; // current cell input logic = previous cell output logic
					cells[curr_cell].in2_logic = cells[ cells[curr_cell].pre_idx[1] ].out_logic; 

				}
				else if(cells[curr_cell].pre_idx.size() == 1){
					if(cells[curr_cell].pre_input[0] == cells[curr_cell].in1)
						cells[curr_cell].in1_logic = cells[ cells[curr_cell].pre_idx[0] ].out_logic;
					else if(cells[curr_cell].pre_input[0] == cells[curr_cell].in2)
						cells[curr_cell].in2_logic = cells[ cells[curr_cell].pre_idx[0] ].out_logic;
				}
				
				if(cells[curr_cell].gatetype == "NOR")
					cells[curr_cell].out_logic = !(cells[curr_cell].in1_logic | cells[curr_cell].in2_logic);
				else if(cells[curr_cell].gatetype == "NAND")
					cells[curr_cell].out_logic = !(cells[curr_cell].in1_logic & cells[curr_cell].in2_logic);
			}	
		}
		

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Step 2
		cal_delay_trans_time();
		
		for(auto i = 0; i < cells.size(); i++){
			step2[k].push_back({cells[i].name, cells[i].out_logic, cells[i].delay, cells[i].trans});
		}

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Step 3
		find_longest_shortest_paths();

		step3[k].longest_delay = output_paths.longest_delay;
		step3[k].shortest_delay = output_paths.shortest_delay;
		step3[k].longest_path.assign(output_paths.longest_path.begin(), output_paths.longest_path.end());
		step3[k].shortest_path.assign(output_paths.shortest_path.begin(), output_paths.shortest_path.end());
		////////////////////////////////////////////////////////////////////////////////////////////

		#if DEBUG
			cout << "longest_delay: " << output_paths.longest_delay << endl;
			cout << "longest_path:  " << endl;
			for(auto i = 0; i < output_paths.longest_path.size(); i++){
				cout << output_paths.longest_path[i] << " ";
			}
			cout << endl;

			cout << "shortest_delay: " << output_paths.shortest_delay << endl;
			cout << "longest_path:  " << endl;
			for(auto i = 0; i < output_paths.shortest_path.size(); i++){
				cout << output_paths.shortest_path[i] << " ";
			}
			cout << endl << endl;
		#endif
	}


	#if DEBUG
		cout << "cells_sort (topolgical sort): " << endl; 
		for(auto i = 0; i < cells_sort.size(); i++){
			curr_cell = cells_sort[i];
			cout << cells[curr_cell].name << " ";

		}
		cout << endl << endl;
		cout << "remain_cells_sort (topolgical sort): " << endl; 
		for(auto i = 0; i < remain_cells_sort.size(); i++){
			curr_cell = remain_cells_sort[i];
			cout << cells[curr_cell].name << " ";

		}
		cout << endl << endl;
	#endif
}


// Sort By delay, then name
bool sortBydelay_name(const STEP2 &a, const STEP2 &b){
	if(a.delay != b.delay)
		return a.delay > b.delay;
	else{
		if(a.name.length() > b.name.length())
			return a.name.length() < b.name.length();
		else
			return a.name < b.name;
	}
}


// Sort by delay value
void sort_delay(){
	for(auto k = 0; k < pattern_num; k++){ 
		sort(step2[k].begin(), step2[k].end(), sortBydelay_name);
	}
}


// Write output_delay_file
void write_output_delay_file(const string filePath){
	ofstream fout(filePath);
	for(auto i = 0; i < step2.size(); i++){ 
		for(auto j = 0; j < step2[i].size(); j++){
			fout << step2[i][j].name << " " << step2[i][j].logic_output << " " << step2[i][j].delay << " " << step2[i][j].trans_time << endl;
		}
		fout << endl;
	}
	fout.close();
}


// Write output_path_file
void write_output_path_file(const string filePath){
	ofstream fout(filePath);
	for(auto i = 0; i < step3.size(); i++){
		fout << "Longest delay = " << step3[i].longest_delay;
		fout << ", the path is: ";
		for(auto j = 0; j < step3[i].longest_path.size() - 1; j++){
			fout << step3[i].longest_path[j] << " -> ";
		}
		fout << step3[i].longest_path[step3[i].longest_path.size() - 1] << endl;


		fout << "Shortest delay = " << step3[i].shortest_delay;
		fout << ", the path is: ";
		for(auto j = 0; j < step3[i].shortest_path.size() - 1; j++){
			fout << step3[i].shortest_path[j] << " -> ";
		}
		fout << step3[i].shortest_path[step3[i].shortest_path.size() - 1] << endl;

		fout << endl;
	}
	fout.close();
}


// main
int main(int argc, char **argv){
	// Assign case name
	case_name = argv[1];
	case_name = remove_substr(case_name, ".v");

	// Check netlist case & pat case is equal
	case_name_check = argv[3];
	case_name_check = remove_substr(case_name_check, ".pat");
	if(case_name != case_name_check)
		cout << "[Error] Case name not match!" << endl;

	// Assign output file path
	output_load_filePath.append("./" ).append(ID).append("_").append(case_name).append("_").append("load.txt");
	output_delay_filePath.append("./" ).append(ID).append("_").append(case_name).append("_").append("delay.txt");
	output_path_filePath.append("./" ).append(ID).append("_").append(case_name).append("_").append("path.txt");
	

	// Parse lib file
	parse_lib(argv[5]);

	// Parse netlist
	parse_netlist(argv[1]);

	// Parse pat
	parse_pat(argv[3]);
	
	// Connect cells
	connect_cells();

	// Step1: Calculate output loading
	cal_loading();
	sort_output_lodaing();
	write_output_load_file(output_load_filePath);

	// Step2: Determine the logic, delay, and transition time of given input patterns
	// Step3: Find Longest & shortest delay and path of output paths
	cell_sorting();
	cal_logic_delay_trans_path();
	sort_delay();

	write_output_delay_file(output_delay_filePath);
	write_output_path_file(output_path_filePath);

	// For Output File Path
	#if DEBUG
		cout << "[Debug] case name: " << case_name << endl;
		cout << "output_load_filePath:  " << output_load_filePath  << endl;
		cout << "output_delay_filePath: " << output_delay_filePath << endl;
		cout << "output_path_filePath:  " << output_path_filePath << endl;
	#endif


	// For Gate Level Netlist
	#if DEBUG
		cout << "[Debug] Gate Level Netlist" << endl;
		cout << "input node: " << endl;
		for(auto i = 0; i < input_node.size(); i++){
			cout << input_node[i] << ' ';
		}
		cout << endl << endl;

		cout << "output node: " << endl;
		for(auto i = 0; i < output_node.size(); i++){
			cout << output_node[i] << ' ';
		}
		cout << endl << endl;
		
		cout << "wire: " << endl;
		for(auto i = 0; i < wire.size(); i++){
			cout << wire[i] << ' ';
		}
		cout << endl << endl;

		int idx = 0;
		cout << "cells info: " << endl;
		cout << "===============================" << endl;
		for(auto i = 0; i < cells.size(); i++){
			cout << "idx:         " << idx << endl;
			cout << "name:        " << cells[i].name << endl;
			cout << "type:        " << cells[i].gatetype << endl << endl;

			cout << "in1:         " << cells[i].in1 << endl;
			cout << "in2:         " << cells[i].in2 << endl;
			cout << "in_trans:    " << cells[i].in_trans << endl;
			cout << "out:         " << cells[i].out << endl;
			cout << "in1 cap:     " << cells[i].in1_cap << endl;
			cout << "in2 cap:     " << cells[i].in2_cap << endl;
			cout << "out cap:     " << cells[i].out_cap << endl << endl;

			cout << "in1_logic:   " << cells[i].in1_logic << endl;
			cout << "in2_logic:   " << cells[i].in2_logic << endl;
			cout << "out_logic:   " << cells[i].out_logic << endl << endl;

			cout << "delay:       " << cells[i].delay << endl;
			cout << "total_delay: " << cells[i].total_delay << endl << endl;

			cout << "trans:       " << cells[i].trans << endl;

			cout << "pre idx:  " << endl;

			for(auto j = 0; j < cells[i].pre_idx.size(); j++)
				cout << cells[i].pre_idx[j] << endl;

			cout << "pre input:" << endl;
			for(auto j = 0; j < cells[i].pre_input.size(); j++)
				cout << cells[i].pre_input[j] << endl;

			cout << "nxt idx:  " << endl;
			for(auto j = 0; j < cells[i].nxt_idx.size(); j++)
				cout << cells[i].nxt_idx[j] << endl;

			cout << "nxt input:" << endl;
			for(auto j = 0; j < cells[i].nxt_input.size(); j++)
				cout << cells[i].nxt_input[j] << endl;

			cout << "long path:" << endl;
			for(auto j = 0; j < cells[i].long_path.size(); j++)
				cout << cells[i].long_path[j] << " ";

			cout << endl;

			cout << "===============================" << endl;
			idx++;
		}
		cout << endl << endl;
	#endif

	// For pat File
	#if DEBUG
		cout << "[Debug] pat input: " << endl;
		for(auto i = 0; i < pattern.inputs.size(); i++)
			cout << pattern.inputs[i] << " ";
		cout << endl;

		cout << "[Debug] pattern num: " << pattern_num << endl;
		cout << "[Debug] pat: " << endl;
		for(auto i = 0; i < pattern.values.size(); i++){
			for(auto j = 0; j < pattern.values[i].size(); j++){
				cout << pattern.values[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl << endl;
	#endif
	
	// For lib File
	#if DEBUG
		cout << "[Debug] Lib File" << endl;
		/////////////////////////////////////////////////////////////
		cout << "=== index_1 (total output capacitance) ===" << endl;
		for(auto i = 0; i < total_output_cap.size(); i++){
			cout << total_output_cap[i] << ' ';
		}
		cout << endl << endl;

		/////////////////////////////////////////////////////////////
		cout << "=== index_2 (input transition) ===" << endl;
		for(auto i = 0; i < input_transition.size(); i++){
			cout << input_transition[i] << ' ';
		}
		cout << endl << endl;

		/////////////////////////////////////////////////////////////
		cout << "=========== cell (NOR2X1) info ===========" << endl;
		cout << "A1 cap: " << nor2x1_a1_cap << endl;
		cout << "A2 cap: " << nor2x1_a2_cap << endl;
		cout << endl;

		cout << "cell rise: " << endl;
		for(auto i = 0; i < nor_cell_rise.size(); i++){
			for(auto j = 0; j < nor_cell_rise[i].size(); j++){
				cout << nor_cell_rise[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "cell fall: " << endl;
		for(auto i = 0; i < nor_cell_fall.size(); i++){
			for(auto j = 0; j < nor_cell_fall[i].size(); j++){
				cout << nor_cell_fall[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "rise transition: " << endl;
		for(auto i = 0; i < nor_rise_tran.size(); i++){
			for(auto j = 0; j < nor_rise_tran[i].size(); j++){
				cout << nor_rise_tran[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "fall transition: " << endl;
		for(auto i = 0; i < nor_fall_tran.size(); i++){
			for(auto j = 0; j < nor_fall_tran[i].size(); j++){
				cout << nor_fall_tran[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;
		/////////////////////////////////////////////////////////////
		cout << "=========== cell (INVX1) info ===========" << endl;
		cout << "input cap: " << invx1_i_cap << endl;
		cout << endl;

		cout << "cell rise: " << endl;
		for(auto i = 0; i < inv_cell_rise.size(); i++){
			for(auto j = 0; j < inv_cell_rise[i].size(); j++){
				cout << inv_cell_rise[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "cell fall: " << endl;
		for(auto i = 0; i < inv_cell_fall.size(); i++){
			for(auto j = 0; j < inv_cell_fall[i].size(); j++){
				cout << inv_cell_fall[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "rise transition: " << endl;
		for(auto i = 0; i < inv_rise_tran.size(); i++){
			for(auto j = 0; j < inv_rise_tran[i].size(); j++){
				cout << inv_rise_tran[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "fall transition: " << endl;
		for(auto i = 0; i < inv_fall_tran.size(); i++){
			for(auto j = 0; j < inv_fall_tran[i].size(); j++){
				cout << inv_fall_tran[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;
		/////////////////////////////////////////////////////////////
		cout << "=========== cell (NANDX1) info ===========" << endl;
		cout << "A1 cap: " << nandx1_a1_cap << endl;
		cout << "A2 cap: " << nandx1_a2_cap << endl;
		cout << endl;

		cout << "cell rise: " << endl;
		for(auto i = 0; i < nand_cell_rise.size(); i++){
			for(auto j = 0; j < nand_cell_rise[i].size(); j++){
				cout << nand_cell_rise[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "cell fall: " << endl;
		for(auto i = 0; i < nand_cell_fall.size(); i++){
			for(auto j = 0; j < nand_cell_fall[i].size(); j++){
				cout << nand_cell_fall[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "rise transition: " << endl;
		for(auto i = 0; i < nand_rise_tran.size(); i++){
			for(auto j = 0; j < nand_rise_tran[i].size(); j++){
				cout << nand_rise_tran[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << "fall transition: " << endl;
		for(auto i = 0; i < nand_fall_tran.size(); i++){
			for(auto j = 0; j < nand_fall_tran[i].size(); j++){
				cout << nand_fall_tran[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;
	#endif
	


	#if DEBUG
		cout << "[Debug] Sort Output loading" << endl;
		for(auto i = 0; i < step1.size(); i++){
			cout << step1[i].name << " " << step1[i].load << endl;
		}
	#endif
	

	#if DEBUG
		cout << "[Debug] Step2: name, logic output, delay, trainsition time" << endl;
			
		for(auto i = 0; i < step2.size(); i++){
			for(auto j = 0; j < step2[i].size(); j++){
				cout << step2[i][j].name << " " << step2[i][j].logic_output << " " << step2[i][j].delay << " " << step2[i][j].trans_time << endl;
			}
			cout << endl;
		}
		cout << endl << endl;
	#endif

	#if DEBUG
		cout << "[Debug] Step3: " << endl;
			
		for(auto i = 0; i < step3.size(); i++){
			cout << "Pattern: " << i << endl << endl;

			cout << "longest_delay: " << step3[i].longest_delay << endl;
			cout << "longest_path:  " << endl;

			for(auto j = 0; j < step3[i].longest_path.size(); j++){
				cout << step3[i].longest_path[j] << " ";
			}
			cout << endl << endl;

			cout << "shortest_delay: " << step3[i].shortest_delay << endl;
			cout << "shortest_path:  " << endl;

			for(auto j = 0; j < step3[i].shortest_path.size(); j++){
				cout << step3[i].shortest_path[j] << " ";
			}
			cout << endl << "--------------------------------" << endl;
		}
		cout << endl << endl;
	#endif
}